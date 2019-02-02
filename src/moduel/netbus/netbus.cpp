#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "recv_msg.h"
#include "../../types_service.h"
#include "netbus.h"
extern "C" {
#include "../../3rd/mjson/json_extends.h"
}
#ifdef GAME_DEVLOP
#include "../session/tcp_session.h"
#endif
#include "../../utils/logger.h"
#include "../../moduel/netbus/service_manger.h"
#include "../../proto/proto_manage.h"
#include "../../moduel/session/tcp_session.h"
#include "../../moduel/session/session_base.h"
#include "../../utils/mem_manger.h"
#include "../net/net_uv.h"
#include "../session/udp_session.h"


#define my_malloc malloc
#define my_free free


struct timer_list* NETBUS_TIMER_LIST = NULL;


extern void close_session(struct session* s);
extern void session_send(struct session*s, unsigned char* body, int len);
extern unsigned int get_session_key();
extern void save_session_by_key(unsigned int key, struct session* s);
//���ظ���ת���ĺ�̨serviceģ��
struct {
	struct service_module* services[MAX_SERVICES];
}gateway_services;



void register_services(int stype, struct service_module* module) {
	if (stype <= 0 || stype >= MAX_SERVICES || NULL == module) {
		printf("register error %d", stype);
		return;
	}

	gateway_services.services[stype] = module;
	if (module->init_service_module) {
		module->init_service_module(module);
	}

}

void init_server_netbus() {
	//����һ��luaģʽ
	memset(&gateway_services, 0, sizeof(gateway_services));
}

void exit_server_netbus() {
	//�ͷ��ڴ�
	for (int i = 0; i < MAX_SERVICES; ++i) {
		struct service_module* moduel = gateway_services.services[i];
		if (NULL!= moduel) {
			my_free(moduel);
		}
	}
}

//�ײ�������ֵײ��������ӶϿ���֪ͨ���ϲ�Ľӿ�
//�ýӿ�֪ͨ���ĵ�ģ��
void on_connect_lost(struct session* s) {
	for (int i = 0; i < MAX_SERVICES;++i) {
		if (gateway_services.services[i] && gateway_services.services[i]->on_connect_lost != NULL) {
			gateway_services.services[i]->on_connect_lost(gateway_services.services[i]->moduel_data,s);
		}
	}
}

static void echo_test(struct session_base* s, recv_msg* msg) {
	int out_len = 0;
	unsigned char* send_pkg = protoManager::encode_cmd_msg(msg,&out_len);
	
	session_send((struct session*)s, send_pkg, out_len);
	if (send_pkg!=NULL) {
		free(send_pkg);
	}
	
}
//������Э�鴦��,������ײ������һ��������package�����
void on_bin_protocal_recv_entry(struct session_base* s, unsigned char* data, int len) {
	//���ݰ�ͷ���type_name�ֶη��䴴��pb��message����
	/*recv_msg* msg = NULL;
	if (false == protoManager::decode_cmd_msg(data, len, &msg)) {
		return;
	}
	if (msg) {
		server_manage::get_instance().on_session_recv_cmd(s, msg);
		memory_mgr::get_instance().free_memory(msg);
	}*/

	raw_cmd rawmsg;
	if (false == protoManager::decode_rwa_cmd_msg(data, len,&rawmsg)) {
		//log
		return;
	}

	if (false==server_manage::get_instance().on_recv_raw_cmd(s,&rawmsg)) {
		//log
		return;
	}

}

//jsonЭ���ʽ����������ײ������һ��������package�����
void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	if (0 == len || NULL == data) {
		return;
	}
	data[len] = 0;
	json_t* root = NULL;
	int ret = json_parse_document(&root, (const char*)data);
	if (ret != JSON_OK || root == NULL) { // ����һ��������json��;
		return;
	}

	json_t* server_type = json_find_first_label(root, "0");
	server_type = server_type->child;
	if (server_type == NULL || server_type->type != JSON_NUMBER) {
		json_free_value(&root);
		return;
	}

	int stype = atoi(server_type->text);

	json_t* jcmd = json_object_at(root, "1");
	if (jcmd == NULL || jcmd->type != JSON_NUMBER) {
		return;
	}
	int cmd = atoi(jcmd->text);

	//����һ��recv_msg,���Կ���ʹ���ڴ��
	//recv_msg* msg = (recv_msg*)my_malloc(sizeof(recv_msg));
	recv_msg* msg = (recv_msg*)memory_mgr::get_instance().alloc_memory(sizeof(recv_msg));
	if (msg==NULL) {
		return;
	}
	msg->head.stype = stype;
	msg->head.ctype = cmd;
	msg->head.utag = 0;
	msg->body = (void*)data;
#ifdef USE_LUA
	server_manage::get_instance().on_session_recv_cmd(s, msg);
#else
	if (gateway_services.services[stype] && gateway_services.services[stype]->on_json_protocal_data) {
#ifdef GAME_DEVLOP
		//����ģʽ���ñ���ģ�飬������ģʽ
		//from_client�����json�����ֶ�uid,skey������д��
		if (0 == s->is_server_session) {
			unsigned int uid = 123;
			//uid = s->uid;
			//��ȡһ�����key��session��
			unsigned int session_key = get_session_key();
			save_session_by_key(session_key, s);
			json_object_push_number(root, "uid", uid);
			json_object_push_number(root, "skey", session_key); //��˷�����Ҫ͸���������ֵ
		}
#endif
	gateway_services.services[stype]->on_json_protocal_data(gateway_services.services[stype]->moduel_data, s, root, data, len);
#endif
	
	json_free_value(&root);
	//my_free(msg);
	memory_mgr::get_instance().free_memory(msg);
}

void tcp_listen(char* ip, int port) {
	if (ip == NULL) {
		ip = "0.0.0.0";
	}
	start_server(ip, port);
	log_debug("start server tcp at %s:%d\n", ip, port);
}

void udp_listen(char* ip, int port) {
	if (ip == NULL) {
		ip = "0.0.0.0";
	}
	udp_session::start_udp_server(ip,port);
	log_debug("start server udp at %s:%d\n", ip, port);
}

void run_loop() {
	run();
}

void tcp_connection(const char* server_ip, int port, void(*connect_cb)(const char* err, session_base* s, void* udata), void* udata) {
	tcp_connect(server_ip, port, connect_cb,udata);
}