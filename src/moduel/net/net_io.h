#ifndef NETIO_H__
#define NETIO_H__

//�ײ㴫��Э��
enum {
	TCP_SOCKET_IO = 0,  // tcp
	WEB_SOCKET_IO = 1,  // websocket
};

//Ӧ�ò�Э��
enum {
	BIN_PROTOCAL = 0, // ������Э��
	JSON_PROTOCAL = 1, // jsonЭ��
};

const char* conver_socket_type_str(int socket_type);
const char* conver_protocal_str(int protocal_type);

void start_server(char* ip,int port ,int socket_type ,int proto_type);

void uv_send_data(void* stream, char* pkg, unsigned int pkg_len);
#endif

