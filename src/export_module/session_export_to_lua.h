#ifndef SESSION_EXPORT_TO_LUA_H__
#define SESSION_EXPORT_TO_LUA_H__

struct lua_State;
//����ע��redisģ�鵽lua�����
int register_session_export_tolua(lua_State*tolua_s);

#endif