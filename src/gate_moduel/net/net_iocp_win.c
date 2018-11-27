#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "net_io.h"
#include <WinSock2.h>
#include <mswsock.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib,"Mswsock.lib") 


#include "../session/tcp_session.h"
#include "../../3rd/http_parser/http_parser.h"
#include "../../3rd/crypt/sha1.h"
#include "../../3rd/crypt/base64_encoder.h"
#include "../../3rd/mjson/json.h"

#include "../services/service_gateway.h"
#include "../../utils/log.h"
//#include "../service/types_service.h"
//#include "../service/table_service.h"

//#include "../server.h"
#define my_malloc malloc
#define my_free free
#define my_realloc realloc

char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade:websocket\r\n" \
"Connection: Upgrade\r\n" \
"Sec-WebSocket-Accept: %s\r\n" \
"WebSocket-Location: ws://%s:%d/chat\r\n" \
"WebSocket-Protocol:chat\r\n\r\n";

extern void init_server_gateway();
extern void exit_server_gateway();
extern void on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len);
extern void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);
static HANDLE g_iocp = 0;
enum {
	IOCP_ACCPET = 0,
	IOCP_RECV,
	IOCP_WRITE,
};

//�洢httpÿ�ν�����ͷ��value
static char header_key[64];
//�Ƿ��������websocket��Sec-WebSocket-Key�ֶ�
static int has_client_key = 0; 

#define MAX_RECV_SIZE 2047
#define MAX_PKG_SIZE ((1<<16) - 1)
struct io_package {
	WSAOVERLAPPED overlapped;
	int opt; // ���һ�����ǵ�ǰ�����������;
	int accpet_sock; //������socket
	WSABUF wsabuffer;
	int recved; // �յ����ֽ���;
	unsigned char* long_pkg;  //�󻺴�������С������������ʹ�����ָ��
	int max_pkg_len; //��ǰ��������С
	unsigned char pkg[MAX_RECV_SIZE]; //С��������
};

static void
post_accept(SOCKET l_sock, HANDLE iocp) {
	struct io_package* pkg = malloc(sizeof(struct io_package));
	memset(pkg, 0, sizeof(struct io_package));

	pkg->wsabuffer.buf = pkg->pkg;
	pkg->wsabuffer.len = MAX_RECV_SIZE - 1;
	pkg->opt = IOCP_ACCPET;
	pkg->max_pkg_len = MAX_RECV_SIZE - 1;

	DWORD dwBytes = 0;
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	int addr_size = (sizeof(struct sockaddr_in) + 16);
	pkg->accpet_sock = client;

	AcceptEx(l_sock, client, pkg->wsabuffer.buf, 0/*pkg->wsabuffer.len - addr_size* 2*/,
		addr_size, addr_size, &dwBytes, &pkg->overlapped);
}

static void post_recv(SOCKET client_fd, HANDLE iocp) {
	// �첽��������;
	// ʲô���첽? recv 8K���ݣ��������ʱ��û�����ݣ�
	// ��ͨ��ͬ��(����)�̹߳��𣬵ȴ����ݵĵ���;
	// �첽�������û�����ݷ�����Ҳ�᷵�ؼ���ִ��;
	struct io_package* io_data = malloc(sizeof(struct io_package));
	// ��0����ҪĿ����Ϊ������overlapped��0;
	memset(io_data, 0, sizeof(struct io_package));

	io_data->opt = IOCP_RECV;
	io_data->wsabuffer.buf = io_data->pkg;
	io_data->wsabuffer.len = MAX_RECV_SIZE - 1;
	io_data->max_pkg_len = MAX_RECV_SIZE - 1;
	// ������recv������;
	// 
	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	int ret = WSARecv(client_fd, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);
	if (0 != ret) {
		;
		//printf("WSARecv error:%d", GetLastError());
	}
}

// ������ͷ�Ļص�����
static char header_key[64];
static char client_ws_key[128];

