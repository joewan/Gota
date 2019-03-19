local config = require("conf")
local stype_module = require("service_type")

local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")

--47min uid����д�꣬��δ����
--stype���������ӵ�session��sessionӳ��,�洢�����ӵķ�������session
local session_map = {}
--����������ӻ�δ�ɹ���session
local session_connecting = {}

local g_ukey = 1;
--��ʱutagӳ��session�����key��lua����
local client_session_utag = {}
--uidӳ��session
local client_session_uid = {}

function connect_to_server(stype,server_ip,server_port)
	netbus_wrapper.tcp_connect(server_ip,server_port,function(err,session)
		--����ʧ��
		if err ~= nil then
	       session_connecting[stype] = false
		   print("connect error server["..config.servers[stype].desic.."]".."error:"..err)
		   return
	    end
		--���ӳɹ�
		session_map[stype] = session
		print("connect sucess server["..config.servers[stype].desic.."]")
end
)
end

function check_session_connect()
	--����������Ҫ���ӵķ�����
	for k,v in pairs(config.servers) do
		if session_map[v.stype] == nil and session_connecting[v.stype] == false then
			session_connecting[v.stype] = true
			print("connecting server["..v.desic.."]"..v.ip..":"..v.port)
			connect_to_server(v.stype,v.ip,v.port)
		end
	end
end

function server_session_init()
	for k,v in pairs(config.servers) do
		session_map[v.stype] = nil
		session_connecting[v.stype] = false
	end
	--�������Ӷ�ʱ��
	timer_wrapper.create_timer(check_session_connect,-1,1000,1000)
end

--local socket_type = WEB_SOCKET
--local proto_type =  PROTO_JSON
function is_login_request(cmd_type)
	if cmd_type == cmd_module.GuestLoginReq then 
		return true
	end

	return false
end

--�ͻ��˷��͹����������ط�����ת����������
function send_to_server(client_session,raw_data)
	local stype,cmd,utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print(stype,cmd,utag)
	
	--��ȡ���������ӵ�session
	local server_session = session_map[stype]
	if server_session == nil then
	print("not found session stype:"..stype)
	   return	
	end
	
	--�ж��Ƿ�Ϊ��¼Э������
	if is_login_request(cmd) then
	  utag = session_wrapper.get_utag(client_session)
	   --��û��utag������һ��tag,�ڵ�¼ǰʹ��
	   if utag ==0 then
		  --��û����ʱ��utag
		  utag = g_ukey
		  g_ukey = g_ukey + 1
		 
		  --����session��utagֵ
		  session_wrapper.set_utag(client_session,utag)
	   end
		   --��ʱ��key�Ϳͻ���session��һ����ʱ�Ĺ�ϵӳ��
	       client_session_utag[utag] = client_session
		else
		 --���ﴦ���ǵ�¼����
		 local uid = session_wrapper.get_uid(client_session)
		 utag = uid
	end
	 
	 --�ȸ����ݰ�д��utag,���������ݷ��ط���client_session����ӳ���ϵ
	 proto_mgr_wrapper.set_raw_utag(raw_data,utag)
	 --�������ݸ�stype��Ӧ�ķ�����
	 session_wrapper.send_raw_msg(server_session,raw_data)

end

--�ж��Ƿ�Ϊ��¼����Э��
function is_login_ctype(ctype)
	if ctype == cmd_module.GuestLoginRes then
		return true
	end
	return false
end

