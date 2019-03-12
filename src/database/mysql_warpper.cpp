#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../utils/logger.h"
#include "uv.h"
#include "mysql.h"
#include "../moduel/net/net_uv.h"
#include "mysql_warpper.h"

#ifdef WIN32
#include <winsock.h>
#endif

extern uv_loop_t* get_uv_loop();

#define my_malloc malloc 
#define my_free free 

typedef struct connect_req {
	char ip[64];
	int port;
	char db_name[64];
	char uname[64];
	char upasswd[64];
	
	cb_connect_db f_connect_db;
	char* err;
	void* context;
	void* u_data;
}connect_req;

typedef struct close_req{
	char* err;
	void* context;
	cb_close_db f_close_cb;
}close_req;

typedef struct query_req {
	char* err;
	void* context; //�����mysql���
	char* sql;
	//DBRES* res;
	MYSQL_RES* res;
	DBRESMAP* resmap;
	void* r_context; //�����Ӧ�ò㴫���������
	void* udata; //lua�ص�����id
	cb_query_db f_query_cb; //���������vector<vector>
	cb_query_db_res_map f_query_map_cb;  //���������vector<map>
	cb_query_no_res_cb f_query_no_res;   //�޽��������
}query_req;

typedef struct mysql_lock_context {
	void* connect_handle;
	uv_mutex_t mutex;
}mysql_lock_context;

void on_connect_work_cb(uv_work_t* req) {
	connect_req* conn_req = static_cast<connect_req*>(req->data);
	if (conn_req==NULL) {
		return;
	}

	MYSQL*pConn = mysql_init(NULL);
	if (NULL==mysql_real_connect(pConn, conn_req->ip, conn_req->uname,
		conn_req->upasswd, conn_req->db_name, conn_req->port, NULL, 0)){
		log_debug("connect error!!! \n %s\n", mysql_error(pConn));
		conn_req->err = strdup(mysql_error(pConn));
		conn_req->context = NULL;
		mysql_close(pConn);
		pConn = NULL;
		
	}
	else {
		mysql_lock_context* pcontext = (mysql_lock_context*)my_malloc(sizeof(mysql_lock_context));
		if (pcontext==NULL) {
			exit(-2);
		}
		pcontext->connect_handle = (void*)pConn;
		uv_mutex_init(&(pcontext->mutex));
		conn_req->context = pcontext;
		conn_req->err = NULL;
	}

}

void on_connect_done_cb(uv_work_t* req, int status) {
	connect_req* conn_req = static_cast<connect_req*>(req->data);
	//֪ͨ�ϲ�ص�����,��������ӳ���������԰����Ӿ��֪ͨ���ϲ�
	conn_req->f_connect_db(conn_req->err, conn_req->context, conn_req->u_data);
	if (conn_req->err!=NULL) {
		//����Ҫ�ͷ�strdup��ȡ��char*
		my_free(conn_req->err);
		conn_req->err = NULL;
	}
	my_free(conn_req);
	my_free(req);
}

void mysql_wrapper::connect(const char* ip, int port, const char* db_name, const char* user_name,
	const char* passwd, cb_connect_db connect_db,void* udata) {

	if (ip==NULL || db_name==NULL) {
		log_error("connect db parament is null\n");
		return;
	}

	uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (work==NULL) {
		log_error("connect db malloc uv_work_t error\n");
		return;
	}
	memset(work,0,sizeof(uv_work_t));

	connect_req* conn_req = (connect_req*)malloc(sizeof(connect_req));
	if (conn_req == NULL) {
		log_error("connect db malloc connect_req error\n");
		return;
	}
	memset(conn_req,0,sizeof(connect_req));

	conn_req->port = port;
	conn_req->f_connect_db = connect_db;
	strncpy(conn_req->ip, ip,strlen(ip)+1);
	strncpy(conn_req->db_name, db_name, strlen(db_name)+1);
	strncpy(conn_req->uname,user_name,strlen(user_name)+1);
	strncpy(conn_req->upasswd, passwd, strlen(passwd) + 1);
	conn_req->u_data = udata; //�洢lua����handleid
	//Я���Լ����Զ�������
	work->data = static_cast<void*>(conn_req);
	
	/*
	uv_queue_work���uv_work_t���̶߳���
	on_connect_work_cb �̵߳�����ں���
	on_connect_done_cb ,��on_connect_work_cbִ����ɵ���
	*/
	uv_queue_work(get_uv_loop(), work, on_connect_work_cb, on_connect_done_cb);
}

