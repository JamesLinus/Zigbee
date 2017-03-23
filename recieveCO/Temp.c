/**************************************************************************************************
  Filename:       Temp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Generic Application (no Profile).


  Copyright 2004-2009 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends "Hello World" to another "Generic"
  application every 15 seconds.  The application will also
  receive "Hello World" packets.

  The "Hello World" messages are sent/received as MSG type message.

  This applications doesn't have a profile, so it handles everything
  directly - itself.

  Key control:
    SW1:
    SW2:  initiates end device binding
    SW3:
    SW4:  initiates a match description request
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "Temp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

#include "UART.h"
#include "74LS164_8LED.h"
#include "ATcmd.h"

#define FAILCOUNT 8

unsigned char failCount = 0;//某个AT命令发送失败的次数,超过一定次数就默认SIM900A断开，从新初始化

unsigned char tmp = 0x44;//温度

unsigned char sendCode = 0;//发送AT指令的代码

//unsigned char initSimFail = 1;//初始化Sim900A失败标志
//unsigned char signalFail = 1;//信号检测失败标志
//unsigned char conTypeFail = 1;//设置连接类型失败标识
//unsigned char conPointFail = 1;//设置接入点失败标志

unsigned char sucFlag[15] = {0};

unsigned char rcvTempFlag = 0;//接收到温度数据标志
unsigned char simReady = 0;
unsigned char sendOK = 1;

char temperature[3] = {0xff,0xff,'\0'};

char rawURLcmd[150] = "AT+HTTPPARA=\"URL\",http://40060210.nat123.net/SimServer/storeDataServlet?ID=SIM900A&temperature=";
char URLcmd[150] = {0};
unsigned char table[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
extern ATcmd ATPacket;
int i = 0;

void setFailFlag(void)
{
  unsigned char i = 0;
  for (i = 0; i < 15; i++)
  {
	sucFlag[i] = 0;
  }
}

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t Temp_ClusterList[Temp_MAX_CLUSTERS] =
{
  Temp_CLUSTERID
};

const SimpleDescriptionFormat_t Temp_SimpleDesc =
{
  Temp_ENDPOINT,              //  int Endpoint;
  Temp_PROFID,                //  uint16 AppProfId[2];
  Temp_DEVICEID,              //  uint16 AppDeviceId[2];
  Temp_DEVICE_VERSION,        //  int   AppDevVer:4;
  Temp_FLAGS,                 //  int   AppFlags:4;
  Temp_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)Temp_ClusterList,  //  byte *pAppInClusterList;
  Temp_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)Temp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in Temp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t Temp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte Temp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // Temp_Init() is called.
devStates_t Temp_NwkState;


byte Temp_TransID;  // This is the unique message ID (counter)

afAddrType_t Temp_DstAddr;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void Temp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void Temp_HandleKeys( byte shift, byte keys );
void Temp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void Temp_SendTheMessage( void );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Temp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void  Temp_Init( byte task_id )
{
  Temp_TaskID = task_id;
  Temp_NwkState = DEV_INIT;
  Temp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  Temp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  Temp_DstAddr.endPoint = 0;
  Temp_DstAddr.addr.shortAddr = 0;

  // Fill out the endpoint description.
  Temp_epDesc.endPoint = Temp_ENDPOINT;
  Temp_epDesc.task_id = &Temp_TaskID;
  Temp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&Temp_SimpleDesc;
  Temp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &Temp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( Temp_TaskID );

  // Update the display
#if defined ( LCD_SUPPORTED )
    HalLcdWriteString( "Temp", HAL_LCD_LINE_1 );
#endif

  ZDO_RegisterForZDOMsg( Temp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( Temp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      Temp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
UINT16 Temp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( Temp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          Temp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          Temp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // Action taken when confirmation is received.
          if ( sentStatus != ZSuccess )
          {
            // The data wasn't delivered -- Do something
          }
          break;

        case AF_INCOMING_MSG_CMD:
          Temp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          Temp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if  (Temp_NwkState == DEV_ZB_COORD)
		  {
			LS164_BYTE(11);
			sendCode = ATI;
//			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
			
		  }
		
          if(Temp_NwkState == DEV_ROUTER)
		  {
			LS164_BYTE(12);
		  }
		
          if (Temp_NwkState == DEV_END_DEVICE)
          {
			  LS164_BYTE(13);
            // Start sending "the" message in a regular interval.
//            osal_start_timerEx( Temp_TaskID,
//                                Temp_SEND_MSG_EVT,
//                              Temp_SEND_MSG_TIMEOUT );
//			osal_set_event(Temp_TaskID, KEY_EVENT);
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( Temp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }		

  if( events & TMP_REQUEST_EVT )//请求温度事件，自定义事件
  {
	
	unsigned char theMessageData = TMP_REQUEST_CMD;
	Temp_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//通信方式是广播
	Temp_DstAddr.addr.shortAddr = 0xFFFF;//目标地址是协调器0xFFFF表示目标地址是网络中的所有节点
 	// Take the first endpoint, Can be changed to search through endpoints
 	Temp_DstAddr.endPoint =Temp_ENDPOINT ;//端点

 	AF_DataRequest( &Temp_DstAddr,
 					&Temp_epDesc,
                 	Temp_CLUSTERID,
                 	1,//(byte)osal_strlen( theMessageData ) + 1,//数据包长度,字符串的长度加’\0’
                 	 &theMessageData,//(byte *)&theMessageData,//数据包首地址
                 	 &Temp_TransID,
                 	  AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );
	  return ( events ^ TMP_REQUEST_EVT);
  }
  
  if( events & AT_CMD_EVT )	//AT命令控制SIM900A事件,串口中断函数中调用,处理接收到的SIM900A反馈
  {
	
	switch (ATPacket.reqCode)
	{
		case ATI:
		  if (strstr(ATPacket.rcv,"OK"))//波特率同步完成
		  {
			sucFlag[ATI] = 1;
			failCount = 0;
			//检测是否联网
			sendCode = ATE0;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else
		  {
			sucFlag[ATI] = 0;
			sendCode = ATI;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		  
		case ATE0:
		  if (strstr(ATPacket.rcv, "OK"))
		  {
			sucFlag[ATE0] = 1;
			sendCode = COPS;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 1000);
		  }
		  else
		  {
			sucFlag[ATE0] = 0;
			sendCode = ATE0;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		case COPS:
		  if (strstr(ATPacket.rcv, "OK"))//连接上网络
		  {
			sucFlag[COPS] = 1;
			failCount = 0;
			//设置GPRS连接
			sendCode = CONTYPE;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else
		  {
			sucFlag[COPS] = 0;
			sendCode = COPS;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		  
		case CONTYPE:
	  		if (strstr(ATPacket.rcv, "OK"))
			{
			  failCount = 0;
			  sucFlag[CONTYPE] = 1;
			  //设置接入点
			  sendCode = CONPOINT;
			  osal_start_timerEx(Temp_TaskID,SEND_AT_EVT,1000);
			}
			else
			{
			  sucFlag[CONTYPE] = 0;
			  sendCode = CONTYPE;
			  osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
			}
		  break;
		  
		case CONPOINT:
		  if (strstr(ATPacket.rcv,"OK"))
		  {
			failCount = 0;
			sucFlag[CONPOINT] = 1;
			LS164_BYTE(2);
			//TODO
			sendCode = START;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else 
		  {
			sucFlag[CONPOINT] = 0;
			sendCode = CONPOINT;
			osal_start_timerEx(Temp_TaskID,SEND_AT_EVT,200);
//			LS164_BYTE(6);
		  }
		  break;
		  
		case START:
		  if (strstr(ATPacket.rcv, "OK"))
		  {
			LS164_BYTE(3);
			failCount = 0;
			sucFlag[START] = 1;
			//初始化HTTP
			sendCode = INITHTTP;
//			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else if(strstr(ATPacket.rcv, "ERROR"))
		  {
			sucFlag[START] = 0;
			sendCode = STOP;	//打开承载失败后先关闭承载之后在打开
			osal_start_timerEx(Temp_TaskID,SEND_AT_EVT,200);
		  }
		  break;
	  
		case STOP:
		  if (strstr(ATPacket.rcv, "OK"))	//关闭承载成功，打开承载
		  {
			LS164_BYTE(4);
			sucFlag[STOP] = 1;
			failCount = 0;
			sucFlag[START] = 0;
			sendCode = START;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else if(strstr(ATPacket.rcv, "ERROR"))//关闭承载失败，说明承载处于关闭状态
		  {
			sucFlag[START] = 0;
			sendCode = START;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		  
		case INITHTTP:
		  if (strstr(ATPacket.rcv,"OK"))	//HTTP 初始化成功
		  {
			LS164_BYTE(5);
			sucFlag[INITHTTP] = 1;
			failCount = 0;
			//设置承载上下文
			sendCode = SETCID;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
			
		  }
		  else if (strstr(ATPacket.rcv, "ERROR"))	//HTTP初始化失败，关闭之后再打开
		  {
			sucFlag[INITHTTP] = 0;
		  	sendCode = TERMHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		  
		case TERMHTTP:
		  if (strstr(ATPacket.rcv,"OK"))	//终止HTTP成功，打开HTTP
		  {
			failCount = 0;
			sucFlag[INITHTTP] = 0;
			sendCode = INITHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
		  }
		  else 	//终止HTTP失败，说明已经终止状态，打开HTTP
		  {
			sendCode = INITHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
			  
		case SETCID:
		  if (strstr(ATPacket.rcv,"OK"))
		  {
			failCount = 0;
			LS164_BYTE(6);
			sucFlag[SETCID] = 1;
			simReady = 1;
			//TODO
			if (rcvTempFlag == 1)	//已经收到温度数据，直接发送
			{
				sendCode = SETURL;
				osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,1000);
			}
			else	//还没有收到温度数据
			{
			  
			}
		  }
		  else 
		  {
			sucFlag[SETCID] = 0;
			sendCode = SETCID;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,200);
		  }
		  break;
		  
		case SETURL:
		  if (strstr(ATPacket.rcv,"OK"))
		  {
			failCount = 0;
			LS164_BYTE(7);
			sucFlag[SETURL] = 1;
			//发送HTTP请求
			sendCode = REQHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 1000);
		  }
		  else
		  {
			sucFlag[SETURL] = 0;
			sendCode = SETURL;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 200);
		  }
		  break;
		  
		case REQHTTP:
		  if (strstr(ATPacket.rcv,"OK"))//HTTP请求发送成功,等待HTTP回执
		  {
			failCount = 0;
			sucFlag[REQHTTP] = 1;
			LS164_BYTE(8);
			//
			
		  }
		  else
		  {
			LS164_BYTE(12);
			sucFlag[REQHTTP] = 0;
			sendCode = REQHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 200);
		  }
		  break;
			  
		case FEEDBACK:
		  if (strstr(ATPacket.rcv, "200"))	//HTTP成功发送
		  {
			failCount = 0;
			sucFlag[FEEDBACK] = 1;
			LS164_BYTE(9);
			rcvTempFlag = 0;
			temperature[0] = 0xff;
			temperature[1] = 0xff;
			temperature[2] = '\0';
			rcvTempFlag = 0;
			//清零重头开始
			failCount = 0;
		    setFailFlag();//重设失败标志
	 	    clearATPacket();
	 		ATPacket.reqCode = 0;
	 	    LS164_BYTE(0);
	  		simReady = 0;
			rcvTempFlag = 0;
		  }
		  else 
		  {
			LS164_BYTE(1);
			sendCode = REQHTTP;
			osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 200);
		  }
		  break;
		  
		default:
	  		break;
	  
	}
	clearATPacket();
	return ( events ^ AT_CMD_EVT);
  }
  
  if ( events & SEND_AT_EVT )	//发送AT命令事件
  {
	if (failCount >= FAILCOUNT)
	{
	  failCount = 0;
	  setFailFlag();//重设失败标志
	  clearATPacket();
	  ATPacket.reqCode = 0;
	  sendCode = ATI;
	  osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,10000);
	  LS164_BYTE(0);
	  simReady = 0;
	  return events ^ SEND_AT_EVT;
	}

	switch(sendCode)
	{
	case ATI:
	  if (sucFlag[ATI] == 0);
	  {
		sendATcmd(ATI);
		sendCode = ATI;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
		failCount++;
	  }
	  break;
	  
	case ATE0:
	  if (sucFlag[ATE0] == 0)
	  {
		sendATcmd(ATE0);
		sendCode = ATE0;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
		failCount++;
	  }
	  break;
	  
	case COPS:
	  if (sucFlag[COPS] == 0)
	  {
		sendATcmd(COPS);
		sendCode = COPS;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
		failCount++;
	  }
	  break;
	  
	case CONTYPE:
	  if (sucFlag[CONTYPE] == 0)
	  {
		sendATcmd(CONTYPE);
		sendCode = CONTYPE;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case CONPOINT:
	  if (sucFlag[CONPOINT] == 0)
	  {
		sendATcmd(CONPOINT);
		sendCode = CONPOINT;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case START:
	  if (sucFlag[START] == 0)
	  {
		sendATcmd(START);
		sendCode = START;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case STOP:
	  if (sucFlag[STOP] == 0)
	  {
		sendATcmd(STOP);
		sendCode = STOP;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case INITHTTP:
	  if (sucFlag[INITHTTP] == 0)
	  {
		sendATcmd(INITHTTP);
		sendCode = INITHTTP;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case TERMHTTP:
	  if (sucFlag[TERMHTTP] == 0)
	  {
		sendATcmd(TERMHTTP);
		sendCode = TERMHTTP;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case SETCID:
	  if (sucFlag[SETCID] == 0)
	  {
		sendATcmd(SETCID);
		sendCode = SETCID;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT,3000);
		failCount++;
	  }
	  break;
	  
	case SETURL:
	  if (sucFlag[SETURL] == 0)
	  {
		sendATcmd(SETURL);
		sendCode = SETURL;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
		failCount++;
	  }
	  break;
	case REQHTTP:
	  if (sucFlag[REQHTTP] == 0 )
	  {
		sendATcmd(REQHTTP);
		sendCode = REQHTTP;
		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
		failCount++;
	  }
	  break;
	default:
	  break;
	}
	return events ^ SEND_AT_EVT;
  }

  // Send a message out - This event is generated by a timer
  //  (setup in Temp_Init()).

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */

