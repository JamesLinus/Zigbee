#ifndef AT_CMD_H
#define AT_CMD_H


#define ATI 		1		//同步波特率
#define ATE0		13		//关闭回显
#define COPS 		2		//查询信号强度
#define CONTYPE 	3		//设置GPRS连接
#define CONPOINT 	4		//设置接入点
#define START 		5		//打开承载
#define INITHTTP 	6		//初始化HTTP
#define SETCID		7		//设置承载上下文
#define SETURL		8		//设置网址
#define REQHTTP		9		//发送HTTP请求
#define TERMHTTP	10		//终止HTTP服务
#define STOP		11		//关闭承载
#define FEEDBACK	12		//HTTP回执



typedef struct {	
	unsigned char reqCode;	//记录该反馈对应的命令
	unsigned char rcv[50];	//反馈的字符串
} ATcmd;

void sendATcmd(unsigned char reqCode);
void InitSim900A(void);
void clearATPacket(void);

#endif
