--����socket���ͺ�Э�����ͣ��������ֵ�͵ײ�һֱ�����ﲻ�ܸı�
local socket_type = {
	TCP_SOCKET = 0,  --tcp
	WEB_SOCKET = 1,  --websocket
}

local proto_type = {
    PROTO_BUF = 0,
	PROTO_JSON = 1,
}

local socket_proto_type = {
	socket_type = socket_type,
	proto_type = proto_type,
}

return socket_proto_type