static int on_header_field(http_parser* p, const char *at,
	size_t length) {
	size_t len = (length < 63) ? length : 64;
	strncpy(header_key, at, len);
	header_key[len] = 0;
	
	return 0;
}

static int on_header_value(http_parser* p, const char *at,
	size_t length) {
	if (strcmp(header_key, "Sec-WebSocket-Key") != 0) {
		return 0;
	}
	strncpy(client_ws_key, at, length);
	client_ws_key[length] = 0;
	has_client_key = 1;
	printf("%s\n", client_ws_key);
	return 0;
}

static int recv_header(unsigned char* pkg, int len, int* pkg_size) {
	if (len <= 1) { // �յ������ݲ��ܹ������ǵİ��Ĵ�С��������
		return -1;
	}

	*pkg_size = (pkg[0]) | (pkg[1] << 8);
	return 0;
}

static void on_bin_protocal_recved(struct session* s, struct io_package* io_data) {
	// Step1: �������ݵ�ͷ����ȡ������Ϸ��Э�����Ĵ�С;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		if (recv_header(io_data->pkg, io_data->recved, &pkg_size) != 0) { // ����Ͷ��recv����֪���ܷ����һ������ͷ;
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
			io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}

		// Step2:�ж����ݴ�С���Ƿ񲻷��Ϲ涨�ĸ�ʽ
		if (pkg_size >= MAX_PKG_SIZE) { // ,�쳣�����ݰ���ֱ�ӹرյ�socket;
			close_session(s);
			my_free(io_data); // �ͷ����socketʹ�õ���ɶ˿ڵ�io_data;
			break;
		}

		// �Ƿ�������һ�����ݰ�;
		if (io_data->recved >= pkg_size) { // ��ʾ�����Ѿ��յ����ٳ�����һ���������ݣ�
			unsigned char* pkg_data = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;

			printf("%s", pkg_data + 4);
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			if (io_data->recved > pkg_size) { // 1.5 ����
				memmove(io_data->pkg, io_data->pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}

			if (io_data->recved == 0) { // ����Ͷ������
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
				io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		}
		else { // û������һ�����ݰ�����������ֱ��Ͷ��recv����;
			unsigned char* recv_buffer = io_data->pkg;
			if (pkg_size > MAX_RECV_SIZE) {
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(pkg_size + 1);
					memcpy(io_data->long_pkg, io_data->pkg, io_data->recved);
				}
				recv_buffer = io_data->long_pkg;
			}

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			io_data->wsabuffer.buf = recv_buffer + io_data->recved;
			io_data->wsabuffer.len = pkg_size - io_data->recved;

			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
		// end 
	}
}

int read_json_tail(unsigned char* pkg_data, int recvlen, int* pkg_size) {
	//����\r\n,ֱ�ӷ��ش���
	if (recvlen < 2) {
		return -1;
	}
#ifdef _DEBUG
	
	printf("read_json_tail:%s",pkg_data);
#endif // !_DEBUG

	*pkg_size = 0;
	int i = 0;
	const int len = recvlen - 1;
	while (i < len) {
		if (pkg_data[i] == '\r' && pkg_data[i + 1] == '\n') {
			*pkg_size = (i + 2); //+2��ʾҪ����\r\n
			return 0;
		}
		i++;
	}

	return -1;
}

