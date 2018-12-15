#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lua_wrapper.h"
#include "../../utils/logger.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../database/mysql_export_to_lua.h"
#include "../database/redis_export_to_lua.h"

lua_State* g_lua_state = NULL;

lua_State* lua_wrapper::get_luastatus() {
	return g_lua_state;
}

static void do_log_message(void(*log)(const char* file_name, int line_num, const char* msg), 	const char* msg) {	lua_Debug info;	int depth = 0;	while (lua_getstack(g_lua_state, depth, &info)) {		lua_getinfo(g_lua_state, "S", &info);		lua_getinfo(g_lua_state, "n", &info);		lua_getinfo(g_lua_state, "l", &info);		if (info.source[0] == '@') {			log(&info.source[1], info.currentline, msg);			return;		}		++depth;	}	if (depth == 0) {		log("trunk", 0, msg);	}}
//��ȡlua�ĵ�����Ϣ
static void print_error(const char* filename,int line_num,const char* msg) {
	logger::log(filename, line_num,ERROR,msg);
}

static void print_waring(const char* filename, int line_num, const char* msg) {
	logger::log(filename, line_num, WARNING, msg);
}

static void print_debug(const char* filename, int line_num, const char* msg) {
	logger::log(filename, line_num, DEBUG, msg);
}


//luaֻ�ܵ�������ǩ����C����
int lua_logdebug(lua_State* L) {
	//ȥջ��string���͵�Ԫ��
	
	const char* msg = luaL_checkstring(L,-1);
	if (msg!=NULL) {
		do_log_message(print_debug, msg);
	}
	return 0;
}


int lua_logwarning(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg != NULL) {
		do_log_message(print_waring,msg);
	}
	return 0;
}

int lua_logerror(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg != NULL) {
		do_log_message(print_error, msg);
	}
	return 0;
}

//lua_wrapper& lua_wrapper::get_instance() {
//	static lua_wrapper _instance;
//	return _instance;
//}

static intlua_panic(lua_State* pState) {	const char *msg = lua_tostring(pState, -1);	do_log_message(print_error, msg);	return 0;}


const char* lua_wrapper::read_table_by_key(lua_State* L, const char* table_name, const char* key) {
	if (table_name==NULL || key==NULL) {
		return NULL;
	}
	lua_getglobal(L, table_name);
	//�ж�ջ���Ƿ�Ϊtable����
	int l_type = lua_type(L, -1);
	if (l_type != LUA_TTABLE) {
		log_error("print_table input parment type error %d", l_type);
		return 0;
	}
	lua_pushstring(L, key);
	lua_gettable(L,-2);
	
	const char* value = lua_tostring(L, -1);
	lua_pop(L, 2);
	return value;
}

//test//////////////////////
//����һ����ֵͨ
int add(lua_State* pState) {
	int a = lua_tonumber(pState, 1);
	int b = lua_tonumber(pState, 2);
	int sum = a + b;
	lua_pushnumber(pState,sum);
	//printf("a+b=%d\n",a+b);
	return 1;
}

//����һ�����
int re_table(lua_State* pState) {
	//֪ͨlua����һ�����
	lua_newtable(pState);
	//����key value 
	lua_pushstring(pState,"name");
	lua_pushstring(pState,"tiny");
	//��ջ���key value���õ���
	//��ջ�� -1 -2λ��Ԫ�س�ջ��Ȼ��ִ��lua_pop(L,2)
	lua_settable(pState,-3);

	lua_pushstring(pState, "age");
	lua_pushnumber(pState, 30);
	////��ջ���key value���õ���
	lua_settable(pState, -3);
	
	return 1;
}
//lua---->C++ ����һ������ 
int print_array(lua_State* pState) {

	//��ȡջָ��λ�����ݳ���
	//����Ԫ�ظ��� string�����ַ�����
	int length = lua_rawlen(pState,1);
	//printf("length=%d\n", length);
	//lua�����1��ʼ
	for (int i = 1; i <= length;++i) {
		//��i push��ջ��
		lua_pushnumber(pState, i); //push i
		//�ڶ���������˵table��stack��λ��
		lua_gettable(pState,1); // pop i push table[i]
		size_t size = 0;
		const char* value = luaL_tolstring(pState, -1, &size);
		printf("arr :%d %s\n",i,value);
		//���ջ��Ԫ�� �ڶ��������Ǹ���
		lua_pop(pState,1);
	}

	return 0;
}

//lua--->C++ ����table key:value
int print_table(lua_State* pState) {

	//��������������,�ڶ��������ǲ�����ջ��λ��
	//���ʧ�ܺ�����ִ��
	//luaL_checktype(pState,1,LUA_TTABLE);
	int l_type = lua_type(pState,1);
	if (l_type != LUA_TTABLE) {
		log_error("print_table input parment type error %d", l_type);
		return 0;
	}
	//push niu��ջ 
	lua_pushnil(pState);
	/*
	����tableԪ��
     1.��pop key
	 2.��key����-2 value����-1��λ��
	*/
	size_t size = 0;
	while (lua_next(pState,1)!=0) {
		
		//��ȡkey value����
		//printf("key = %s\n", lua_typename(pState, lua_type(pState, -1) ));
		//printf("value = %s\n", lua_typename(pState, lua_type(pState, -2)));
		printf("key = %s\n", lua_tolstring(pState,-2,&size));
		printf("value = %s\n", lua_tolstring(pState, -1, &size));
		lua_pop(pState, 1);
	}

	//��ȡtable��key��Ӧ��value
	lua_getfield(pState,1,"name");
	printf("value = %s\n", lua_tolstring(pState, -1, &size));
	return 0;
}


