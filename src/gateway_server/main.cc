#include <stdio.h>
#include <string.h>
#include  <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/log.h"
#ifdef __cplusplus
}
#endif

#include "./src/gw_config.h"
#include "../../moduel/net/net_uv.h"
#include "src/from_client.h"
#include "src/return_server.h"
#include "../utils/hash_map_int.h"
#include "../../moduel/netbus/session_key_mgr.h"
#include "../moduel/netbus/netbus.h"
#ifdef GAME_DEVLOP

#include "../center_server/src/center_services.h"
#endif

#include "../lua_wrapper/lua_wrapper.h"
#include "../database/center_db.h"
#include "./src/server_session_mgr.h"
#include "../proto/pb_cmd_map.h"
#include "../moduel/session/udp_session.h"
//#include "../proto/message.pb.h"
//28
int main(int argc, char** argv) {
	
	//��ʼ��log���
	init_log();
	init_uv();
	init_session_key_map();
	//��ʼ�����Ӻ�˷�������Ͷ�ʱ��
	init_server_session();
	init_server_netbus();
	init_pb_cmd_map();
#ifdef GAME_DEVLOP
	/*
	  GMAE_DEVLOP�궨������:
	  ������ԣ��ڿ������Խ׶Σ�����������������ģ����룬
	  �ڶ���GMAE_DEVLOP�󣬸��������߼�����һ��������Ժ��޸�,
	  �ڿ�����ɺ�ȥ��GMAE_DEVLOP����Ϳ���������̷ֿ�����
	*/
	register_services(SYPTE_CENTER,&CENTER_SERVICE);
	//connect_to_centerdb();
#else
	//��ʼ���ͻ��˵���˷���ת������ģ��
	//for (int i = 0; i < GW_CONFIG.num_server_moudle;++i) {
	for (int i = 0; i < 1; ++i) {
		 (GW_CONFIG.module_set[i].stype);
		register_server_return_moduel(GW_CONFIG.module_set[i].stype);
	}
#endif 
	int socket_type = TCP_SOCKET_IO;
	int proto_type = BIN_PROTOCAL;
	//��ʼ��sessionģ��,�ڽ�������ͻ������ӵķ�����ó�ʼ�����ģ�� 
	init_session_manager(socket_type, proto_type);
	//��ʼ��lua�����
	lua_wrapper::init_lua();
	int a = lua_wrapper::exce_lua_file("./main.lua");
	//�ýӿ���C++��ȡlua table�������ýӿ�
	//const char* value = lua_wrapper::read_table_by_key(g_lua_state,"table1","name");

	//lua_getglobal(g_lua_state, "myname");
	////const char* name = luaL_checkstring(L, -1);
	//int idx = lua_gettop(g_lua_state);
	//printf("lua var =%s\n", lua_tostring(g_lua_state, -1));
	//lua_pop(g_lua_state, 1);

	//��������
	//start_server("127.0.0.1",8000,TCP_SOCKET_IO,BIN_PROTOCAL);
	//start_server("127.0.0.1", 8000, TCP_SOCKET_IO, JSON_PROTOCAL);
	LOGINFO("start gateway server at %s:%d\n", GW_CONFIG.ip, GW_CONFIG.port);
	//tcp+protobuf��ʽ
	start_server(GW_CONFIG.ip, GW_CONFIG.port, socket_type, proto_type);
	//udp_session::start_udp_server();
	//websocket+jsob��ʽ
	//start_server(GW_CONFIG.ip, GW_CONFIG.port, WEB_SOCKET_IO, JSON_PROTOCAL);


	exit_session_key_map();
	destroy_session_mgr();
	exit_server_netbus();
	lua_wrapper::exit_lua();
	return 0;
}

