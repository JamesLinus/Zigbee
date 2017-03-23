#ifndef AT_CMD_H
#define AT_CMD_H


#define ATI 		1		//ͬ��������
#define ATE0		13		//�رջ���
#define COPS 		2		//��ѯ�ź�ǿ��
#define CONTYPE 	3		//����GPRS����
#define CONPOINT 	4		//���ý����
#define START 		5		//�򿪳���
#define INITHTTP 	6		//��ʼ��HTTP
#define SETCID		7		//���ó���������
#define SETURL		8		//������ַ
#define REQHTTP		9		//����HTTP����
#define TERMHTTP	10		//��ֹHTTP����
#define STOP		11		//�رճ���
#define FEEDBACK	12		//HTTP��ִ



typedef struct {	
	unsigned char reqCode;	//��¼�÷�����Ӧ������
	unsigned char rcv[50];	//�������ַ���
} ATcmd;

void sendATcmd(unsigned char reqCode);
void InitSim900A(void);
void clearATPacket(void);

#endif