static void on_json_protocal_recved(struct session* s, struct io_package* io_data) {
	if (s == NULL || io_data == NULL) {
		return;
	}
	//io_data->recved��ǰ���������ݴ�С
	while (io_data->recved) {
		//��ǰһ��json����С��һ��������json����\r\n�ָ�
		int pkg_size = 0;
		//��ȡ������ָ��
		unsigned char* pkg_data = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
		if (pkg_data == NULL) {
			printf("get io_data buffer error\n");
			return;
		}

		//�ָ�һ�������İ�
		if (read_json_tail(pkg_data, io_data->recved, &pkg_size) != 0) {
			//û���ҵ�\r\n,���һ������쳣���ر�session����
			if (io_data->recved > MAX_PKG_SIZE) {
				close_session(s);
				my_free(io_data);
				break;
			}

			//�жϵ�ǰ�ռ��Ƿ���Ҫ����
			if (io_data->recved >= io_data->max_pkg_len) {
				//���յ�ǰ��������2������
				int new_alloc_len = io_data->recved * 2;
				new_alloc_len = (new_alloc_len > MAX_PKG_SIZE) ? MAX_PKG_SIZE : new_alloc_len;
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(new_alloc_len + 1);
					memcpy_s(io_data->long_pkg, new_alloc_len, io_data->pkg, io_data->recved);
				}
				else {
					io_data->long_pkg = my_realloc(io_data->long_pkg, new_alloc_len + 1);
				}

				io_data->max_pkg_len = new_alloc_len;
			}
			//��Ͷ��һ������
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			unsigned char* buf = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;
			io_data->wsabuffer.buf = buf + io_data->recved;
			io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}

		//�ߵ������ʾ������һ�����������ݰ�
		//�����ϲ㴦����
		on_json_protocal_recv_entry(s, pkg_data, pkg_size);
		//�������������������ǰ��pkg_size , 
		//���io_data->recved == pkg_size��ʾ����û�����ݣ�������memmove��
		if (io_data->recved > pkg_size) {
			memmove(pkg_data, pkg_data + pkg_size, io_data->recved - pkg_size);
		}
		io_data->recved -= pkg_size;
		//������û������
		if (0 >= io_data->recved) {
			io_data->recved = 0;
			//�����ڴ���գ���ʹ��io_data���С����ռ�
			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}
			//��Ͷ��һ��������
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->max_pkg_len = MAX_RECV_SIZE;
			io_data->wsabuffer.buf = io_data->pkg;
			io_data->wsabuffer.len = io_data->max_pkg_len;

			int r = WSARecv(s->c_sock, &(io_data->wsabuffer), 1, &dwRecv, &dwFlags, &(io_data->overlapped), NULL);
			break;
		}
	}
}

//��ȡһ��������֡
//ws_sizeһ������ws������
//head_len������
static int process_websocket_pack(unsigned char* pkg,int pkg_len,int* head_len,int* ws_size) {
	unsigned char* mask = NULL;
	unsigned char* rawdata = NULL;
	int datalen = 0;
	unsigned char chlen = pkg[1];
	chlen = chlen & 0x7f; //ȥ�����λ��1 0x7f = 0111ffff
	if (chlen <= 125) {
		//chlen�������ݳ���
		if (pkg_len < 2 + 4) {
			return -1;
		}
		datalen = chlen;
		mask = (unsigned char*)&(pkg[2]);
	}
	else if (chlen == 126) {
		//7+16
		datalen = pkg[2] + (pkg[3] << 8); //�����data[3]�൱�ڶ����Ƶ�ʮλ
		if (pkg_len < 4 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[4]);
	}
	else if (chlen == 127) {
		//����8���ֽڱ�ʾ����һ���޿������ϣ�����ǰ��32λ�Ϳ�����'7+64
		datalen = pkg[2] + (pkg[3] << 8) + (pkg[4] << 16) + (pkg[5] << 24);
		if (pkg_len < 2 + 8 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[6]);
	}
	//������ʼ��ַ
	rawdata = (unsigned char*)(mask + 4);
	*head_len = (int)(rawdata - pkg);
	*ws_size = *head_len + datalen;
	return 0;
}

