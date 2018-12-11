#ifndef QUERY_CALLBACK_H__
#define QUERY_CALLBACK_H__

extern "C" {
#include "../3rd/mjson/json.h"
}
#include <vector>
#include <map>

typedef struct context_req {
	void* mysql_handle;
	json_t* root; //session�յ�������
}context_req;

typedef std::vector<std::vector<std::string> > DBRES;
typedef std::vector<std::map<std::string,std::string> > DBRESMAP;
typedef void(*cb_connect_db)(char* error, void* context,void* udata);
typedef void(*cb_query_db)(char*error, DBRES* res);
typedef void(*cb_close_db)(char*error);
/*
�ϲ�����ص���������
��ѯ�ɹ� error==NULL,��֮��ΪNULL����ѯ����
res.size()!=0˵����ѯ���������res.size()==0û�в�ѯ����¼
res��err�����ڵײ�ᱻ�ͷţ��ϴ�Ҫһֱʹ����Ҫcopyһ������
context����Ӧ�ò㴫����Զ�������
*/
typedef void(*cb_query_db_res_map)(char*error, DBRESMAP* res,void* context);

typedef void(*cb_query_no_res_cb)(char* err);


#endif