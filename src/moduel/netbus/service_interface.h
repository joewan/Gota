#ifndef __SERVICE_INTERFACE_H__
#define __SERVICE_INTERFACE_H__

struct session;
struct recv_msg;
struct session_base;
struct raw_cmd;
//��ʾһ����lua��ע��ķ������ͣ���������Ӧ����lua��ص�����
class service {
public:
	service() { 
		stype = 0;
		using_direct_cmd = false;
	}
public:
	virtual bool on_session_recv_cmd(struct session_base* s, recv_msg* msg) = 0;
	virtual bool on_session_recv_raw_cmd(struct session_base* s, raw_cmd* msg) = 0;
	virtual void on_session_disconnect(struct session* s, int stype) = 0;
	virtual void on_session_connect(struct session_base* s, int stype) = 0;
public:
	//service��Ӧ��stype
	int stype;
	//ֱ��ת��ģʽ,Ϊtrue�������ض����Ʋ���������
	//2��ת��ģʽ: 
	//���ز������ֱ�ӽ�����ͷ������ctype��ת��  using_direct_cmd=true
	//������ģʽ:�����������ݰ���
	bool using_direct_cmd; 
};

#endif