--����������������Ϣ��ת����Ӧ�Ŀͻ���
function send_to_client(server_session,raw_data)
	local cmdtype, ctype, utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print(utag)
	local client_session = nil
	
	--�ж��Ƿ�Ϊ��¼����Э��
	print("send_to_client ctype,"..ctype)
	if is_login_ctype(ctype) == true then
	    print("is_login_ctype")
		--��¼Э�鷵�أ��������ȡ��֤���������ص�uid
		local t_body = proto_mgr_wrapper.read_msg_body(raw_data)
		if t_body == nil then
		   print("t_body is nil")
		   return
		end
		--if true == true then
		--    return
		--end

		client_session = client_session_utag[utag]
		if client_session==nil then
			print("client_session is nil")
			return
		end
		if t_body.status ~= res_module.OK then
		   
		   proto_mgr_wrapper.set_raw_utag(raw_data,0)
		   if client_session ~= nil then
			  session_wrapper.send_raw_msg(client_session,raw_data)
			end
		end
		--local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=200}}
		local t_userinfo = t_body.userinfo
		local login_uid = t_userinfo.uid
		print("login_uid"..login_uid)
		if login_uid~= 0 then 
		   --�ж��Ƿ���session�Ƿ��Ѿ���¼
		   if client_session_uid[login_uid] ~= nil and client_session_uid[login_uid] ~= client_session then
		       print("Relogin uid is"..login_uid)
		       --����һ���ظ���¼��Ϣ,�¼�һ���ظ���¼Э��21
			  local ret_msg = {stype=stype_module.AuthSerser,ctype=cmd_module.ReLoginRes,utag=0,body=nil}
			  session_wrapper.send_msg(client_session,ret_msg)
			   --ɾ���Ѿ����ڵ�session
			   session_wrapper.close_session(client_session)
			   client_session_uid[login_uid] = nil
		   end 
		end

		--�Ȱѵ�¼ǰ��utag�����utag��Ӧ��ֵ����Ϊ��
		client_session_utag[utag] = nil
		client_session_uid[login_uid] = client_session
		--��¼�ɹ�������������uid���ײ�session����
		print("client_session_uid uid="..login_uid)
		session_wrapper.set_uid(client_session,login_uid)
		--��������Ϊ0,��Ҫ��Ϊ�˲���¶uid���ͻ���
		t_body.userinfo.uid = 0
		--���ص�¼�����userinfo��Ϣ��ǰ��
		print("t_body is")
		utils.print_table(t_body)
		local ret_msg = {stype=stype_module.AuthSerser,ctype=cmdtype,utag=0,body=t_body}
		session_wrapper.send_msg(client_session,ret_msg)
		return
	end

	--���ݵ�¼ǰ�����ǵ�½���ȡclient_session
	if client_session_utag[utag] ~= nil then
		print("client_session_utag")
		client_session = client_session_utag[utag]
	elseif client_session_uid[utag] ~= nil then
	    print("client_session_uid")
		client_session = client_session_uid[utag]
	else 
	    print("not fonud client session")
	end

	--��Э�����utag����Ϊ0 
	proto_mgr_wrapper.set_raw_utag(raw_data,0)
	if client_session ~= nil then
	   --ת�����ݸ��ͻ���
	   --print("send_raw_msg client session")
	   session_wrapper.send_raw_msg(client_session,raw_data)
	else
		print("send_to_client: not found clcient session")
	end
end

-- raw_data��C++���Ǹ����͵��������ݰ�
--���ؿ����յ�2��session��������
--���Կͻ��ˣ���Ҫ����stypeת������Ӧ��session
--���Է�����������utag����uidת�����ͻ��˶�Ӧ��session
function on_gw_recv_raw_cmd(s, raw_data)
	print("on_gw_recv_raw_cmd")
	is_client_session = session_wrapper.is_client_session(s)
	if is_client_session==0 then 
	   --���Կͻ�������
	   --print("send_to_server")
	   send_to_server(s,raw_data)
	else
	   --���Է�����
	  --print("send_to_client")
	   send_to_client(s,raw_data)
	end
end

--session�Ͽ��ص�����
function on_gw_session_disconnect(s,service_stype) 
	print("on_gw_session_disconnect!!")
    --�������������������������session
	if session_wrapper.is_client_session(s)==1 then
	--�������ӵ�session�Ͽ��ص�����
		--print(s)
		for k,v in pairs(session_map) do
			--print(v)
			if v == s then
				--print("disconnect server["..config.servers[k].desic.."]")
				session_map[k] = nil
				session_connecting[k] = false
				return
			end
		end
		return
	end
	--print("on_gw_session_disconnect BBB!!")
	--�ͻ������ӵ����ص�session,�����ǵ�¼ǰ
	local utag = session_wrapper.get_utag(s)
	--print("on_gw_session_disconnect BBB!!"..utag)
	if client_session_utag[utag] ~= nil  and client_session_utag[utag] == s then
	   print("client_session_utag[utag] remove!!")
	   client_session_utag[utag] = nil --��仰�ܱ�֤utag��Ӧ�����ݱ�ɾ��������ʹ��remove
	   session_wrapper.set_utag(s,0)
	end

	--�ͻ������ӵ����أ��Ѿ��ǵ�¼��
	local uid = session_wrapper.get_uid(s)
	if client_session_uid[uid] ~= nil and client_session_uid[uid] == s then
	   print("client_session_uid[uid] remove!!")
	   client_session_uid[uid] = nil
	end

	--�ͻ��˶��ߣ����ظ���֪ͨ������������
	local server_session = session_map[service_stype]
	if server_session == nil then
		return
	end
	
	if uid~=0 then
	  --�㲥�û����ߵ���Ϣ�����������ӵ�����
	  local ret_msg = {stype=service_stype,ctype=cmd_module.UserLostConn,utag=uid,body=nil}
	  session_wrapper.send_msg(server_session,ret_msg)
	end
end

local gw_service = {
	on_session_recv_raw_cmd = on_gw_recv_raw_cmd,
	on_session_disconnect = on_gw_session_disconnect,
}

server_session_init()

return gw_service