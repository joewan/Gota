#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif

#include "tcp_session.h"
#include "../net/net_io.h"
#include "../../3rd/mjson/json_extends.h"
#include "../netbus/netbus.h"

#define MAX_SESSION_NUM 6000
#define my_malloc malloc
#define my_free free

#define MAX_RECV_BUFFER 8096

extern void on_connect_lost(struct session* s);

struct {
	struct session* online_session;

	struct session* cache_mem;
	struct session* free_list;


	//char recv_buffer[MAX_RECV_BUFFER];
	int readed; // ��ǰ�Ѿ���socket�����ȡ������;

	int has_removed;
	int socket_type;  //0 tcp_socket 1��ʾwebsocket
	int protocal_type;// 0 ��ʾ������Э�飬size + ���ݵ�ģʽ
	               // 1,��ʾ�ı�Э�飬�Իس��������ֽ��յ�������Ϊһ����
}session_manager;

static struct session* cache_alloc() {
	struct session* s = NULL;
	if (session_manager.free_list != NULL) {
		s = session_manager.free_list;
		session_manager.free_list = s->next;
	}
	else { // ����ϵͳ�ĺ��� malloc
		s = my_malloc(sizeof(struct session));
	}
	memset(s, 0, sizeof(struct session));

	return s;
}

static void cache_free(struct session* s) {
	// �ж�һ�£��Ǵ�cache�����ȥ�ģ����Ǵ�ϵͳmy_malloc�����ȥ�ģ�
	if (s >= session_manager.cache_mem && s < session_manager.cache_mem + MAX_SESSION_NUM) {
		s->next = session_manager.free_list;
		session_manager.free_list = s;
	}
	else { 
		my_free(s);
	}
	
}

int get_socket_type() {
	return session_manager.socket_type;
}

int get_proto_type() {
	return session_manager.protocal_type;
}

void init_session_manager(int socket_type,int protocal_type) {
	memset(&session_manager, 0, sizeof(session_manager));
	session_manager.socket_type = socket_type;
	session_manager.protocal_type = protocal_type; //���ο��ʹ�õ�Э������
	// ��6000��sessionһ�η��������
	session_manager.cache_mem = (struct session*)my_malloc(MAX_SESSION_NUM * sizeof(struct session));
	memset(session_manager.cache_mem, 0, MAX_SESSION_NUM * sizeof(struct session));
	// end 

	for (int i = 0; i < MAX_SESSION_NUM; i++) {
		session_manager.cache_mem[i].next = session_manager.free_list;
		session_manager.free_list = &session_manager.cache_mem[i];
	}

}

void exit_session_manager() {

}

struct session* save_session(void* c_sock, char* ip, int port) {
	struct session* s = cache_alloc();
#ifdef USE_LIBUV
	s->c_sock = c_sock;
#else
	s->c_sock = (int)c_sock;
#endif
	
	s->c_sock = c_sock;
	s->c_port = port;
	int len = strlen(ip);
	if (len >= 32) {
		len = 31;
	}
	strncpy(s->c_ip, ip, len);
	s->c_ip[len] = 0;

	s->next = session_manager.online_session;
	session_manager.online_session = s;
	return s;
}

void foreach_online_session(int(*callback)(struct session* s, void* p), void*p) {
	if (callback == NULL) {
		return;
	}

	struct session* walk = session_manager.online_session;
	while (walk) {
		if (walk->removed == 1) {
			walk = walk->next;
			continue;
		}
		if (callback(walk, p)) {
			return;
		}
		walk = walk->next;
	}
}

void close_session(struct session* s) {
	s->removed = 1;
	session_manager.has_removed = 1;
	printf("client %s:%d exit\n", s->c_ip, s->c_port);
}

void clear_offline_session() {
	if (session_manager.has_removed == 0) {
		return;
	}

	struct session** walk = &session_manager.online_session;
	while (*walk) {
		struct session* s = (*walk);
		if (s->removed) {
			*walk = s->next;
			s->next = NULL;
			//֪ͨ�ϲ�session�ر�
			on_connect_lost(s);
			/*if (s->c_sock != 0) {
				closesocket(s->c_sock);
			}*/

			s->c_sock = 0;
			// �ͷ�session
			cache_free(s);
		}
		else {
			walk = &(*walk)->next;
		}
	}
	session_manager.has_removed = 0;
}

