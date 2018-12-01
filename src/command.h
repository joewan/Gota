#ifndef COMMAND_H__
#define COMMAND_H__

#include "3rd/mjson/json.h"
#include "./3rd/mjson/json_extends.h"

extern const int CENTER_TYPE;

void json_get_uid_and_key(json_t* cmd, unsigned int* uid, unsigned int* s_key);
json_t* json_new_server_return_cmd(int stype, int cmd,unsigned int uid,unsigned int s_key);
void write_error(struct session* s, int stype,
	int cmd, int status,char* errmsg,
	unsigned int uid, unsigned int s_key);
//����
enum {
	USER_LOST_CONNECT = 1,
	MAX_COMMON_NUM,
};

//���ķ�����Э��
enum {
	GUEST_LOGIN = MAX_COMMON_NUM, // �ο͵�½
	USER_LOGIN, // �û������¼
	EDIT_PROFILE, // �޸��û�����
};

//ͨ�÷�����Э��
//��Ϸ���ѷ�����
//��Ϸ���˷�����

#endif