#ifndef RECV_MSG_H_
#define RECV_MSG_H_

typedef struct recv_msg {
	int stype;
	int ctype;
	unsigned int utag;
	void* body; // JSON str ������message;
}recv_msg;

#endif
