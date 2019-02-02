#ifndef RECV_MSG_H_
#define RECV_MSG_H_

#define BIN_HEAD_LEN (sizeof(msg_head))

//2�ֽڱ�ʶ����һ�������İ�����
#define SIZE_HEAD 2
//������json����pb��ʽͨ��ͷ
typedef struct msg_head {
	//service���
	int stype;
	//Э��cmdid
	int ctype;
	//�û�uid
	unsigned int utag;
}msg_head;

typedef struct recv_msg {
	
	msg_head head;
	/*
	JSON�ַ���������pb�����Ƹ�ʽ;
	*/
	void* body; 
}recv_msg;

//�������ط��������ͷ��������bodyֱ��ת��������service
typedef struct raw_cmd {
	msg_head head;
	//ָ��ԭʼ����ͷ(������ͷ)
	unsigned char* raw_data;
	int raw_len;
}raw_cmd;
#endif