static int parser_websocket_pack(struct session* s,unsigned char* body,int len, unsigned char* mask, int protocal_type) {
	if (s == NULL || body == NULL || mask == NULL) {
		printf("parser_websocket_pack parament error\n");
		return -1;
	}
	//ʹ��mask����body,���벻��ı����ݳ���
	for (int i = 0; i < len; ++i) {
		int j = i % 4;
		body[i] = body[i] ^ mask[j];
	}

#if _DEBUG
	char buf[1024];
	memcpy_s(buf,sizeof(buf), body, len);
	buf[len] = 0;
	printf("websocket data:  %s",buf);
#endif

	if (BIN_PROTOCAL == protocal_type) {
		on_bin_protocal_recv_entry(s, body, len);
	}
	else if (JSON_PROTOCAL == protocal_type) {
		on_json_protocal_recv_entry(s, body, len);
	}

	return 0;
}

//����websocketЭ�� 
static int process_websocket_data(struct session* s, struct io_package* io_data,int protocal_type) {
	if (s == NULL || io_data == NULL) {
		printf("process_websocket_data parament error\n");
		return -1; 
	}

	//ѭ�����������������
	//������һ��websocket������֡�����ݸ��ϲ�Ӧ�ã�Ȼ�����Ͷ�ݶ�����
	//û�н�����������websocket֡��������Ͷ�ݶ�����
	//û�н�����������websocket֡��,���һ���������˵������ʽ�д��󣬶Ͽ�����
	unsigned char* pkg = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
	while (io_data->recved >0) {
		int pkg_size = 0;
		int header_size = 0;
		//if (0x81 == pkg[0] || 0x82 == pkg[0]) {
			//��ȡһ��������֡�����ز�����0˵��û�ж�ȡ��һ��������֡ 
			if (process_websocket_pack(pkg, io_data->recved,&header_size,&pkg_size) != 0) {
				//����Ͷ��
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = pkg + io_data->recved;
				io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}

			//������һ��������֡���ж�֡�Ϸ���
			if (pkg_size >= MAX_PKG_SIZE) {
				close_session(s);
				if (io_data->long_pkg) {
					my_free(io_data->long_pkg);
				}
				
				my_free(io_data);
				io_data = NULL;
				break;
			}
			//��������������һ��������֡
			if (pkg_size <= io_data->recved) {
#if _DEBUG
				printf("websocket pack: header_size:%d body_size:%d\n", header_size, pkg_size);
#endif
				//�����0x88 websocket�ͻ�������ر�
				if (0x88 == pkg[1]) {
					if (NULL != io_data->long_pkg) {
						my_free(io_data->long_pkg);
						my_free(io_data);
						io_data = NULL;
					}
					close_session(s);
					break;
				}
				//�������ݰ�
				parser_websocket_pack(s, pkg + header_size, pkg_size- header_size, pkg+ header_size-4, protocal_type);
				
				if (io_data->recved > pkg_size) {
					memmove(pkg, pkg + pkg_size, io_data->recved - pkg_size);
				}
				//������ݿ�����С�������洢����copy��С������
				io_data->recved -= pkg_size;
				if (io_data->recved < MAX_RECV_SIZE && io_data->long_pkg != NULL) {
					memcpy(io_data->pkg, io_data->long_pkg, io_data->recved);
					my_free(io_data->long_pkg);
					io_data->long_pkg = NULL;
				}

				if (0 == io_data->recved) {
					//������û��������Ͷ�ݶ�
					DWORD dwRecv = 0;
					DWORD dwFlags = 0;
					io_data->wsabuffer.buf = io_data->pkg;
					io_data->wsabuffer.len = MAX_RECV_SIZE;
					int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
						1, &dwRecv, &dwFlags,
						&(io_data->overlapped), NULL);
					break;
				}
			}
			else {
				//����˵��һ��֡ͷ�����ˣ�����û��������֡����Ҫ����������
				if (pkg_size > MAX_RECV_SIZE) {
					//����С����������Ҫ��������ռ�
					if (io_data->long_pkg == NULL) {
						io_data->long_pkg = my_malloc(pkg_size + 1);
						memcpy_s(io_data->long_pkg, pkg_size + 1, pkg, io_data->recved);
					}
					pkg = io_data->long_pkg;
					io_data->max_pkg_len = pkg_size + 1;
				} //if

				//��Ͷ�ݶ�����
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = pkg + io_data->recved;
				io_data->wsabuffer.len = pkg_size - io_data->recved;
				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		//}
	}

	return 0;
}

