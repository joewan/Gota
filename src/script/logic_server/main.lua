--��ʼ����־ģ����������ǰ�棬�����������ģ���log��û�г�ʼ��logģ��
--true��ʾд�ļ�ͬʱ���������̨
Logger_wrapper.init("logger/logic/", "logic", true)

local socket_proto_type = require("socket_proto_type")
local stype = require("service_type")
local config = require("conf")

session_wrapper.set_socket_and_proto_type(socket_proto_type.socket_type.TCP_SOCKET,socket_proto_type.proto_type.PROTO_BUF)
--session_wrapper.set_socket_and_proto_type(socket_type.TCP_SOCKET,proto_type.PROTO_JSON)
--protobufЭ�飬ע��cmd
if session_wrapper.get_proto_type() == socket_proto_type.proto_type.PROTO_BUF then
   local cmd_name_map = require("cmd_name_map")
   if cmd_name_map then
	proto_mgr_wrapper.register_protobuf_cmd(cmd_name_map)
   end
end


local servers = config.servers
--�����������
print("start logic service success ip:".. servers[stype.LogicServer].ip,"port:"..servers[stype.LogicServer].port)
netbus_wrapper.tcp_listen(servers[stype.LogicServer].ip,servers[stype.LogicServer].port)
print("start logic udp ip:"..config.logic_upd.host.." port:"..config.logic_upd.port)
netbus_wrapper.udp_listen(config.logic_upd.host,config.logic_upd.port)

local logic_service = require("logic_server/logic_service")
--ע��auth����servicesģ��
print("service_wrapper.register_service call!!")
local ret = service_wrapper.register_service(stype.LogicServer, logic_service)
	if ret then
		print("register logic_servive:[" .. stype.LogicServer.. "] success!!!")
	else
		print("register logic_servive:[" .. stype.LogicServer.. "] failed!!!")
	end