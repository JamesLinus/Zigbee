#include <ioCC2530.h>
#include "OSAL.h"
#include "Temp.h"
#include "OSAL_Timers.h"
#include "ATcmd.h"
#include <string.h>

#define uchar unsigned char
#define uint unsigned int

extern unsigned char Temp_TaskID;
extern ATcmd ATPacket;
void InitUart();              //初始化串口
void Uart_Send_String(unsigned char *Data);

unsigned char temp[50] = {0};//临时存储反馈的字符串
unsigned char count = 0;	//接收到反馈字符串的长度


/****************************************************************
   串口初始化函数
***********************************************************/
void InitUart()
{
    CLKCONCMD &= ~0x40; // 设置系统时钟源为 32MHZ晶振
    while(CLKCONSTA & 0x40);                     // 等待晶振稳定
    CLKCONCMD &= ~0x47;                          // 设置系统主时钟频率为 32MHZ

  PERCFG&=~0x01;   //有2个备用位置，0使用备用位置1；1使用备用位置2
  P0SEL |= 0x0C;   //P0_2 RXD P0_3 TXD 外设功能 0000 1100

  U0CSR |= 0xC0;  //串口接收使能  1100 0000 工作UART模式+允许接受
  U0UCR |= 0x00;  //无奇偶校验，1位停止位

  U0GCR |= 11;           //U0GCR与U0BAUD配合
  U0BAUD |= 216;       // 波特率设为115200

//  IEN0 |= 0X04;     //开串口接收中断 'URX0IE = 1',也可以写成 URX0IE=1;
  URX0IE=1;
  EA=1;

}

void Uart0_SendCh(char ch)
{
    U0DBUF = ch;
    while(UTX0IF == 0);
    UTX0IF = 0;
}
/****************************************************************
串口发送字符串函数
****************************************************************/
//void Uart_Send_String(unsigned char *Data,int len)
//{
// {
//  int j;
//  for(j=0;j<len;j++)
//  {
//     Uart0_SendCh(*Data++);
//  }
// }
//}


void Uart_Send_String(unsigned char *Data)
{
  while (*Data != '\0')
  {
     Uart0_SendCh(*Data++);
  }
}


#pragma vector = URX0_VECTOR
__interrupt void recvdInterrupt(void)//串口接收中断函数
{
	URX0IF = 0;//接收完成自动置1， 进入中断函数之后手动清0

	//读取反馈的字符串
	unsigned char i = 0;
	temp[count] = U0DBUF;
	count++;
	
	if (temp[count - 2] == 0x0d && temp[count - 1] == 0x0a )	//接收到完整的反馈，存到结构体当中并处理
	{
	  if(count > 2)
	  {
		if (strstr(temp, "+HTTPACTION"))	//该反馈是HTTP回执
		{
		  ATPacket.reqCode = FEEDBACK;
		}
		for (i = 0; i < count; i++)
		{
			ATPacket.rcv[i] = temp[i];
			temp[i] = 0;
		}
		count = 0;
		
		osal_set_event(Temp_TaskID,AT_CMD_EVT);
	  }
	  else 
		count = 0;
	}
	
	else if (count >= 50)		//大于50个字符还没有收到完整的反馈
	{
	  	count = 0;
	}
}
