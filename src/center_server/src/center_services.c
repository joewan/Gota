#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/log.h"
#include "../../types_service.h"
#include "../../protocal_cmd.h"
#include "../../gate_moduel/netbus/netbus.h"
#include "../../3rd/mjson/json_extends.h"
#include "center_services.h"

static void init_service_module(struct service_module* module) {
	
}

//jsonЭ�鴦��ӿ�
int on_center_json_protocal_data(void* moduel_data, struct session* s, json_t* root, unsigned char* pkg, int len) {
	printf("on_center_json_protocal_data\n");
	json_t* jcmd = json_object_at(root, "1");
	if (jcmd == NULL || jcmd->type != JSON_NUMBER) {
		return 0;
	}
	int cmd = atoi(jcmd->text);
	// ת���ϲ�ҵ��ģ�鴦��
	// end 
	json_t* jskey = json_object_at(root,"skey");
	if (NULL == jskey) {
		return 0;
	}
	//�ط���gateway
	json_t* juid = json_object_at(root, "uid");
	json_t* ret = json_new_comand((SYPTE_CENTER + TYPE_OFFSET), cmd);
	json_object_push_number(ret, "2", 1);
	json_object_push_number(ret, "uid", atoll(juid->text));
	json_object_push_number(ret, "skey", atoll(jskey->text));
	session_json_send(s, ret);
	json_free_value(&ret);
	return 0;
}

void on_center_connect_lost(void* module_data, struct session* s) {
	LOGINFO("center_connect_lost\n");
}

struct service_module CENTER_SERVICE = {
	SYPTE_CENTER,
	init_service_module,   //��ʼ������
	NULL,   //������Э��
	on_center_json_protocal_data,   //jsonЭ��
	on_center_connect_lost,   //���ӹر�
	NULL,   //ʹ���Զ�������
};
