--��ʼ����־ģ��
Logger_wrapper.init("logger/auth/", "auth", true)

local socket_type = {
	TCP_SOCKET = 0,  --tcp
	WEB_SOCKET = 1,  --websocket
}

local proto_type = {
    PROTO_BUF = 0,
	PROTO_JSON = 1,
}

session_wrapper.set_socket_and_proto_type(socket_type.TCP_SOCKET,proto_type.PROTO_BUF)
--protobufЭ�飬ע��cmd
if session_wrapper.get_proto_type() == proto_type.PROTO_BUF then
   local cmd_name_map = require("cmd_name_map")
   if cmd_name_map then
	proto_mgr_wrapper.register_protobuf_cmd(cmd_name_map)
   end
end

--��������
local stype = require("service_type")
local config = require("conf")
local servers = config.servers
--�����������
print("start gateway service success ip:".. servers[stype.AuthSerser].ip,"port:"..servers[stype.AuthSerser].port)
netbus_wrapper.tcp_listen(servers[stype.AuthSerser].ip,servers[stype.AuthSerser].port)

local auth_service = require("auth_server/auth_service")
--ע��auth����servicesģ��
local ret = service_wrapper.register_service(stype.AuthSerser, auth_service)
	if ret then
		print("register auth_servive:[" .. stype.AuthSerser.. "] success!!!")
	else
		print("register auth_servive:[" .. stype.AuthSerser.. "] failed!!!")
	end