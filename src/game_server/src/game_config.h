#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

struct game_config {
	char* ip;
	int port;

	// ���ݿ⣬redis
	// end 
};

extern struct game_config GAME_CONF;
#endif

