#include <stdlib.h>
#include <string.h>
#include "proto_mgr_export_to_lua.h"
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
#include "../proto/proto_manage.h"

#define my_malloc malloc
#define my_free free

const char *  proto_moduel_name = "proto_mgr_wrapper";

static int lua_register_protobuf_cmd(lua_State*tolua_s) {
	/*
	luaע��Э�鴫��һ��table�ַ���
	���ظ���������ֵ�ġ����ȡ�
	*/
	int array_len = luaL_len(tolua_s,1);
	for (int i= 1; i <= array_len;++i) {
		lua_pushnumber(tolua_s,i);
		lua_gettable(tolua_s,1);
		const char* cmd_name = luaL_checkstring(tolua_s,-1);
		if (cmd_name!=NULL) {
			protoManager::register_cmd(i, cmd_name);
		}
		lua_pop(tolua_s, 1);
	}
	return 0;
}

int register_proto_export_tolua(lua_State*tolua_s) {

	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//ע��һ������ģ��
		tolua_module(tolua_s, proto_moduel_name, 0);

		//��ʼ����ģ��ӿ�
		tolua_beginmodule(tolua_s, proto_moduel_name);
		tolua_function(tolua_s, "register_protobuf_cmd", lua_register_protobuf_cmd);

		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}