void on_close_work_cb(uv_work_t* req){
	close_req* cl_req = static_cast<close_req*>(req->data);
	mysql_lock_context* c = (mysql_lock_context*)cl_req->context;
	uv_mutex_lock(&(c->mutex));
	MYSQL* pconn = static_cast<MYSQL*>(c->connect_handle);
	if (pconn!=NULL) {
		mysql_close(pconn);
		pconn = NULL;
	}
	uv_mutex_unlock(&(c->mutex));
}

void on_close_done_cb(uv_work_t* req, int status) {
	close_req* cl_req = static_cast<close_req*>(req->data);
	if (cl_req->f_close_cb!=NULL) {
		cl_req->f_close_cb(cl_req->err);
	}
	if (cl_req->err!=NULL) {
		my_free(cl_req->err);
	}

	mysql_lock_context* c = (mysql_lock_context*)cl_req->context;
	if (c!=NULL) {
		my_free(c);
	}
	my_free(cl_req);
	my_free(req);
}

//context��mysql_lock_context�ṹ
void mysql_wrapper::close(void* context, cb_close_db on_close) {
	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("close db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	close_req* req = (close_req*)my_malloc(sizeof(close_req));
	req->err = NULL; 
	req->context = context;
	req->f_close_cb = on_close;
	work->data = static_cast<void*>(req);
	uv_queue_work(get_uv_loop(), work, on_close_work_cb, on_close_done_cb);
 }


void on_query_work_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	mysql_lock_context* c = (mysql_lock_context*)q_req->context;
	
	MYSQL* mysql_handle = static_cast<MYSQL*>(c->connect_handle);
	if (mysql_handle==NULL) {
		return;
	}

	uv_mutex_lock(&(c->mutex));
	if (mysql_query(mysql_handle, q_req->sql)) {
		log_error("%s %s\n", q_req->sql, mysql_error(mysql_handle));
		q_req->err = strdup(mysql_error(mysql_handle));
		if (mysql_errno(mysql_handle) == 2006 || mysql_errno(mysql_handle) == 2013) {
			//����
			//mysql_ping(mysql_handle);
			//continue;
		}
		uv_mutex_unlock(&(c->mutex));
		return;
	}
	//
	//��ȡ�����
	MYSQL_RES* res = mysql_store_result(mysql_handle);
	if (res == NULL) {
		q_req->err = strdup("get store is null");
		q_req->f_query_cb(q_req->err, NULL,NULL);
		uv_mutex_unlock(&(c->mutex));
		return;
	}
	if (mysql_num_rows(res) == 0) {
		//û�в�ѯ�����ݣ�err����Ϊ NULL
		q_req->err = NULL;
		q_req->f_query_cb(q_req->err, NULL,NULL);
		mysql_free_result(res);
		uv_mutex_unlock(&(c->mutex));
		return;
	}
	
	int fields_num = mysql_field_count(mysql_handle);
	if (fields_num<=0) {
		q_req->f_query_cb(q_req->err, NULL,NULL);
		mysql_free_result(res);
		uv_mutex_unlock(&(c->mutex));
		return;
	}
	
	q_req->res = res;
	uv_mutex_unlock(&(c->mutex));
}

void on_query_done_cb(uv_work_t* req,int status) {
	query_req* q_req = static_cast<query_req*>(req->data);
	if (q_req->f_query_cb!=NULL) {
		//����ѯ�Ľ�����ص����ϲ��Ӧ��
		q_req->f_query_cb(q_req->err, q_req->res, q_req->udata);
	}

	if (q_req->err!=NULL) {
		my_free(q_req->err);
		q_req->err = NULL;
	}

	if (q_req->sql!=NULL) {
		my_free(q_req->sql);
		q_req->sql = NULL;
	}

	if (q_req->res!=NULL) {
		mysql_free_result(q_req->res);
		q_req->res = NULL;
	}
	my_free(q_req);
	my_free(req);
}

