--����������������
local stype = require("service_type")
                          
--gateway���ӵķ����������������
local remote_server = {}
remote_server[stype.AuthSerser] = {
   stype = stype.AuthSerser,
   ip = "127.0.0.1",
   port = 8000,
   desic = "AuthSerser",
}

--[[
remote_server[stype.SystemServer] = {
   stype= stype.SystemServer,
   ip= "127.0.0.1",
   port= 8001,
   desic = "SystemServer",
}

]]

local conf = {
    --tcp��������
	gateway_tcp_ip = "127.0.0.1",
	gateway_tcp_port = 6080,
	--websocket��������
	gateway_ws_ip = "127.0.0.1",
	gateway_ws_port = 6081,
	--����Զ��gateway��Ҫ���ӵ�������Ϣ
	servers = remote_server,
	
	--��֤������mysql
	auth_mysql= {
	  host = "123.206.46.126",      port = 3306,      db_name = "auth_center",      uname = "root",      upwd = "123321",
	},
}

return conf