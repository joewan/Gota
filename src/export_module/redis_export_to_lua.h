#ifndef REDIS_EXPORT_TO_LUA_H__
#define REDIS_EXPORT_TO_LUA_H__

struct lua_State;
//����ע��redisģ�鵽lua�����
int register_redis_export_tolua(lua_State*L);

#endif