void on_query_map_work_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	mysql_lock_context* c = (mysql_lock_context*)q_req->context;
	MYSQL* mysql_handle = static_cast<MYSQL*>(c->connect_handle);

	uv_mutex_lock(&(c->mutex));
	if (mysql_query(mysql_handle, q_req->sql)) {
		log_error("%s %s\n", q_req->sql, mysql_error(mysql_handle));
		q_req->err = q_req->sql, mysql_error(mysql_handle);
		if (mysql_errno(mysql_handle) == 2006 || mysql_errno(mysql_handle) == 2013) {
			//����
			//mysql_ping(mysql_handle);
			//continue;
		}
		uv_mutex_unlock(&(c->mutex));
		return;
	}

	//��ȡ�����
	MYSQL_RES* res = mysql_store_result(mysql_handle);
	if (res == NULL) {
		q_req->err = strdup("get store is null");
		q_req->f_query_map_cb(q_req->err, NULL, q_req->r_context);
		uv_mutex_unlock(&(c->mutex));
		return;
	}
	if (mysql_num_rows(res) == 0) {
		q_req->f_query_map_cb(q_req->err, NULL,q_req->r_context);
		mysql_free_result(res);
		uv_mutex_unlock(&(c->mutex));
		return;
	}

	int fields_num = mysql_field_count(mysql_handle);
	if (fields_num <= 0) {
		mysql_free_result(res);
		uv_mutex_unlock(&(c->mutex));
		return;
	}

	std::vector<std::string> fileld_names;
	MYSQL_FIELD *field=NULL;
	while ((field = mysql_fetch_field(res)))
	{
		fileld_names.push_back(field->name);
	}
	
	DBRESMAP* db_res = new DBRESMAP;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != NULL) {
		//std::vector<std::string> d_row;
		std::map<std::string,std::string> d_row;
		
		for (int i = 0; i < fields_num; i++) {
			if (row[i] == NULL) {
				
				d_row.insert(std::make_pair(fileld_names.at(i),""));
			}
			else {
				
				d_row.insert(std::make_pair(fileld_names.at(i), row[i]));
			}
		}
	
		db_res->push_back(d_row);
	}
	q_req->resmap = db_res;
	mysql_free_result(res);

	uv_mutex_unlock(&(c->mutex));
}

void mysql_wrapper::query2map(void* context, const char* sql, cb_query_db_res_map on_query_map) {
	//if (context == NULL || sql == NULL) {
	//	return;
	//}

	//uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	//if (work == NULL) {
	//	log_error("query db malloc uv_work_t error\n");
	//	return;
	//}
	//memset(work, 0, sizeof(uv_work_t));
	//query_req* req = (query_req*)my_malloc(sizeof(query_req));
	//if (req == NULL) {
	//	log_error("query db malloc query_req error\n");
	//	return;
	//}
	//context_req* r_context = (context_req*)context;
	//req->context = r_context->mysql_handle;
	//req->sql = strdup(sql); //�ڻص���ɺ�Ҫ�ͷ�
	//req->f_query_map_cb = on_query_map;
	//req->err = NULL;
	//req->res = NULL;
	//req->r_context = r_context; //�����Ǵ洢lua�Ļص�����handleֵ
	//work->data = static_cast<void*>(req);
	//uv_queue_work(get_uv_loop(), work, on_query_map_work_cb, on_query_done_cb);

}

void mysql_wrapper::query(void* context,const char* sql, cb_query_db on_query, void* udata) {
	if (context==NULL ||sql==NULL) {
		return;
	}

	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("query db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	query_req* req = (query_req*)my_malloc(sizeof(query_req));
	if (req==NULL) {
		log_error("query db malloc query_req error\n");
		return;
	}
	req->context = context;
	req->sql = strdup(sql); //�ڻص���ɺ�Ҫ�ͷ�
	req->f_query_cb = on_query;
	req->err = NULL;
	req->res = NULL;
	req->udata = udata;
	work->data = static_cast<void*>(req);
	uv_queue_work(get_uv_loop(), work, on_query_work_cb, on_query_done_cb);
 }

void on_query_done_no_res_cb(uv_work_t* req, int status) {
	query_req* q_req = static_cast<query_req*>(req->data);
	if (q_req->f_query_no_res!=NULL) {
		q_req->f_query_no_res(q_req->err);
	}
}

void on_query_work_no_res_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	MYSQL* mysql_handle = static_cast<MYSQL*>(q_req->context);

	if (mysql_query(mysql_handle, q_req->sql)) {
		q_req->err = strdup(mysql_error(mysql_handle));
		return;
	}
}

void mysql_wrapper::query_no_res(void* context, const char* sql, cb_query_no_res_cb on_query_no_res_cb) {
	if (context == NULL || sql == NULL) {
		return;
	}

	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("query db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	query_req* req = (query_req*)my_malloc(sizeof(query_req));
	if (req == NULL) {
		log_error("query db malloc query_req error\n");
		return;
	}
	req->context = context;
	req->sql = strdup(sql); //�ڻص���ɺ�Ҫ�ͷ�
	req->f_query_no_res = on_query_no_res_cb;
	req->err = NULL;
	req->res = NULL;
	work->data = static_cast<void*>(req);

	uv_queue_work(get_uv_loop(), work, on_query_work_no_res_cb, on_query_done_no_res_cb);

}
