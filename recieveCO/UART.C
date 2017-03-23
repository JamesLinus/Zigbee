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
void InitUart();              //��ʼ������
void Uart_Send_String(unsigned char *Data);

unsigned char temp[50] = {0};//��ʱ�洢�������ַ���
unsigned char count = 0;	//���յ������ַ����ĳ���


/****************************************************************
   ���ڳ�ʼ������
***********************************************************/
void InitUart()
{
    CLKCONCMD &= ~0x40; // ����ϵͳʱ��ԴΪ 32MHZ����
    while(CLKCONSTA & 0x40);                     // �ȴ������ȶ�
    CLKCONCMD &= ~0x47;                          // ����ϵͳ��ʱ��Ƶ��Ϊ 32MHZ

  PERCFG&=~0x01;   //��2������λ�ã�0ʹ�ñ���λ��1��1ʹ�ñ���λ��2
  P0SEL |= 0x0C;   //P0_2 RXD P0_3 TXD ���蹦�� 0000 1100

  U0CSR |= 0xC0;  //���ڽ���ʹ��  1100 0000 ����UARTģʽ+��������
  U0UCR |= 0x00;  //����żУ�飬1λֹͣλ

  U0GCR |= 11;           //U0GCR��U0BAUD���
  U0BAUD |= 216;       // ��������Ϊ115200

//  IEN0 |= 0X04;     //�����ڽ����ж� 'URX0IE = 1',Ҳ����д�� URX0IE=1;
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
���ڷ����ַ�������
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
__interrupt void recvdInterrupt(void)//���ڽ����жϺ���
{
	URX0IF = 0;//��������Զ���1�� �����жϺ���֮���ֶ���0

	//��ȡ�������ַ���
	unsigned char i = 0;
	temp[count] = U0DBUF;
	count++;
	
	if (temp[count - 2] == 0x0d && temp[count - 1] == 0x0a )	//���յ������ķ������浽�ṹ�嵱�в�����
	{
	  if(count > 2)
	  {
		if (strstr(temp, "+HTTPACTION"))	//�÷�����HTTP��ִ
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
	
	else if (count >= 50)		//����50���ַ���û���յ������ķ���
	{
	  	count = 0;
	}
}