#ifndef SESSION_KEY_MGR_H__
#define SESSION_KEY_MGR_H__

//��from_clientת������˷��񣬻�����һ��key��client��session��Ӧ
//������return_server���ظ�client����ʹ�����key�ҵ�session


void init_session_key_map();

void exit_session_key_map();

unsigned int get_session_key();

void save_session_by_key(unsigned int key,struct session* s);

void remove_session_by_key(unsigned int key);

void clear_session_by_value(struct session* s);

struct session* get_session_by_key(unsigned int key);

#endif 