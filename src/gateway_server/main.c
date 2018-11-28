#include <stdio.h>
#include <string.h>
#include  <stdlib.h>
#include "../gate_moduel/net/net_io.h"
#include "../utils/log.h"
#include "./src/gw_config.h"
#include "./src/server_session_mgr.h"
#include "src/from_client.h"
#include "src/return_server.h"
#include "../utils/hash_map_int.h"
//53
int main(int argc, char** argv) {
	//��ʼ��log���
	init_log();
	init_session_key_map();
	//��ʼ�����Ӻ�˷�������Ͷ�ʱ��
	init_server_session();
	init_server_gateway();
	//��ʼ���ͻ��˵���˷���ת������ģ��
	//for (int i = 0; i < GW_CONFIG.num_server_moudle;++i) {
	for (int i = 0; i < 1; ++i) {
		register_from_client_moduel(GW_CONFIG.module_set[i].stype);
		register_server_return_moduel(GW_CONFIG.module_set[i].stype);
	}

	//��ʼ��sessionģ��,�ڽ�������ͻ������ӵķ�����ó�ʼ�����ģ�� 
	init_session_manager(WEB_SOCKET_IO, JSON_PROTOCAL);
	//��������
	//start_server("127.0.0.1",8000,TCP_SOCKET_IO,BIN_PROTOCAL);
	//start_server("127.0.0.1", 8000, TCP_SOCKET_IO, JSON_PROTOCAL);
	LOGINFO("start gateway server at %s:%d\n", GW_CONFIG.ip, GW_CONFIG.port);
	start_server(GW_CONFIG.ip, GW_CONFIG.port, WEB_SOCKET_IO, JSON_PROTOCAL);


	exit_session_key_map();
	destroy_session_mgr();
	return 0;
}

