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

#include "DS18B20.h"
#include "UART.h"
#include "74LS164_8LED.h"

#define devID 2
#define DELAY_MIN 30//延迟发送温度数据时间

unsigned char tmp = 0x66;//温度
unsigned char incomingCmd = 0;//是否接收到无线数据命令

unsigned char firstTime = 1;
unsigned char temperature[4] = {0xff, 0xff, devID,'\0'};//记录温度数据的数组
unsigned char delayCount = 0;

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

        case AF_INCOMING_MSG_CMD://接收到射频数据消息
          Temp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          Temp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if  (Temp_NwkState == DEV_ZB_COORD)
		  {
			LS164_BYTE(11);
		  }
		
          if(Temp_NwkState == DEV_ROUTER)
		  {
			LS164_BYTE(12);
		  }
		
          if (Temp_NwkState == DEV_END_DEVICE)
          {
			  LS164_BYTE(13);
			  firstTime = 1;
			  osal_set_event(Temp_TaskID, DELAY_EVENT);
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

//  // Send a message out - This event is generated by a timer
//  //  (setup in Temp_Init()).
//  if ( events & Temp_SEND_MSG_EVT )
//  {
//    // Send "the" message
//    Temp_SendTheMessage();
//
//    // Setup to send message again
//    osal_start_timerEx( Temp_TaskID,
//                        Temp_SEND_MSG_EVT,
//                      Temp_SEND_MSG_TIMEOUT );
//
//    // return unprocessed events
//    return (events ^ Temp_SEND_MSG_EVT);
//  }

  if ( events & TEST_TMP_EVENT )//测温度事件
  {
	osal_start_timerEx(Temp_TaskID,DELAY_EVENT,60000);
		incomingCmd = 0;
		tmp = get_temperature();
		if (tmp == 0xf0)//初始化失败
			osal_set_event(Temp_TaskID,SEND_TMP_EVENT);
		else
			osal_start_timerEx(Temp_TaskID, READ_TMP_EVENT, 1000);//等待1秒钟DS18B20温度转换完成，发送读温度事件
	return (events ^ TEST_TMP_EVENT);
  }

  if (events & READ_TMP_EVENT)//读温度事件
  {
	tmp = read_temperature();
//	Uart0_SendCh(tmp);
	osal_set_event(Temp_TaskID,SEND_TMP_EVENT);
	return (events ^ READ_TMP_EVENT);
  }


  if ( events & SEND_TMP_EVENT )//发送温度数据事件
  {
    unsigned char theMessageData = tmp;
	Temp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            Temp_DstAddr.addr.shortAddr = 0x0000;//目标地址是协调器0x0000
            // Take the first endpoint, Can be changed to search through endpoints
            Temp_DstAddr.endPoint =Temp_ENDPOINT ;
			
	
	//发送数组数据
	AF_DataRequest( &Temp_DstAddr, &Temp_epDesc,
                       Temp_CLUSTERID,
                       (byte)osal_strlen( temperature ) + 1,
                       (byte *)&temperature,
                       &Temp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS );
	
	
	
	  return (events ^ SEND_TMP_EVENT);
  }
  
  if ( events & DELAY_EVENT)
  {
	if (firstTime == 1)
	{
	  if (delayCount == devID*2)
	  {
		osal_set_event(Temp_TaskID,TEST_TMP_EVENT);
		firstTime = 0;
		delayCount = 0;
	  }
	  else
	  {
		delayCount++;
		osal_start_timerEx(Temp_TaskID,DELAY_EVENT,60000);
	  }
	}
	else
	{
		if (++delayCount == DELAY_MIN)
		{
		  osal_set_event(Temp_TaskID,TEST_TMP_EVENT);
		  delayCount = 0;
		}
		else
		{
		  osal_start_timerEx(Temp_TaskID,DELAY_EVENT,60000);
		}
	}
	return (events ^ DELAY_EVENT);
  }
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

    if (TMP_REQUEST_CMD == *(pkt->cmd.Data))
	{
		osal_set_event(Temp_TaskID, TEST_TMP_EVENT);
		incomingCmd = 1;
		P0_4 ^= 1;
	}
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
