#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "google\protobuf\message.h"
#include "session_export_to_lua.h"
#include "../utils/conver.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif
#include "../moduel/net/proto_type.h"
#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "../moduel/net/net_uv.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../moduel/netbus/netbus.h"
#include "../moduel/netbus/service_manger.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/netbus/recv_msg.h"
#include "../proto/proto_manage.h"

const char * session_moduel_name = "session_wrapper";
static unsigned s_function_ref_id = 0;
using namespace google::protobuf;
using namespace std;

//������lua��Ķ���export_session�Ľӿ�ʵ��
int lua_close_session(lua_State* tolua_s) {

	session_base* s = (session_base*)lua_touserdata(tolua_s,1);
	if (s==NULL) {
		return 0;
	}
	s->close();
	return 0;
}


//��protocol��message����ת����lua�ı���ʽ
static Message* create_message_from_lua_table(lua_State* tolua_s,int table_idx,const string& type_name) {
	if (type_name.empty() || !lua_istable(tolua_s,table_idx)) {
		return NULL;
	}

	/*
	ͨ�����ƴ�����Ӧ��Message,���ظ���ָ�룬ʵ�ʴ�����
	�Ǻ����ֶ�Ӧ����ʵ��
	*/
	Message* message = protoManager::create_message_by_name(type_name);
	if (message==NULL) {
		return NULL;
	}

	const Reflection* reflection = message->GetReflection();
	const Descriptor* descriptor = message->GetDescriptor();

	//ͨ��lua�����table����ֵMessage�������Ӧ���ֶ���required���ͣ�
	//����û�л�ȡֵ������ʧ��
	int file_count = descriptor->field_count();
	for (int i = 0; i < file_count;++i) {
		const FieldDescriptor* filedes = descriptor->field(i);
		const string& file_name = filedes->name();
		if (file_name.empty()) {
			delete message;
			message = NULL;
			return NULL;
		}

		bool is_required = filedes->is_required();
		bool is_repeated = filedes->is_repeated();
		
		//���ֶ����ݷŵ�ջ��
		lua_pushstring(tolua_s, file_name.c_str());
		//table[ջ��strnig]��ֵ����ջ��,������string
		//�� t[k] ֵѹ���ջ�� ����� t ��ָ��Ч���� index ָ���ֵ�� �� k ����ջ���ŵ�ֵ
		//�൱���߻�ȡջ��stringԪ��ֵ��Ҳ����file_name�ڴ����table���ȡֵ���ڷ���ջ����
		lua_rawget(tolua_s, table_idx);

		//ջ��Ԫ���Ƿ���ֵ
		bool isNil = lua_isnil(tolua_s, -1);
		if (is_repeated) {
			//��������
			if (isNil) {
				lua_pop(tolua_s, 1);
				continue;
			}
			else {
				//��lua����������table�ķ�ʽ����C++��,�����������ж������Ƿ���ȷ
				if (!lua_istable(tolua_s, -1)) {
					log_error("cant find repeated field %s\n", file_name.c_str());
					delete message;
					message = NULL;
					return NULL;
				}
			}
			lua_pushnil(tolua_s);
			//lua_next���ȵ���ջ��Ԫ�أ�����Ҫ�ȷ�һ��nil
			//Ȼ���key,value���� key����-2 value����-1��λ��

			for (; lua_next(tolua_s, -2) != 0;) {
				FieldDescriptor::CppType cpptype = filedes->cpp_type();
				switch (cpptype) {
				case FieldDescriptor::CPPTYPE_DOUBLE:
				{
					double value = luaL_checknumber(tolua_s, -1);
					reflection->AddDouble(message, filedes, value);

				}break;
				case FieldDescriptor::CPPTYPE_FLOAT:
				{
					float value = luaL_checknumber(tolua_s, -1);
					reflection->AddFloat(message, filedes, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_INT64:
				{
					int64_t value = luaL_checknumber(tolua_s, -1);
					reflection->AddInt64(message, filedes, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_UINT64:
				{
					uint64_t value = luaL_checknumber(tolua_s, -1);
					reflection->AddUInt64(message, filedes, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_ENUM: // ��int32һ������
				{
					int32_t value = luaL_checknumber(tolua_s, -1);
					const EnumDescriptor* enumDescriptor = filedes->enum_type();
					const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
					reflection->AddEnum(message, filedes, valueDescriptor);
				}
				break;
				case FieldDescriptor::CPPTYPE_INT32:
				{
					int32_t value = luaL_checknumber(tolua_s, -1);
					reflection->AddInt32(message, filedes, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_UINT32:
				{
					uint32_t value = luaL_checknumber(tolua_s, -1);
					reflection->AddUInt32(message, filedes, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_STRING:
				{
					size_t size = 0;
					const char* value = luaL_checklstring(tolua_s, -1, &size);
					reflection->AddString(message, filedes, std::string(value, size));
				}
				break;
				case FieldDescriptor::CPPTYPE_BOOL:
				{
					bool value = lua_toboolean(tolua_s, -1);
					reflection->AddBool(message, filedes, value);

				}
				break;
				case FieldDescriptor::CPPTYPE_MESSAGE:
				{
					//�ݹ����
					Message* value = create_message_from_lua_table(tolua_s, lua_gettop(tolua_s), filedes->message_type()->name().c_str());
					if (!value) {
						log_error("convert to message %s failed whith value %s \n", filedes->message_type()->name().c_str(), file_name.c_str());
						delete message;
						message = NULL;
						return NULL;
					}
					Message* msg = reflection->AddMessage(message, filedes);
					msg->CopyFrom(*value);
					delete msg;
					msg = NULL;
				}
				break;
				default:
					break;
				} //switch

				lua_pop(tolua_s, 1);
			}

		}
		else {
			//�������ͻ���Ƕ������
			if (is_required) {
				if (isNil) {
					log_error("cant find required field %s\n", file_name.c_str());
					delete message;
					message = NULL;
					return NULL;
				}
			}
			else {
				if (isNil) {
					lua_pop(tolua_s, 1);
					continue;
				}
			}
			switch (filedes->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				double value = luaL_checknumber(tolua_s, -1);
				reflection->SetDouble(message, filedes, value);
				
			}break;
			case FieldDescriptor::CPPTYPE_FLOAT:
			{
				float value = luaL_checknumber(tolua_s, -1);
				reflection->SetFloat(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT64:
			{
				int64_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetInt64(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT64:
			{
				uint64_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetUInt64(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_ENUM: // ��int32һ������
			{
				int32_t value = luaL_checknumber(tolua_s, -1);
				const EnumDescriptor* enumDescriptor = filedes->enum_type();
				const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
				reflection->SetEnum(message, filedes, valueDescriptor);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT32:
			{
				int32_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetInt32(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT32:
			{
				uint32_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetUInt32(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_STRING:
			{
				size_t size = 0;
				const char* value = luaL_checklstring(tolua_s, -1, &size);
				reflection->SetString(message, filedes, std::string(value, size));
			}
			break;
			case FieldDescriptor::CPPTYPE_BOOL:
			{
				bool value = lua_toboolean(tolua_s, -1);
				reflection->SetBool(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_MESSAGE:
			{
				//�ݹ����
				Message* value = create_message_from_lua_table(tolua_s, lua_gettop(tolua_s), filedes->message_type()->name().c_str());
				if (!value) {
					log_error("convert to message %s failed whith value %s \n", filedes->message_type()->name().c_str(), file_name.c_str());
					delete message;
					message = NULL;
					return NULL;
				}
				Message* msg = reflection->MutableMessage(message, filedes);
				msg->CopyFrom(*value);
				delete value;
				value = NULL;
			}
			break;
			default:
				break;
			} //switch

		}
	}
	return message;
}

// {1: stype, 2: ctype, 3: utag, 4 body}
int lua_send_msg(lua_State* tolua_s) {
	session_base* s = (session_base*)lua_touserdata(tolua_s, 1);
	if (s == NULL) {
		return 0;
	}

	if (!lua_istable(tolua_s, 2)) {
		return 0;
	}

	//local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=1}}
	//����Ҫʹ�������ĸ�ʽ��������
	//��ȡ��valueֵ,������ջ��3-6��λ��
	lua_getfield(tolua_s, 2,"stype");
	lua_getfield(tolua_s, 2, "ctype");
	lua_getfield(tolua_s, 2, "utag");
	lua_getfield(tolua_s, 2, "body");

	struct recv_msg msg;
	msg.head.stype = (int)lua_tointeger(tolua_s,3);
	msg.head.ctype = (int)lua_tointeger(tolua_s, 4);
	msg.head.utag = (int)lua_tointeger(tolua_s, 5);
	if (get_proto_type() == JSON_PROTOCAL) {
		//json����֮���Ӷ���ȡstringȻ����
		msg.body = (void*)lua_tostring(tolua_s,6);
		s->send_msg(&msg);
	}
	else if(get_proto_type() == BIN_PROTOCAL){
		//pb����������,lua��Ҫ{}�ĸ�ʽ�������ݣ���C++����ת����Message����
		if (!lua_istable(tolua_s,6)) {
			//û������
			msg.body = NULL;
			s->send_msg(&msg);
		}
		else {
			string type_name = protoManager::get_cmmand_protoname(msg.head.ctype);
			if (type_name.empty()) {
				msg.body = NULL;
				s->send_msg(&msg);
			}

			Message* pb_msg = create_message_from_lua_table(tolua_s,6,type_name);
			if (pb_msg==NULL) {
				//error log
				log_error("create_message_from_lua_table field error %s\n", type_name.c_str());
				return 0;
			}

			msg.body = (void*)pb_msg;
			s->send_msg(&msg);
			delete pb_msg;
			pb_msg = NULL;
		}
	}

	return 0;
}

static int lua_set_uid(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}
	session_base* s = (session_base*)tolua_touserdata(tolua_s, 1, NULL);
	if (s == NULL) {
		return 0;
	}
	int uid = (int)tolua_tonumber(tolua_s, 2, 0);
	s->set_uid(uid);
	return 0;
}

static int lua_get_uid(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 1) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	session_base* s = (session_base*)tolua_touserdata(tolua_s, 1, NULL);
	if (s == NULL) {
		lua_pushinteger(tolua_s,0);
		return 1;
	}

	lua_pushinteger(tolua_s, s->get_uid());
	return 1;
}

static int lua_set_utag(lua_State* tolua_s) {
	session_base* s = (session_base*)tolua_touserdata(tolua_s,1,NULL);
	if (s==NULL) {
		return 0;
	}

	int utag = tolua_tonumber(tolua_s,2,0);
	s->set_utag(utag);
	return 0;
}

static int lua_get_utag(lua_State* tolua_s) {
	session_base* s = (session_base*)tolua_touserdata(tolua_s, 1, NULL);
	if (s == NULL) {
		return 0;
	}

	tolua_pushnumber(tolua_s,s->get_utag());
	return 1;
}

int lua_get_session_type(lua_State* tolua_s) {
	session_base* s = (session_base*)tolua_touserdata(tolua_s, 1, NULL);
	if (s == NULL) {
		return 0;
	}

	tolua_pushnumber(tolua_s, s->get_is_server_session());
	return 1;
}

static int lua_get_socket_type(lua_State* tolua_s) {
	lua_pushnumber(tolua_s, get_socket_type());
	return 1;
}

static int lua_get_proto_type(lua_State* tolua_s) {

	lua_pushnumber(tolua_s, get_proto_type());
	return 1;
}

static int lua_set_socket_and_proto_type(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc!=2) {
		return 0;
	}

	int socket_type = lua_tonumber(tolua_s,1);
	int proto_type =  lua_tonumber(tolua_s, 2);
	init_socket_and_proto_type(socket_type, proto_type);
	return 0;
}

//�ж�session������
//1���������ӷ�������session����
//0�ǿͻ����������ص�session����
static int lua_is_client_session(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 1) {
		return 0;
	}
	session_base* session = (session_base*)lua_touserdata(tolua_s,1);
	if (session==NULL) {
		return 0;
	}
	lua_pushinteger(tolua_s, session->get_is_server_session());
	return 1;
}

static int lua_send_raw_msg(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}

	session_base* session = (session_base*)lua_touserdata(tolua_s,1);
	if (session==NULL) {
		return 0;
	}

	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s, 2);
	if (raw_data==NULL) {
		return 0;
	}

	session->send_raw_msg(raw_data);
	return 0;
}

int register_session_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//ע��һ������ģ��
		tolua_module(tolua_s, session_moduel_name, 0);

		//��ʼ����ģ��ӿ�
		tolua_beginmodule(tolua_s, session_moduel_name);
		tolua_function(tolua_s, "close_session", lua_close_session);
		tolua_function(tolua_s, "send_msg", lua_send_msg);
		tolua_function(tolua_s, "send_raw_msg", lua_send_raw_msg);
		tolua_function(tolua_s, "set_utag", lua_set_utag);
		tolua_function(tolua_s, "get_utag", lua_get_utag);
		tolua_function(tolua_s, "set_uid", lua_set_uid);
		tolua_function(tolua_s, "get_uid", lua_get_uid);
		tolua_function(tolua_s, "get_session_type", lua_get_session_type);
		tolua_function(tolua_s, "get_socket_type", lua_get_socket_type);
		tolua_function(tolua_s, "get_proto_type", lua_get_proto_type);
		tolua_function(tolua_s, "set_socket_and_proto_type", lua_set_socket_and_proto_type);
		tolua_function(tolua_s, "is_client_session", lua_is_client_session);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}