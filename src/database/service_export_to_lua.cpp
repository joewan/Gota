#include <stdlib.h>
#include <string.h>

#include "service_export_to_lua.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif

#include "../moduel/netbus/service_interface.h"
#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "hiredis.h"
#include "../moduel/net/net_uv.h"
#include "../3rd/tolua/tolua_fix.h"
#include "redis_warpper.h"
#include "query_callback.h"
#include "../moduel/netbus/netbus.h"
#include "../moduel/netbus/service_manger.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/netbus/recv_msg.h"

const char * service_moduel_name = "service_wrapper";
static unsigned s_function_ref_id = 0;

//�洢lua��������ĺ���handle_id
class lua_service_module :public service {
public:
	lua_service_module() {

		on_session_recv_cmd_handle = 0;
		on_session_disconnect_handle = 0;
	}

	virtual bool on_session_recv_cmd(struct session* s, recv_msg* msg);
	virtual void on_session_disconnect(struct session* s);
public:
	int on_session_recv_cmd_handle;
	int on_session_disconnect_handle;
};

//���յ���Ϣ�����stype�����ö�Ӧ�ĺ���,Ȼ���Э�����ݷ���ջ������lua����
bool lua_service_module::on_session_recv_cmd(struct session* s, recv_msg* msg) {
	
	if (s==NULL || msg==NULL) {
		return false;
	}
	lua_State* lua_status = lua_wrapper::get_luastatus();
	int idx = 1;
	tolua_pushuserdata(lua_status,s);

	//����һ��������{1: stype, 2 ctype, 3 utag, 4 body str}
	lua_newtable(lua_status);

	lua_pushinteger(lua_status, msg->stype);
	lua_rawseti(lua_status,-2,idx);
	idx++;

	lua_pushinteger(lua_status, msg->ctype);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	lua_pushinteger(lua_status, msg->utag);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	if (msg->body == NULL) {
		lua_pushnil(lua_status);
	}
	else {
		if (get_proto_type() == BIN_PROTOCAL) {
			//��������pb��ʽ
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
		}
		else if (get_proto_type() == JSON_PROTOCAL) {
			//�ַ���json��ʽ
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
			//����json
			lua_pushstring(lua_status,(char*)msg->body);
			lua_rawseti(lua_status, -2, idx);
			idx++;
		}
	}

	//ִ��lua�ص�����
	if (lua_wrapper::execute_service_fun_by_handle(on_session_recv_cmd_handle, 2)==0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_recv_cmd_handle);
	}
	
	return true;
}

void lua_service_module::on_session_disconnect(struct session* s) {
	tolua_pushuserdata(lua_wrapper::get_luastatus(),s);
	if (lua_wrapper::execute_service_fun_by_handle(on_session_disconnect_handle, 2) == 0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_disconnect_handle);
	}
}

static void init_service_function_map(lua_State* L) {
	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	lua_newtable(L);
	//��һ���ȼ��� t[k] = v �Ĳ����� ���� t ��һ��������Ч���� index ����ֵ�� v ָջ����ֵ�� �� k ��ջ��֮�µ��Ǹ�ֵ
	//���������Ѽ���ֵ���Ӷ�ջ�е���
	lua_rawset(L, LUA_REGISTRYINDEX);
}

//lo������ַ��ջ�е�λ��
static unsigned int save_service_function(lua_State* L, int lo, int def)
{
	if (!lua_isfunction(L, lo)){
		return 0;
	}

	s_function_ref_id++;

	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	//�� t[k] ֵѹ���ջ�� ����� t ��ָ��Ч���� index ָ���ֵ�� �� k ����ջ���ŵ�ֵ��
	//��������ᵯ����ջ�ϵ� key ���ѽ������ջ����ͬλ�ã�
	//ͨ��α������ȡSERVICE_FUNCTION_MAPPING��������ջ��
	lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: fun ... refid_fun */
	//���뺯����Ӧ��func_id
	lua_pushinteger(L, s_function_ref_id);                      /* stack: fun ... refid_fun refid */
	//����ջ�������� func_id table table_name
	//�Ѷ�ջ�ϸ�����Ч����������Ԫ����һ������ѹջ
	lua_pushvalue(L, lo);                                       /* stack: fun ... refid_fun refid fun */

	/*
	t[k] = v �Ĳ���
	���� t ��һ��������Ч���� index ����ֵ�� v ָջ����ֵ�� �� k ��ջ��֮�µ��Ǹ�ֵ
	������Ѽ���ֵ���Ӷ�ջ�е���
	*/
	lua_rawset(L, -3);                  /* refid_fun[refid] = fun, stack: fun ... refid_ptr */
	lua_pop(L, 1);                                              /* stack: fun ... */

	return s_function_ref_id;
}

/*luaע��serviceģ�� 
local my_service = {
-- msg {1: stype, 2 ctype, 3 utag, 4 body_table_or_str}
on_session_recv_cmd = function(session, msg)

end,

on_session_disconnect = function(session)
end
}

local ret = service.register(100, my_service)
*/
int register_service(lua_State* tolua_s) {
	int stype = (int)tolua_tonumber(tolua_s, 1,0);
	if (stype <= 0 || stype > MAX_SERVICES) {
		return -1;
	}

	if (tolua_istable(tolua_s,2,0,NULL)==0) {
		return -1;
	}

	//��ȡlua�����table��ֵ�����Ǻ�����ַ 3-4
	lua_getfield(tolua_s, 2, "on_session_recv_cmd");
	lua_getfield(tolua_s, 2, "on_session_disconnect");
	
	int on_session_recv_cmd_handle = save_service_function(tolua_s, 3, 0);
	int on_session_disconnect_handle = save_service_function(tolua_s, 4, 0);
	
	if (on_session_recv_cmd_handle ==0 && on_session_disconnect_handle ==0) {
		return -1;
	}

	lua_service_module* lua_module = new lua_service_module;
	if (lua_module == NULL) {
		return -1;
	}
	lua_module->stype = stype;
	lua_module->on_session_recv_cmd_handle = on_session_recv_cmd_handle;
	lua_module->on_session_disconnect_handle = on_session_disconnect_handle;
	//ע�ᵽservice����ģ��
	server_manage::get_instance().register_service(stype, lua_module);
	return 0;
}

int register_service_export_tolua(lua_State*tolua_s) {
	init_service_function_map(tolua_s);
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//ע��һ������ģ��
		tolua_module(tolua_s, service_moduel_name, 0);

		//��ʼ����ģ��ӿ�
		tolua_beginmodule(tolua_s, service_moduel_name);
		tolua_function(tolua_s, "register_service", register_service);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);

	return 0;
}