//����websocket����Э��
static int process_websocket_connect(struct session* s, struct io_package* io_data, char* ip,int port) {
	if (s==NULL || io_data==NULL) {
		printf("process_websocket_connect parament error\n");
		return -1;
	}

	struct http_parser p;
	http_parser_init(&p, HTTP_REQUEST);

	struct http_parser_settings setting;
	http_parser_settings_init(&setting);

	//on_header_fieldÿ������һ��httpͷ��field�ֶα�����
	//on_header_valueÿ�ν�����ͷ���ֶα�����
	setting.on_header_field = on_header_field;
	setting.on_header_value = on_header_value;
	unsigned char* pkg = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
	
	/*
	������߼���:
	�ж�has_client_key==0˵����û�ж�ȡ��ͷ����Sec-WebSocket-Key�ֶΣ���Ҫ��
	��Ͷ�ݶ����󣬵�on_header_value��ȡ��Sec-WebSocket-Key�ֶκ�has_client_key==1
	˵��Sec-WebSocket-Key�Ѿ��洢��client_ws_key
	*/
	has_client_key = 0;
	http_parser_execute(&p, &setting, pkg, io_data->recved);
	if (0 == has_client_key) {
		s->is_shake_hand = 0;
		//�����������ְ�;
		if (io_data->recved >= MAX_RECV_SIZE) { 
			close_session(s);
			my_free(io_data);
			return -1;
		}

		//�ڴ�Ͷ��һ��������
		DWORD dwRecv = 0;
		DWORD dwFlags = 0;

		if (io_data->long_pkg != NULL) {
			my_free(io_data->long_pkg);
			io_data->long_pkg = NULL;
		}

		io_data->max_pkg_len = MAX_RECV_SIZE;
		io_data->wsabuffer.buf = &(io_data->pkg[io_data->recved]);
		io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

		int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),1, &dwRecv, &dwFlags, &(io_data->overlapped),NULL);
		return -1;
	}

	int sha1_len = 0;
	int base64_len = 0;
	static char bufferreq[256] = {0};
	const char* migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	sprintf(bufferreq, "%s%s", client_ws_key, migic);

	char* sha1_str = crypt_sha1((uint8_t*)bufferreq, strlen(bufferreq), &sha1_len);
	char* bs_str = base64_encode((uint8_t*)sha1_str, sha1_len, &base64_len);

	char buffer[1024] = {0};
	strncpy(buffer, bs_str, base64_len);
	buffer[base64_len] = '\0';

	char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
		"Upgrade:websocket\r\n" \
		"Connection: Upgrade\r\n" \
		"Sec-WebSocket-Accept: %s\r\n" \
		"WebSocket-Location: ws://%s:%d/chat\r\n" \
		"WebSocket-Protocol:chat\r\n\r\n";

	static char accept_buffer[256];
	sprintf(accept_buffer, wb_accept, buffer, ip, port);

	send(s->c_sock, accept_buffer, strlen(accept_buffer), 0);
	s->is_shake_hand = 1;

	//���ӳɹ���Ͷ��һ������
	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	if (io_data->long_pkg != NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;

	}
	//���ӳɹ���������0
	io_data->recved = 0;
	io_data->max_pkg_len = MAX_RECV_SIZE;
	io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
	io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

	int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);

	return 0;
}

struct session* gateway_connect(char* server_ip, int port,int stype) {
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return NULL;
	}
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr(server_ip);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);

	int ret = connect(sock, ((struct sockaddr*) &sockaddr), sizeof(sockaddr));
	if (ret != 0) {
		closesocket(sock);
		return NULL;
	}

	struct session* s = save_session(sock, inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));
	//���Ӻ�˷���ʹ��tcp+jsonЭ��
	s->socket_type = TCP_SOCKET_IO;
	CreateIoCompletionPort((HANDLE)sock, g_iocp, (DWORD)s, 0);
	post_recv((SOCKET)sock, g_iocp);

	return s;
}

