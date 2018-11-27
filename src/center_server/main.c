#include <stdio.h>
#include <string.h>
#include  <stdlib.h>
#include "../gate_moduel/net/net_io.h"
#include "../utils/log.h"
#include "./src/cener_config.h"
#include "../gate_moduel/services/service_gateway.h"
#include "../types_service.h"
#include "src/center_services.h"
int main(int argc, char** argv) {
	init_log();
	//ע��center����ģ��
	init_server_gateway();
	//��ʼ�����Ӷ�ʹ�õ�Э�� 
	init_session_manager(TCP_SOCKET_IO, JSON_PROTOCAL);
	
	register_services(SYPTE_CENTER, &CENTER_SERVICE);
	//��������
	//start_server(CENTER_CONF.ip, CENTER_CONF.port, WEB_SOCKET_IO, JSON_PROTOCAL);
	start_server(CENTER_CONF.ip, CENTER_CONF.port, TCP_SOCKET_IO, JSON_PROTOCAL);
	return 0;
}