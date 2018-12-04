#ifndef __TCP_SESSION_H__
#define __TCP_SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../3rd/mjson/json.h"

#ifdef __cplusplus
}
#endif

#define MAX_SEND_PKG 2048

struct session {
	char c_ip[32];
	int c_port;
#ifdef USE_LIBUV
	void* c_sock;
#else
	int c_sock;
#endif
	
	int removed;
	//websocket�Ƿ����ֳɹ�
	int is_shake_hand;
	//��������ǰ���sessionЭ������
	int socket_type;
	unsigned int uid;
	int is_server_session;
	struct session* next;
	unsigned char send_buf[MAX_SEND_PKG];
};

void init_session_manager(int socket_type, int protocal_type);
void exit_session_manager();


// �пͷ��˽������������sesssion;
struct session* save_session(void* c_sock, char* ip, int port);
extern void close_session(struct session* s);

// ��������session�������������session
void foreach_online_session(int(*callback)(struct session* s, void* p), void*p);

// �������ǵ�����
void clear_offline_session();
// end 

//�������ݽӿ�
//Ӧ�ò�ֻ�ô������ݺͳ��ȼ��ɣ��ײ㸺������Э����
extern void session_send(struct session* s, unsigned char* body, int len);

//����json����
extern void session_json_send(struct session* s, json_t* object);

extern int get_proto_type();

#endif

