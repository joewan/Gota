#include <stdlib.h>
#include <string.h>
#include "netbus_export_to_lua.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif

#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../moduel/netbus/netbus.h"

#define my_malloc malloc
#define my_free free

const char *  netbus_moduel_name = "netbus_wrapper";

static int lua_tcp_listen(lua_State* tolua_s) {
	//����ջ��Ԫ�ص������� ��Ϊ�����Ǵ� 1 ��ʼ��ŵģ� �������������ڶ�ջ�ϵ�Ԫ�ظ���
	int argc = lua_gettop(tolua_s);
	if (argc!=2) {
		return 0;
	}

	char* ip = (char*)lua_tostring(tolua_s,1);
	int port = lua_tonumber(tolua_s,2);
	tcp_listen(ip, port);
	return 0;
}

static int lua_udp_listen(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}

	char* ip = (char*)lua_tostring(tolua_s, 1);
	int port = lua_tonumber(tolua_s, 2);
	udp_listen(ip, port);
}

int register_betbus_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//ע��һ������ģ��
		tolua_module(tolua_s, netbus_moduel_name, 0);

		//��ʼ����ģ��ӿ�
		tolua_beginmodule(tolua_s, netbus_moduel_name);
		tolua_function(tolua_s, "tcp_listen", lua_tcp_listen);
		tolua_function(tolua_s, "udp_listen", lua_udp_listen);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}
