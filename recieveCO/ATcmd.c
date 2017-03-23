#include <iocc2530.h>
#include "ATcmd.h"
#include "Temp.h"
#include "OSAL.h"
#include "OSAL_Timers.h"
#include "UART.h"


ATcmd ATPacket = {0,NULL};
extern unsigned char sendCode;
extern unsigned char initSimFail;
extern byte Temp_TaskID;
extern char URLcmd[150];
void InitSim900A(void)
{
  if (initSimFail == 1)
  {
	  sendCode = ATI;
	  osal_start_timerEx(Temp_TaskID, SEND_AT_EVT, 3000);
	  
  }
  
}

void clearATPacket(void)
{
  unsigned char i = 0;
  for (i = 0; i < 50; i++)
  {
	ATPacket.rcv[i] = 0;
  }
}

void sendATcmd(unsigned char reqCode)
{
	switch (reqCode)
	{
	case ATI:
	  ATPacket.reqCode = ATI;
//	  Uart_Send_String("ATI\r\n",5);
	  Uart_Send_String("ATI\r\n");
	  break;
	  
	case ATE0:
	  ATPacket.reqCode = ATE0;
	  Uart_Send_String("ATE0\r\n");
	  break;
	  
	case COPS:
	  ATPacket.reqCode = COPS;
//	  Uart_Send_String("AT+COPS?\r\n",10);
	   Uart_Send_String("AT+COPS?\r\n");
	  break;
	  
	case CONTYPE:
	  ATPacket.reqCode = CONTYPE;
//	  Uart_Send_String("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",29);
	  Uart_Send_String("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
	  break;
	  
	case CONPOINT:
	  ATPacket.reqCode = CONPOINT;
	  Uart_Send_String("AT+SAPBR=3,1,\"APN\",\"UNINET\"\r\n");
	  break;
	  
	case START:
	  ATPacket.reqCode = START;
	  Uart_Send_String("AT+SAPBR=1,1\r\n");
	  break;
	  
	case STOP:
	  ATPacket.reqCode = STOP;
	  Uart_Send_String("AT+SAPBR=0,1\r\n");
	  break;
	  
	  case INITHTTP:
		ATPacket.reqCode = INITHTTP;
		Uart_Send_String("AT+HTTPINIT\r\n");
		break;
		
	case SETCID:
	  ATPacket.reqCode = SETCID;
	  Uart_Send_String("AT+HTTPPARA=\"CID\",1\r\n");
	  break;
	  
	case SETURL:	//温度数据整合到URL中
	  ATPacket.reqCode = SETURL;
	  Uart_Send_String(URLcmd);
	  break;
	  
	case REQHTTP:
	  ATPacket.reqCode = REQHTTP;
	  Uart_Send_String("AT+HTTPACTION=0\r\n");
	  break;
	  
	case TERMHTTP:
	  ATPacket.reqCode = TERMHTTP;
	  Uart_Send_String("AT+HTTPTERM\r\n");
	  break;
	default:
	  ATPacket.reqCode = 0;
	  break;
	}
}