void start_server(char* ip, int port, int socket_type, int protocal_type) {

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	// �½�һ����ɶ˿�;
	SOCKET l_sock = INVALID_SOCKET;
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == NULL) {
		goto failed;
	}
	g_iocp = iocp;
	// ����һ���߳�
	// CreateThread(NULL, 0, ServerWorkThread, (LPVOID)iocp, 0, 0);
	// end

	// �����ͻ��˼���socket
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (l_sock == INVALID_SOCKET) {
		goto failed;
	}
	// bind socket
	struct sockaddr_in s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_addr.s_addr = inet_addr(ip);
	s_address.sin_port = htons(port);

	if (bind(l_sock, (struct sockaddr *) &s_address, sizeof(s_address)) != 0) {
		goto failed;
	}

	if (listen(l_sock, SOMAXCONN) != 0) {
		goto failed;
	}

	// start 
	CreateIoCompletionPort((HANDLE)l_sock, iocp, (DWORD)0, 0);
	post_accept(l_sock, iocp);
	
	// end 

	DWORD dwTrans;
	struct session* s;
	//  ������������¼��������Ժ�,
	// GetQueuedCompletionStatus �᷵�����������
	// ʱ���WSAOVERLAPPED �ĵ�ַ,���������ַ���ҵ�
	// io_data, �ҵ���io_data,Ҳ����ζ�������ҵ���,
	// ���Ļ�������
	struct io_package* io_data;
	
	const char* socket_type_str = conver_socket_type_str(socket_type);
	const char* protocal_str = conver_protocal_str(protocal_type);
	printf("server start [ip:%s, port:%d, socket_type:%s, protocal_type:%s]\n",
		ip, port, socket_type_str, protocal_str);
	while (1) {
		clear_offline_session();
		int m_sec = -1;
		if (NULL != GATEWAY_TIMER_LIST) {
			m_sec = update_timer_list(GATEWAY_TIMER_LIST);
		}
		
		// ������������IOCP��������߳��������Ѿ������¼�
		// ��ʱ�򣬲Ż������̻߳���;
		// IOCP ��select��һ�����ȴ�����һ����ɵ��¼�;
		s = NULL;
		dwTrans = 0;
		io_data = NULL;
		//GetQueuedCompletioStatus����
		//iocp��ɶ˿�
		//dwTrans����ʵ�ʶ�ȡ�����ֽ��� 
		//s��CreateIoCompletionPort���ù����ĵ���������
		//io_data�������WSARecv���õ�WSAOVERLAPPED����
		//WSAOVERLAPPEDһ�������Զ���ṹ���һ���ֶΣ������Ϳ��Ժ��Զ���ṹ�����
		int timeout_sec = m_sec > 0 ? m_sec : WSA_INFINITE;
		int ret = GetQueuedCompletionStatus(iocp, &dwTrans, (LPDWORD)&s, (LPOVERLAPPED*)&io_data, timeout_sec);
		if (ret == 0) {
			
			//if (WAIT_TIMEOUT == GetLastError()) {
			//	//printf("iocp time out\n");
			//	continue;
			//}
			if (dwTrans==0 && io_data!=NULL) {
				//˵����������ɶ˿ڵ�socket�ر�,�Է������رջ����쳣����
				if (io_data->long_pkg != NULL) {
					my_free(io_data->long_pkg);
					io_data->long_pkg = NULL;
				}
				//�����server sessionҪ����lost_server_connect
				
				close_session(s); 
				my_free(io_data);
			}
			else if (io_data!=NULL) {
				//���ret==0����io_data��ΪNULL��˵�����ǳ�ʱ������IOCP�����˴���
				LOGWARNING("error iocp %d\n", GetLastError());
				if (io_data->opt == IOCP_ACCPET) {
					closesocket(io_data->accpet_sock);
					post_accept(l_sock, iocp, io_data);
				}
				else if (io_data->opt == IOCP_RECV) {
					DWORD dwRecv = 0;
					DWORD dwFlags = 0;
					if (io_data->long_pkg != NULL) {
						my_free(io_data->long_pkg);
						io_data->long_pkg = NULL;

					}
					io_data->max_pkg_len = MAX_RECV_SIZE;
					io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
					io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
					int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
						1, &dwRecv, &dwFlags,
						&(io_data->overlapped), NULL);
				}
			}
			continue;
		}
		// IOCP�˿ڻ�����һ�������̣߳�
		// �������û���socket������¼�������;
		// printf("IOCP have event\n");
		if (dwTrans == 0 && io_data->opt == IOCP_RECV) { // socket �ر�
			close_session(s);
			free(io_data);
			continue;
		}// end

		switch (io_data->opt) {
		case IOCP_RECV: {
			io_data->recved += dwTrans;
			if (s->socket_type == TCP_SOCKET_IO) {
				if (protocal_type == BIN_PROTOCAL) {
					on_bin_protocal_recved(s, io_data);
				}
				else if (protocal_type == JSON_PROTOCAL) {
					on_json_protocal_recved(s, io_data);
				}
			}
			else if (s->socket_type == WEB_SOCKET_IO) {
				//���ж��Ƿ����ֳɹ�
				if (0 == s->is_shake_hand) {
					//����websocket����
					process_websocket_connect(s, io_data,ip,port);
				}
				else {
					//����websocket����Э��
					//�жϵ�һ���ֽ��Ƿ�Ϊwebsocket֡��ʽ
					process_websocket_data(s, io_data, protocal_type);
				
				}
		
			}
		}break;
		case IOCP_ACCPET:
		{
			int client_fd = io_data->accpet_sock;
			int addr_size = (sizeof(struct sockaddr_in) + 16);
			struct sockaddr_in* l_addr = NULL;
			int l_len = sizeof(struct sockaddr_in);

			struct sockaddr_in* r_addr = NULL;
			int r_len = sizeof(struct sockaddr_in);

			GetAcceptExSockaddrs(io_data->wsabuffer.buf,
				0, /*io_data->wsabuffer.len - addr_size * 2, */
				addr_size, addr_size,
				(struct sockaddr**)&l_addr, &l_len,
				(struct sockaddr**)&r_addr, &r_len);
			//����һ��session��ӵ��б���session��������socket
			struct session* s = save_session(client_fd, inet_ntoa(r_addr->sin_addr), ntohs(r_addr->sin_port));
			//���ó������������õ�socket_type
			s->socket_type = socket_type;
			//������ɶ˿ھ���������Ӿ����������lpCompletionKey
			//���lpCompletionKey������GetQueuedCompletioStatusʱ�򷵻�
			CreateIoCompletionPort((HANDLE)client_fd, iocp, (DWORD)s, 0);
			post_recv(client_fd, iocp);
			post_accept(l_sock, iocp);
		}
		break;
		}
	}
failed:
	if (iocp != NULL) {
		CloseHandle(iocp);
	}

	if (l_sock != INVALID_SOCKET) {
		closesocket(l_sock);
	}
	WSACleanup();
	exit_server_gateway();
	printf("server stop!!\n");
}

const char* conver_socket_type_str(int socket_type) {
	if (TCP_SOCKET_IO == socket_type) {
		return  "TCP_SOCKET";
	}
	else if (WEB_SOCKET_IO == socket_type) {
		return "WEB_SOCKET";
	}
	else {
		return "Unkonow";
	}
}

const char* conver_protocal_str(int protocal_type) {
	if (BIN_PROTOCAL == protocal_type) {
		return  "������Э��";
	}
	else if (JSON_PROTOCAL == protocal_type) {
		return "JSONЭ��";
	}
	else {
		return "Unkonow";
	}
}