#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "google\protobuf\message.h"
#include "service_export_to_lua.h"
#include "../utils/conver.h"
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
#include "../database/redis_warpper.h"
#include "../database/query_callback.h"
#include "../moduel/netbus/netbus.h"
#include "../moduel/netbus/service_manger.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/netbus/recv_msg.h"

const char * service_moduel_name = "service_wrapper";
static unsigned s_function_ref_id = 0;

using namespace google::protobuf;

//�洢lua��������ĺ���handle_id
class lua_service_module :public service {
public:
	lua_service_module() {

		on_session_recv_cmd_handle = 0;
		on_session_disconnect_handle = 0;
		on_session_recv_raw_cmd_handle = 0;
	}

	virtual bool on_session_recv_cmd(struct session_base*s, recv_msg* msg);
	virtual void on_session_disconnect(struct session* s);
	virtual bool on_session_recv_raw_cmd(struct session_base* s, raw_cmd* msg);
public:
	int on_session_recv_cmd_handle;
	int on_session_disconnect_handle;
	int on_session_recv_raw_cmd_handle;
};

static void push_pb_message_tolua(const Message* pb_msg) {
	if (pb_msg==NULL) {
		return;
	}

	lua_State* lua_status = lua_wrapper::get_luastatus();

	lua_newtable(lua_status);
	const Descriptor* descriptor = pb_msg->GetDescriptor();
	if (descriptor==NULL) {
		return;
	}

	const Reflection* refletion = pb_msg->GetReflection();
	if (refletion == NULL) {
		return;
	}

	for (int i = 0; i < descriptor->field_count();++i) {
		const FieldDescriptor* filedes = descriptor->field(i);
		
		const std::string& file_name = filedes->lowercase_name();
		lua_pushstring(lua_status, file_name.c_str());

		//�Ƿ�Ϊһ������
		if (filedes->is_repeated()) {
			//��������ÿ��Ԫ��
			lua_newtable(lua_status);
			//�����С
			int size = refletion->FieldSize(*pb_msg, filedes);
			for (int i = 0; i < size;++i) {
				//�������ͣ���������Ƕ��message
				switch (filedes->cpp_type()) {
				case FieldDescriptor::CPPTYPE_DOUBLE: {
					lua_pushnumber(lua_status, refletion->GetDouble(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_FLOAT: {
					lua_pushnumber(lua_status, refletion->GetFloat(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_INT64: {
					std::string ss = convert<int64, std::string>(refletion->GetInt64(*pb_msg, filedes));
					lua_pushstring(lua_status, ss.c_str());

				}break;
				case FieldDescriptor::CPPTYPE_UINT64: {
					std::string ss = convert<int64, std::string>(refletion->GetUInt64(*pb_msg, filedes));
					lua_pushstring(lua_status, ss.c_str());
				}break;
				case FieldDescriptor::CPPTYPE_ENUM: {
					lua_pushinteger(lua_status, refletion->GetEnumValue(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_INT32: {
					lua_pushinteger(lua_status, refletion->GetInt32(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_UINT32: {
					lua_pushinteger(lua_status, refletion->GetUInt32(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_STRING: {
					std::string ss = refletion->GetString(*pb_msg, filedes);
					lua_pushstring(lua_status, ss.c_str());
				}break;
				case FieldDescriptor::CPPTYPE_BOOL: {
					lua_pushboolean(lua_status, refletion->GetBool(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_MESSAGE: {
					//������Ƕ��Message,��Ҫ�ݹ��ڴ˵���push_pb_message_tolua
					push_pb_message_tolua(pb_msg);
				}break;
				default: {

				}break;
				}//switch
				
				//t[n] = v,v��ջ�� ��������Ὣֵ����ջ
				lua_rawseti(lua_status, -2, i + 1);
			}
		}
		else {
			//�������ͣ���������Ƕ��message
			switch (filedes->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE: {
				lua_pushnumber(lua_status, refletion->GetDouble(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_FLOAT: {
				lua_pushnumber(lua_status, refletion->GetFloat(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_INT64: {
				std::string ss = convert<int64, std::string>(refletion->GetInt64(*pb_msg, filedes));
				lua_pushstring(lua_status, ss.c_str());
				
			}break;
			case FieldDescriptor::CPPTYPE_UINT64:{
				std::string ss = convert<int64, std::string>(refletion->GetUInt64(*pb_msg, filedes));
				lua_pushstring(lua_status, ss.c_str());
			}break;
			case FieldDescriptor::CPPTYPE_ENUM: {
				lua_pushinteger(lua_status, refletion->GetEnumValue(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_INT32: {
				lua_pushinteger(lua_status, refletion->GetInt32(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_UINT32: {
				lua_pushinteger(lua_status, refletion->GetUInt32(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_STRING: {
				std::string ss = refletion->GetString(*pb_msg, filedes);
				lua_pushstring(lua_status, ss.c_str());
			}break;
			case FieldDescriptor::CPPTYPE_BOOL: {
				lua_pushboolean(lua_status,refletion->GetBool(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_MESSAGE: {
				//������Ƕ��Message,��Ҫ�ݹ��ڴ˵���push_pb_message_tolua
				push_pb_message_tolua(pb_msg);
			}break;
			default: {

			}break;
			}//switch
		}
		//t[k] = v  ��������Ὣ����ֵ������ջ
		lua_rawset(lua_status,-3);
	}//for

}

bool lua_service_module::on_session_recv_raw_cmd(struct session_base* s, raw_cmd* msg) {
	if (s == NULL || msg == NULL) {
		return false;
	}
	lua_State* lua_status = lua_wrapper::get_luastatus();
	//ֱ�Ӱ�ԭʼ�������͵�lua��
	tolua_pushuserdata(lua_status, (void*)s);
	tolua_pushuserdata(lua_status, (void*)msg);
	if (on_session_recv_raw_cmd_handle!=0 && lua_wrapper::execute_service_fun_by_handle(on_session_recv_raw_cmd_handle, 2) == 0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_recv_raw_cmd_handle);
	}

	return true;
}

//Lua����������ջ�����ԣ���һ��ԭ����Զ��Ҫ��ָ��Lua�ַ�����ָ�뱣�浽�������ǵ��ⲿ������
//���յ���Ϣ�����stype�����ö�Ӧ�ĺ���,Ȼ���Э�����ݷ���ջ������lua����
//������msg���pb����ת��lua��table��ʽ
//���body����json���ַ�������ֱ�ӷ���lua��
bool lua_service_module::on_session_recv_cmd(struct session_base* s, recv_msg* msg) {
	
	if (s==NULL || msg==NULL) {
		return false;
	}
	lua_State* lua_status = lua_wrapper::get_luastatus();
	int idx = 1;
	
	tolua_pushuserdata(lua_status, s);
	//����һ����2��λ�ã�����{1: stype, 2 ctype, 3 utag, 4 pb{}���� json��string}
	lua_newtable(lua_status);
	//lua_rawseti
	lua_pushinteger(lua_status, msg->head.stype);
	lua_rawseti(lua_status,-2,idx);
	idx++;

	lua_pushinteger(lua_status, msg->head.ctype);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	lua_pushinteger(lua_status, msg->head.utag);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	if (msg->body == NULL) {
		lua_pushnil(lua_status);
	}
	else {
		if (get_proto_type() == BIN_PROTOCAL) {
			//��������pb��ʽ
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
			//pb������ת��table��ʽ,���ʱ�����ҪMessage�����name,value��Ϣ
			push_pb_message_tolua((const Message*)msg->body);
		}
		else if (get_proto_type() == JSON_PROTOCAL) {
			//�ַ���json��ʽ
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
			//json��lua�㣬ֱ�Ӱ�json�ı������ջ
			lua_pushstring(lua_status,(char*)msg->body);
		}
		lua_rawseti(lua_status, -2, idx);
		idx++;
	}

	//ִ��lua�ص�����
	if (lua_wrapper::execute_service_fun_by_handle(on_session_recv_cmd_handle, 2)==0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_recv_cmd_handle);
	}
	
	return true;
}

void lua_service_module::on_session_disconnect(struct session* s) {
	tolua_pushuserdata(lua_wrapper::get_luastatus(),s);
	if (lua_wrapper::execute_service_fun_by_handle(on_session_disconnect_handle, 1) == 0) {
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
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	if (tolua_istable(tolua_s,2,0,NULL)==0) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	//��ȡlua�����table��ֵ�����Ǻ�����ַ 3-4
	lua_getfield(tolua_s, 2, "on_session_recv_cmd");
	lua_getfield(tolua_s, 2, "on_session_disconnect");
	
	int on_session_recv_cmd_handle = save_service_function(tolua_s, 3, 0);
	int on_session_disconnect_handle = save_service_function(tolua_s, 4, 0);
	
	if (on_session_recv_cmd_handle ==0 && on_session_disconnect_handle ==0) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	lua_service_module* lua_module = new lua_service_module;
	if (lua_module == NULL) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	//����stype���ò�ͬ��service_module
	lua_module->using_direct_cmd = false;
	lua_module->stype = stype;
	lua_module->on_session_recv_cmd_handle = on_session_recv_cmd_handle;
	lua_module->on_session_disconnect_handle = on_session_disconnect_handle;
	lua_module->on_session_recv_raw_cmd_handle = 0;
	//ע�ᵽservice����ģ��
	server_manage::get_instance().register_service(stype, lua_module);
	lua_pushinteger(tolua_s,1);
	return 1;

}

int register_raw_service(lua_State* tolua_s) {
	int stype = (int)tolua_tonumber(tolua_s, 1, 0);
	if (stype <= 0 || stype > MAX_SERVICES) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	if (tolua_istable(tolua_s, 2, 0, NULL) == 0) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	//��ȡlua table��2������handle
	lua_getfield(tolua_s, 2, "on_session_recv_raw_cmd");
	lua_getfield(tolua_s, 2, "on_session_disconnect");
	int on_session_recv_raw_cmd = save_service_function(tolua_s, 3, 0);
	int on_session_disconnect_handle = save_service_function(tolua_s, 4, 0);
	
	if (on_session_recv_raw_cmd == 0 && on_session_disconnect_handle == 0) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	lua_service_module* lua_module = new lua_service_module;
	if (lua_module == NULL) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	lua_module->using_direct_cmd = true;
	lua_module->stype = stype;
	lua_module->on_session_recv_cmd_handle = 0;
	lua_module->on_session_disconnect_handle = on_session_disconnect_handle;
	lua_module->on_session_recv_raw_cmd_handle = on_session_recv_raw_cmd;

	server_manage::get_instance().register_service(stype, lua_module);
	lua_pushinteger(tolua_s, 1);
	return 1;
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
		//������ע��service�ӿ�
		tolua_function(tolua_s, "register_service", register_service);
		//����ע��service�ӿ�
		tolua_function(tolua_s, "register_raw_service", register_raw_service);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	
	return 0;
}

