#include <stdio.h>
#include <string.h>
#include  <stdlib.h>
#include "../gate_moduel/net/net_io.h"
#include "../utils/log.h"
#include "./src/command_config.h"

int main(int argc, char** argv) {
	init_log();
	//��������
	//start_server("127.0.0.1",8000,TCP_SOCKET_IO,BIN_PROTOCAL);
	//start_server("127.0.0.1", 8000, TCP_SOCKET_IO, JSON_PROTOCAL);
	start_server(COMMAND_CONF.ip, COMMAND_CONF.port, WEB_SOCKET_IO, JSON_PROTOCAL);
	return 0;
}