/*********************************************************************
 * @fn      Temp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void Temp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined(BLINK_LEDS)
      else
      {
        // Flash LED to show failure
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            Temp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            Temp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            Temp_DstAddr.endPoint = pRsp->epList[0];

            // Light LED
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      Temp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void Temp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;

  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                            Temp_epDesc.endPoint,
                            Temp_PROFID,
                            Temp_MAX_CLUSTERS, (cId_t *)Temp_ClusterList,
                            Temp_MAX_CLUSTERS, (cId_t *)Temp_ClusterList,
                            FALSE );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      // Initiate a Match Description Request (Service Discovery)
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        Temp_PROFID,
                        Temp_MAX_CLUSTERS, (cId_t *)Temp_ClusterList,
                        Temp_MAX_CLUSTERS, (cId_t *)Temp_ClusterList,
                        FALSE );
    }
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      Temp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void Temp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{ 
	
	P0SEL &= 0xef;//1110 1111
	P0DIR |= 0x10;//0001 0000
	P0_4 ^= 1;//灯取反
	temperature[0] = pkt->cmd.Data[0];
	temperature[1] = pkt->cmd.Data[1];
	temperature[2] = pkt->cmd.Data[2];//devID
	
	char devID[] = {temperature[2]+'0','\0'};
//	Uart0_SendCh(temperature[0]);
//	Uart0_SendCh(temperature[1]);
	unsigned char temp[17] = {0};
	int j;
	 for (j = 0; j < 8; j++)
	  {
		if (temperature[0] & table[j])
		  temp[j] = '1';
		else 
		  temp[j] = '0';
		if (temperature[1] & table[j])
		  temp[8+j] = '1';
		else 
		  temp[8+j] = '0';
	  }
	temp[16] = '\0';
	strcpy(URLcmd, rawURLcmd);
	strcat(URLcmd,temp);
	strcat(URLcmd,"&devID=");
	strcat(URLcmd,devID);
  	strcat(URLcmd,"\r\n");
	rcvTempFlag = 1;
	Uart_Send_String(URLcmd);
//	if (simReady == 1)	//SIM900A已经准备好，直接可以发送
//	{
//	  sendCode = SETURL;
//	  osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 1000);
//	}
//	else
//	{
//		sendCode = ATI;
//		osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
//	}
}



/*********************************************************************
 * @fn      Temp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
void Temp_SendTheMessage( void )
{
  char theMessageData[] = "Hello World";

  if ( AF_DataRequest( &Temp_DstAddr, &Temp_epDesc,
                       Temp_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &Temp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // Successfully requested to be sent.
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
*********************************************************************/
