#ifndef LOGIN_MODUELS_H__
#define LOGIN_MODUELS_H__

// db_modyelsģ���¶��ǲ���mysql��redis���������ʵ��
// �����û���uid
enum {
	MODEL_SUCCESS = 0,
	STATUS_ERROR = -1,
	MODEL_ACCOUNT_IS_NOT_GUEST=-2,
	MODEL_SYSTEM_ERR=-3,
};

int query_guest_login(char* rand_key, char* unick, unsigned int usex, unsigned int uface, struct user_info* out_info);
#endif