#include <stdio.h>
#include <string.h>
#include  <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "../moduel/net/net_io.h"
#include "../utils/log.h"
#include "./src/gw_config.h"
#include "./src/server_session_mgr.h"
#include "src/from_client.h"
#include "src/return_server.h"
#include "../utils/hash_map_int.h"
#include "../../moduel/netbus/session_key_mgr.h"

#ifdef GAME_DEVLOP
#include "../moduel/netbus/netbus.h"
#include "../center_server/src/center_services.h"

#endif

#ifdef __cplusplus
}
#endif

#include "../lua_wrapper/lua_wrapper.h"
#include "../database/center_db.h"


//28
int main(int argc, char** argv) {
	
	//初始化log组件
	init_log();
	init_uv();
	init_session_key_map();
	//初始化连接后端服务组件和定时器
	init_server_session();
	init_server_netbus();
#ifdef GAME_DEVLOP
	/*
	  GMAE_DEVLOP宏定义用于:
	  方便调试，在开发调试阶段，可以引入其他服务模块代码，
	  在定义GMAE_DEVLOP后，各个进程逻辑都在一个服务调试和修改,
	  在开发完成后，去掉GMAE_DEVLOP定义就可以做多进程分开部署
	*/
	register_services(SYPTE_CENTER,&CENTER_SERVICE);
	//connect_to_centerdb();
#else
	//初始化客户端到后端服务转发处理模块
	//for (int i = 0; i < GW_CONFIG.num_server_moudle;++i) {
	for (int i = 0; i < 1; ++i) {
		 (GW_CONFIG.module_set[i].stype);
		register_server_return_moduel(GW_CONFIG.module_set[i].stype);
	}
#endif 
	//初始化session模块,在接入大量客户端连接的服务采用初始化这个模块 
	init_session_manager(WEB_SOCKET_IO, JSON_PROTOCAL);
	//初始化lua虚拟机
	lua_wrapper::init_lua();
	lua_wrapper::exce_lua_file("./main.lua");
	//该接口是C++读取lua table类型配置接口
	//const char* value = lua_wrapper::read_table_by_key(g_lua_state,"table1","name");

	//lua_getglobal(g_lua_state, "myname");
	////const char* name = luaL_checkstring(L, -1);
	//int idx = lua_gettop(g_lua_state);
	//printf("lua var =%s\n", lua_tostring(g_lua_state, -1));
	//lua_pop(g_lua_state, 1);

	//启动服务
	//start_server("127.0.0.1",8000,TCP_SOCKET_IO,BIN_PROTOCAL);
	//start_server("127.0.0.1", 8000, TCP_SOCKET_IO, JSON_PROTOCAL);
	LOGINFO("start gateway server at %s:%d\n", GW_CONFIG.ip, GW_CONFIG.port);
	start_server(GW_CONFIG.ip, GW_CONFIG.port, WEB_SOCKET_IO, JSON_PROTOCAL);


	exit_session_key_map();
	destroy_session_mgr();
	exit_server_netbus();
	lua_wrapper::exit_lua();
	return 0;
}

