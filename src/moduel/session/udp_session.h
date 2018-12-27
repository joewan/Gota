#ifndef __UDP_SESSION_H__
#define __UDP_SESSION_H__
#include "session_base.h"

#define MAX_SEND_PKG 2048

extern uv_loop_t* get_uv_loop();

class export_session;
class export_udp_session;
//udpʹ��һ����������������
typedef struct udp_recv_buf {
	unsigned char* recv_buf; //malloc����Ŀռ�
	size_t max_recv_len; //��������С
}udp_recv_buf;

struct udp_session : public session_base {
	
	static void start_udp_server();
	static udp_recv_buf _recv_buf;
public:
	virtual void close();
	virtual void send_data(unsigned char* pkg, int pkg_len);
	virtual void send_msg(recv_msg* msg);
};


#endif