#ifndef NETIO_H__
#define NETIO_H__

#include "uv.h"

struct session;
struct session_base;
//�ײ㴫��Э��
enum {
	TCP_SOCKET_IO = 0,  // tcp
	WEB_SOCKET_IO = 1,  // websocket
};

//Ӧ�ò�Э��
enum {
	BIN_PROTOCAL = 0, // ������Э�� pb
	JSON_PROTOCAL = 1, // jsonЭ��
};


const char* conver_socket_type_str(int socket_type);
const char* conver_protocal_str(int protocal_type);

void start_server(char* ip,int port);
void start_server_ws(char* ip, int port);
void uv_send_data(void* stream, char* pkg, unsigned int pkg_len);

void init_uv();
extern uv_loop_t* get_uv_loop();

extern struct session* netbus_connect(char* server_ip, int port);
extern void tcp_connect(const char* server_ip,int port,void(*connect_cb)(const char* err,session_base* s,void* udata),void* udata);
void run();
#endif

