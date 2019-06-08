local config = require("conf")
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")

--stype���������ӵ�session��sessionӳ��,�洢�����ӵķ�������session
local session_map = {}
--����������ӻ�δ�ɹ���session
local session_connecting = {}

local g_ukey = 1;
--��¼ǰ�洢�û���session�����key��ȫ�ֱ���g_ukey
local client_session_utag = {}
--��½��ʹ�ã���½���ȡuid��ʹ��uid��Ϊ���key
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

--����ʱ������
function check_session_connect()
	--����������Ҫ���ӵķ�����
	for k,v in pairs(config.servers) do
	    --�ж� ���û�����ӳɹ�������Ҳ�����������ӵ�server���ͷ���һ�����ӵ���
		if session_map[v.stype] == nil and session_connecting[v.stype] == false then
			session_connecting[v.stype] = true
			print("connecting server["..v.desic.."]"..v.ip..":"..v.port)
			connect_to_server(v.stype,v.ip,v.port)
		end
	end
end

function server_session_init()
    --��ȡconf.lua��gateway��Ҫ���ӵ�servers����
	for k,v in pairs(config.servers) do
		--��ѭ���г�ʼ��2��map
		session_map[v.stype] = nil
		session_connecting[v.stype] = false
	end
	--�������Ӷ�ʱ�������ÿ1s����check_session_connect
	timer_wrapper.create_timer(check_session_connect,-1,1000,1000)
end

--local socket_type = WEB_SOCKET
--local proto_type =  PROTO_JSON
function is_login_request(cmd_type)
	if cmd_type == cmd_module.GuestLoginReq or 
	   cmd_type == cmd_module.UnameLoginReq then 
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
	  print("is_login_request cmd"..cmd)
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
function is_loginresp_ctype(ctype)
	if ctype == cmd_module.GuestLoginRes or 
	   ctype == cmd_module.UnameLoginRes then
		return true
	end
	return false
end

--����������������Ϣ��ת����Ӧ�Ŀͻ���
function send_to_client(server_session,raw_data)
	local cmdtype, ctype, utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print("send_to_client".." cmdtype:"..cmdtype.." ctype:"..ctype.." utag:"..utag)
	local client_session = nil
	
	--�ж��Ƿ�Ϊ��¼����Э��
	--print("send_to_client ctype,"..ctype)
	--ctype��Э��id,�ж��Ƿ�Ϊ��¼����Э��
	if is_loginresp_ctype(ctype) == true then
	    print("is_login_ctype ctype:"..ctype)
		--��¼Э�鷵�أ��������ȡ��֤���������ص�uid
		local t_body = proto_mgr_wrapper.read_msg_body(raw_data)
		if t_body == nil then
		   print("t_body is nil")
		   return
		end

		utils.print_table(t_body)
		--client_session_utag�ڵ�¼ǰ������,
		--client_session_utag[utag] = client_session�Ķ�Ӧ��ϵ
		client_session = client_session_utag[utag]
		print("is_login_ctype utag:"..utag)
		if client_session==nil then
			--�����ȡ��������һ���쳣
			print("client_session is nil")
			return
		end
		--�жϵ�¼��Ϣ�Ƿ�ɹ�
		if t_body.status ~= res_module.OK then
		   proto_mgr_wrapper.set_raw_utag(raw_data,0)
		   if client_session ~= nil then
			  session_wrapper.send_raw_msg(client_session,raw_data)
			end
		end
		
		--�����ǵ�¼�ɹ��߼�
		
		local t_userinfo = t_body.userinfo
		--�û�uid,uid���ڵײ㴴����
		local login_uid = t_userinfo.uid
		print("login_uid"..login_uid)
		if login_uid~= 0 then 
		   --�ж��Ƿ���session�Ƿ��Ѿ���¼
		   if client_session_uid[login_uid] ~= nil and client_session_uid[login_uid] ~= client_session then
		       print("Relogin uid is"..login_uid)
		       --����һ���ظ���¼��Ϣ
			  local ret_msg = {stype=stype_module.AuthSerser,ctype=cmd_module.ReLoginRes,utag=0,body=nil}
			  session_wrapper.send_msg(client_session,ret_msg)
			   --�ȹرյײ�session��ɾ���Ѿ����ڵ�session
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
		local ret_msg = {stype=stype_module.AuthSerser,ctype=ctype,utag=0,body=t_body}
		utils.print_table(t_body)
		session_wrapper.send_msg(client_session,ret_msg)
		return
	end

	
	
	--�ڵ�¼�ɹ��Ժ�Э��ͷ��utag�����û���uid
	client_session = client_session_uid[utag]
	if client_session ~= nil then
	   --ת�����ݸ��ͻ���
	   --��Э�����utag����Ϊ0 ,���Ⱪ¶uid���ͻ���
	   proto_mgr_wrapper.set_raw_utag(raw_data,0)
	   session_wrapper.send_raw_msg(client_session,raw_data)

	   --�ж��Ƿ�Ϊ�˳���ϷЭ�飬�ǵĻ�Ҫ��client_session_uid��Ӧ�Ŀͻ���session�����
	   if ctype == cmd_module.LoginOutRes then
	      session_wrapper.set_uid(client_session,0)
	      client_session_uid[utag] = nil

		  --�û��˳�����Ҫ�������������û��˳�����Ϣ�������ﴦ��

	   end
	else
		print("send_to_client: not found clcient session")
	end
end

-- raw_data����C++�ײ����͵�����ԭʼ���ݰ�
--���ؿ����յ�2��session��������
--���Կͻ��ˣ���Ҫ����stypeת������Ӧ��session
--���Է�����������utag����uidת�����ͻ��˶�Ӧ��session
function on_gw_recv_raw_cmd(s, raw_data)
	--print("on_gw_recv_raw_cmd")
	--���ж�session����
	is_client_session = session_wrapper.is_client_session(s)
	if is_client_session==0 then 
	   --���Կͻ������ݣ�����Э�����stype��ת��
	   --print("send_to_server")
	   send_to_server(s,raw_data)
	else
	   --���Է���������Ϣ��ת�����ͻ���,����Э�����utag/uid��ת��
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
	   print("client_session_uid[uid] remove!! uid"..uid)
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

--��������
local gw_service = {
    --�ײ��յ����ݺ�������ע�ắ��
	on_session_recv_raw_cmd = on_gw_recv_raw_cmd,
	--session���ߵײ����(�ͻ���session��server��session���ᱻ����)
	on_session_disconnect = on_gw_session_disconnect,
}

--ģ�鱻���غ󣬺����ᱻ����
server_session_init()

return gw_service