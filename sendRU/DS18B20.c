#include "DS18B20.h"
#include <iocc2530.h>

extern unsigned char temperature[2];

void Delay(unsigned int i)
{
	while(--i);
}

void DQIN(void)//普通IO口三态输入模式,2.6us
{
	P0SEL &= 0xfe;
	P0DIR &= 0xfe;
	P0INP |= 0x01;
}

void DQOUT(void)//普通IO口输出模式
{
	P0SEL &= 0xfe;
	P0DIR |= 0x01;
}


unsigned char init_DS18B20(void)//初始化DS18B20，若初始化失败，返回0x00
{
	unsigned x = 0x00;
	
	DQOUT();
	
	DQ = 1;    //DQ复位
	Delay(44);  //稍做延时,使之稳定
	
	DQ = 0;    //单片机将DQ拉低，表示与DS18B20通信
	Delay(1000); //精确延时 大于 480us,小于960us
	DQ = 1;    //释放总线
	Delay(100);//60-240us
	
	DQIN();
	
	x = DQ;      //稍做延时后 如果x=0则初始化成功 x=1则初始化失败
	Delay(1000);
	if (!x)//成功
		return 0xff;
	else//失败
		return 0x00;
}

void write_byte(unsigned char dat)//向DS18B20写一个字节的数据
{
	unsigned char i = 0;
	DQOUT();
	for (i = 0; i < 8; i++)//先写低位
	{
		DQ = 0;
		Delay(30);
		DQ = dat&0x01;//写入低位
		Delay(40);//延迟15-45us，让DS18B20采样
		DQ = 1;//复位，释放总线
		dat >>= 1;	   //右移一位，此时次低位在最低位，下一个循环中写入
		Delay(100);//延迟65us，写时序之间至少1us
	}
}


unsigned char read_byte(void)//从DS18B20中读一个字节的数据
{
	unsigned char dat = 0x00;
	unsigned int i = 0;
	for (i = 0; i < 8; i++)//读入到dat中
	{
		DQOUT();
		
		DQ = 0;//拉低后15us内主机采样
		
		dat >>= 1;	//1us,   dat右移一位，腾出最高位，待判断写1还是0
		DQ = 1;//释放总线
		
		DQIN();//2.6us
		Delay(30);
		if (DQ)		  	   //如果DQ不为0，则将1写入到dat的最高位中
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
	write_byte(0xcc);//跳过序号列号操作
	write_byte(0x44);//启动温度转换
	

	DQ = 1;//在温度转换时IO口必须提供10ms以上的高电平，使得它有足够的电源实现温度转换
	return 0xff;
}

unsigned char read_temperature(void)
{
	unsigned char a;//低八位
	unsigned char b;//高八位
	unsigned char res = 0;
	
	init_DS18B20();
	write_byte(0xcc);//跳过序号列号操作
	write_byte(0xbe);//读取温度寄存器
	
	a = read_byte();//低八位
	b = read_byte();
	
	temperature[1] = a;
	temperature[0] = b;
	
	a >>= 4;//舍弃小数部分
	b <<= 4;//舍弃符号位
	b &= 0xf0;
	res = a + b;
	return res;
}