//����ֶ�����
//����������ͨ���ͷ���ֵ
//��������ֵ��ʾ���������ز�������
int test_func_res(lua_State* pState) {
	//��ջ�����뷵��ֵ
	lua_pushstring(pState,"tiny");
	lua_pushnumber(pState,30);
	return 2;
}
//��������table���ͷ���ֵ
/////////////////////////////////////////////

void lua_wrapper::init_lua() {
	g_lua_state = luaL_newstate();
	//����luapanic������������ҽ��Զ��庯��	//��luaһ�Σ������abort������ֹ����	lua_atpanic(g_lua_state, lua_panic);	
	//ע��lua�� 
	luaL_openlibs(g_lua_state);
	//tolua++��ʼ��
	toluafix_open(g_lua_state);
	//ע�ᵼ������
	register_mysql_export_tolua(g_lua_state);
	register_redis_export_tolua(g_lua_state);
	/////////////////////////////////////////////////

	//������ܽӿ�
	reg_func2lua("LOGDEBUG", lua_logdebug);
	reg_func2lua("LOGWARNING", lua_logwarning);
	reg_func2lua("LOGERROR", lua_logerror);
	//test
	//reg_func2lua("Add", add);
	//reg_func2lua("print_array", print_array);
	//reg_func2lua("print_table", print_table);
	//reg_func2lua("re_table", re_table);
}

void lua_wrapper::exit_lua() {
	if (g_lua_state!=NULL) {
		lua_close(g_lua_state);
		g_lua_state = NULL;
	}
}

int lua_wrapper::exce_lua_file(const char* lua_file_path) {
	if (0!=luaL_dofile(g_lua_state,lua_file_path)) {
		lua_logerror(g_lua_state);
		return -1;
	}
	return 0;
}

void lua_wrapper::reg_func2lua(const char* func_name, int(*cfunction)(lua_State* L)) {
	lua_pushcfunction(g_lua_state, cfunction);
	lua_setglobal(g_lua_state,func_name);
}

int lua_wrapper::push_function_by_handle(int handle_id) {
	toluafix_get_function_by_refid(g_lua_state, handle_id);
	if (!lua_isfunction(g_lua_state,-1)) {
		log_error("push_function_by_handle error %d", handle_id);
		lua_pop(g_lua_state, 1);
		return -1;
	}
	return 0;
}

//�ú���copy quick-cocos2d��lua_bindingģ��CCLuaStatck.cpp����
int lua_wrapper::execute_function(int args_num) {
	int functionIndex = -(args_num + 1);
	if (!lua_isfunction(g_lua_state, functionIndex))
	{
		log_error("value at stack [%d] is not function", functionIndex);
		lua_pop(g_lua_state, args_num + 1); // remove function and arguments
		return 0;
	}

	int traceback = 0;
	//lua_getglobal��ȡȫ�ֱ�����ֵ��������ջ��
	lua_getglobal(g_lua_state, "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
	if (!lua_isfunction(g_lua_state, -1))
	{
		lua_pop(g_lua_state, 1);                                            /* L: ... func arg1 arg2 ... */
	}
	else
	{
		lua_insert(g_lua_state, functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
		traceback = functionIndex - 1;
	}

	int error = 0;
	error = lua_pcall(g_lua_state, args_num, 1, traceback);                  /* L: ... [G] ret */
	if (error)
	{
		if (traceback == 0)
		{
			log_error("[LUA ERROR] %s", lua_tostring(g_lua_state, -1));        /* L: ... error */
			lua_pop(g_lua_state, 1); // remove error message from stack
		}
		else                                                            /* L: ... G error */
		{
			lua_pop(g_lua_state, 2); // remove __G__TRACKBACK__ and error message from stack
		}
		return 0;
	}

	// get return value
	int ret = 0;
	if (lua_isnumber(g_lua_state, -1))
	{
		ret = (int)lua_tointeger(g_lua_state, -1);
	}
	else if (lua_isboolean(g_lua_state, -1))
	{
		ret = (int)lua_toboolean(g_lua_state, -1);
	}
	// remove return value from stack
	lua_pop(g_lua_state, 1);                                                /* L: ... [G] */

	if (traceback)
	{
		lua_pop(g_lua_state, 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
	}

	return ret;
}
int lua_wrapper::execute_lua_script_by_handle(int handle_id, int args_num) {
	int ret = 0; 
	if (0 == push_function_by_handle(handle_id)) {
		if (args_num > 0) {
			//��ջ��Ԫ�ز���ָ������Ч���������������ƶ��������֮�ϵ�Ԫ��
			lua_insert(g_lua_state,-(args_num+1));
		}
		//ִ�нű�����
		if (execute_function(args_num)!=0) {
			return -1;
		}
	}
	else {
		return -1;
	}
	lua_settop(g_lua_state,0);
	return 0;
}

int lua_wrapper::remove_lua_script_by_handle(int handle_id) {
	toluafix_remove_function_by_refid(g_lua_state,handle_id);
	return 0;
}