static int send_tcp_data(struct session* s, unsigned char* data, int len) {
	//int long_pkg = 0;
	unsigned char* pkg_ptr = NULL;

	if (len + 2 > MAX_SEND_PKG) {
		pkg_ptr = my_malloc(len + 2); //2�ֽ�ͷ������
		//long_pkg = 1;
	}
	else {
		//pkg_ptr = s->send_buf;
		pkg_ptr = my_malloc(MAX_SEND_PKG);
	}
	int ssize = 0;
	if (session_manager.protocal_type == JSON_PROTOCAL) {
		memcpy(pkg_ptr, data, len);
		strncpy(pkg_ptr + len, "\r\n", 2);
		//ssize = send(s->c_sock, pkg_ptr, len + 2, 0);
		uv_send_data(s->c_sock, pkg_ptr, len + 2);
	}
	else if (session_manager.protocal_type == BIN_PROTOCAL) {
		memcpy(pkg_ptr + 2, data, len);
		pkg_ptr[0] = ((len + 2)) & 0x000000ff;
		pkg_ptr[1] = (((len + 2)) & 0x0000ff00) >> 8;
		//ssize = send(s->c_sock, pkg_ptr, len + 2, 0);
		uv_send_data(s->c_sock, pkg_ptr, len + 2);
	}
	if (pkg_ptr != NULL) {
		my_free(pkg_ptr);
	}

	return 0;
}

//����websocket����
static int send_websocket_data(struct session* s, unsigned char* data, int len) {
	
	//int long_pkg = 0;
	unsigned char* pkg_ptr = NULL;

	int head_len = 1; //�̶�һ�ֽ�
	if (len <= 125) {
		++head_len;
	}
	else if (len <= 0xffff) {
		head_len += 3; //126 + ����ռ2���ֽ�
	}
	else {
		head_len += 9; //127+8�ֽڳ���s
	}

	if (len + head_len > MAX_SEND_PKG) {
		//��Ҫ��̬�����ڴ�
		pkg_ptr = my_malloc(len + head_len);
		//long_pkg = 1;
	}
	else {
		//pkg_ptr = s->send_buf;
		pkg_ptr = my_malloc(MAX_SEND_PKG);
	}

	//���͸�ʽ �̶�1�ֽ�0x81 + ���ݳ���(�䳤) + ����
	//����д���λ��
	unsigned int send_len = 0;
	pkg_ptr[0] = 0x81;
	if (len <= 125) {
		pkg_ptr[1] = len;
		send_len = 2;
	}
	else if (len <= 0xffff) {
		pkg_ptr[1] = 126;
		pkg_ptr[2] = (len & 0x000000ff);
		pkg_ptr[3] = ((len & 0x0000ff00) >> 8);
		send_len = 4;
	}
	else {
		pkg_ptr[1] = 127;
		pkg_ptr[2] = (len & 0x000000ff);
		pkg_ptr[3] = ((len & 0x0000ff00) >> 8);
		pkg_ptr[4] = ((len & 0x00ff0000) >> 16);
		pkg_ptr[5] = ((len & 0xff000000) >> 24);
		//��ʹ�ã���������ô���İ���ǰ32���ֽ��㹻��ʾ��
		pkg_ptr[6] = 0;
		pkg_ptr[7] = 0;
		pkg_ptr[8] = 0;
		pkg_ptr[9] = 0;
		send_len = 10;

	}
	//��������,������
# ifdef _DEBUG
	printf("�������ݸ�web:%s\n", data);
#endif

	//copy���ݵ�������
	memcpy(pkg_ptr + send_len, data, len);
	send_len += len; //�������ݰ�����
	//int send_bytes = send(s->c_sock, pkg_ptr, send_len, 0);
	uv_send_data(s->c_sock, pkg_ptr, send_len);
	if (pkg_ptr!=NULL){
		my_free(pkg_ptr);
		pkg_ptr = NULL;
	}

	return 0;
}

void session_send(struct session* s, unsigned char* body, int len) {
	if (s == NULL || body == NULL || len <= 0) {
		return;
	}
	int send_bytes = 0;
	if (TCP_SOCKET_IO == s->socket_type) {
		send_bytes = send_tcp_data(s, body, len);
	}
	else if (WEB_SOCKET_IO == s->socket_type) {
		send_bytes = send_websocket_data(s, body, len);
	}
#ifdef _DEBUG
	printf("send_bytes%d\n", send_bytes);
#endif
}

void session_json_send(struct session* s, json_t* json_object) {
	if (NULL == s || NULL == json_object) {
		printf("session_json_send s is null\n");
		return;
	}
	char* json_str = NULL;
	json_tree_to_string(json_object,&json_str);
	session_send(s, json_str,strlen(json_str));
	json_free_str(json_str);
}