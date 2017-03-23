#include "DS18B20.h"
#include <iocc2530.h>

extern unsigned char temperature[2];

void Delay(unsigned int i)
{
	while(--i);
}

void DQIN(void)//��ͨIO����̬����ģʽ,2.6us
{
	P0SEL &= 0xfe;
	P0DIR &= 0xfe;
	P0INP |= 0x01;
}

void DQOUT(void)//��ͨIO�����ģʽ
{
	P0SEL &= 0xfe;
	P0DIR |= 0x01;
}


unsigned char init_DS18B20(void)//��ʼ��DS18B20������ʼ��ʧ�ܣ�����0x00
{
	unsigned x = 0x00;
	
	DQOUT();
	
	DQ = 1;    //DQ��λ
	Delay(44);  //������ʱ,ʹ֮�ȶ�
	
	DQ = 0;    //��Ƭ����DQ���ͣ���ʾ��DS18B20ͨ��
	Delay(1000); //��ȷ��ʱ ���� 480us,С��960us
	DQ = 1;    //�ͷ�����
	Delay(100);//60-240us
	
	DQIN();
	
	x = DQ;      //������ʱ�� ���x=0���ʼ���ɹ� x=1���ʼ��ʧ��
	Delay(1000);
	if (!x)//�ɹ�
		return 0xff;
	else//ʧ��
		return 0x00;
}

void write_byte(unsigned char dat)//��DS18B20дһ���ֽڵ�����
{
	unsigned char i = 0;
	DQOUT();
	for (i = 0; i < 8; i++)//��д��λ
	{
		DQ = 0;
		Delay(30);
		DQ = dat&0x01;//д���λ
		Delay(40);//�ӳ�15-45us����DS18B20����
		DQ = 1;//��λ���ͷ�����
		dat >>= 1;	   //����һλ����ʱ�ε�λ�����λ����һ��ѭ����д��
		Delay(100);//�ӳ�65us��дʱ��֮������1us
	}
}


unsigned char read_byte(void)//��DS18B20�ж�һ���ֽڵ�����
{
	unsigned char dat = 0x00;
	unsigned int i = 0;
	for (i = 0; i < 8; i++)//���뵽dat��
	{
		DQOUT();
		
		DQ = 0;//���ͺ�15us����������
		
		dat >>= 1;	//1us,   dat����һλ���ڳ����λ�����ж�д1����0
		DQ = 1;//�ͷ�����
		
		DQIN();//2.6us
		Delay(30);
		if (DQ)		  	   //���DQ��Ϊ0����1д�뵽dat�����λ��
			dat |= 0x80;
		else
			dat &= 0x7f;
		Delay(100);	
	}
	return dat;
}

unsigned char get_temperature(void)
{
	if (!init_DS18B20())
		return ERROR;
	write_byte(0xcc);//��������кŲ���
	write_byte(0x44);//�����¶�ת��
	

	DQ = 1;//���¶�ת��ʱIO�ڱ����ṩ10ms���ϵĸߵ�ƽ��ʹ�������㹻�ĵ�Դʵ���¶�ת��
	return 0xff;
}

unsigned char read_temperature(void)
{
	unsigned char a;//�Ͱ�λ
	unsigned char b;//�߰�λ
	unsigned char res = 0;
	
	init_DS18B20();
	write_byte(0xcc);//��������кŲ���
	write_byte(0xbe);//��ȡ�¶ȼĴ���
	
	a = read_byte();//�Ͱ�λ
	b = read_byte();
	
	temperature[1] = a;
	temperature[0] = b;
	
	a >>= 4;//����С������
	b <<= 4;//��������λ
	b &= 0xf0;
	res = a + b;
	return res;
}