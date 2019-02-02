--��ʼ����־ģ��
Logger_wrapper.init("logger/gateway/", "gateway", true)

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
local config = require("conf")
--�����������
--netbus_wrapper.tcp_listen("0.0.0.0",6080)
--netbus_wrapper.udp_listen("0.0.0.0",8002)
netbus_wrapper.tcp_listen(config.gateway_tcp_ip,config.gateway_tcp_port)

print("start gateway service success ip:".. config.gateway_tcp_ip,"port:"..config.gateway_tcp_port)
--ע������ת��ģ��
local servers = config.servers
local gate_service = require("gateway_server/gateway_service")
for k,v in pairs(servers) do
	--local ret = service_wrapper.register_service(v.stype, gate_service)
	local ret = service_wrapper.register_raw_service(v.stype, gate_service)
	if ret then
		print("register gw_servce:[" .. v.stype.. "] success!!!")
	else
		print("register gw_servce:[" .. v.stype.. "] failed!!!")
	end
end