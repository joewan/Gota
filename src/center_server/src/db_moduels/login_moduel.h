#ifndef LOGIN_MODUELS_H__
#define LOGIN_MODUELS_H__

// db_modyelsģ���¶��ǲ���mysql��redis���������ʵ��
// �����û���uid
enum {
	STATUS_SUCCESS = 0,
	STATUS_ERROR = -1,
};

struct user_info {
	unsigned int uid;
	char unick[64];
	int is_guest;
};

int query_guest_login(char* ukey, struct user_info* out_info);
#endif