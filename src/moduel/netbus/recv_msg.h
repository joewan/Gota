#ifndef RECV_MSG_H_
#define RECV_MSG_H_

#define BIN_HEAD_LEN 8 
//2�ֽڱ�ʶ����һ�������İ�����
#define SIZE_HEAD 2
typedef struct recv_msg {
	int stype;
	int ctype;
	unsigned int utag;
	void* body; // JSON str ������message;
}recv_msg;

#endif
