#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/log.h"
#include "../../types_service.h"
#include "../../command.h"
#include "../../error_command.h"
#include "../../moduel/netbus/netbus.h"
#include "../../3rd/mjson/json_extends.h"
#include "center_services.h"

//�߼�ģ�����
#include "logic_moduels/auth.h"
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
static void init_service_module(struct service_module* module) {
	
}

//jsonЭ�鴦��ӿ�
int on_center_json_protocal_data(void* moduel_data, struct session* s, json_t* root, unsigned char* pkg, int len) {
	//printf("on_center_json_protocal_data\n");
	json_t* jcmd = json_object_at(root, "1");
	if (jcmd == NULL || jcmd->type != JSON_NUMBER) {
		return 0;
	}
	int cmd = atoi(jcmd->text);
	json_t* juid = json_object_at(root, "uid");
	json_t* jskey = json_object_at(root,"skey");
	unsigned int uid = strtoul(juid->text, NULL, 10);
	unsigned int skey = strtoul(jskey->text, NULL, 10);
	// ת���ϲ�ҵ��ģ�鴦��
	switch (cmd) {
	case GUEST_LOGIN: {
		guest_login(moduel_data,s, root,uid,skey);
	}break;

	}

	//�ط���gateway
//	json_t* ret = json_new_comand((SYPTE_CENTER + TYPE_OFFSET), cmd);
//	json_object_push_number(ret, "2", 1);
//#ifndef GAME_DEVLOP
//	json_object_push_number(ret, "uid", strtoul(juid->text,NULL,10));
//	json_object_push_number(ret, "skey", strtoul(jskey->text,NULL,10));
//#endif
//	session_json_send(s, ret);
//	json_free_value(&ret);
	return 0;
}

void on_center_connect_lost(void* module_data, struct session* s) {
#ifndef GAME_DEVLOP
	//����̲���ģʽ��sΪgateway����
	LOGINFO("center_connect_lost\n");
#else
	//�����̵��Է�ʽ��s��ǰ�����ӵ�client
#endif // !GAME_DEVLOP

	
}

struct service_module CENTER_SERVICE = {
	SYPTE_CENTER,
	init_service_module,   //��ʼ������
	NULL,   //������Э��
	on_center_json_protocal_data,   //jsonЭ��
	on_center_connect_lost,   //���ӹر�
	NULL,   //ʹ���Զ�������
};
