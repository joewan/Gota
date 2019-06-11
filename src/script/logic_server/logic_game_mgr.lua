local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")

--uid��player�Ķ�Ӧ��ϵ
local online_player_map = {}
local online_player_num = 0

local function send_logic_enter_status(s,uid,sstatus)	
 
  local ret_msg = {
				   stype=stype_module.LogicServer,ctype=cmd_module.LoginLogicRes,utag=uid,
				   body={
							status = sstatus
					    }}
                    
					--utils.print_table(ret_msg)
			        session_wrapper.send_msg(s,ret_msg)
end

--������Ϸ�߼�������
function login_server_enter(s,msg)
   local uid = msg[3]
   print("login_server_enter uid:"..uid)
   
   local play = online_player_map[uid]
   if play ~= nil then
      --����session,�����session�ǿͻ��˺�gateway�����Ӷ���
      play:set_session(s)
	  send_logic_enter_status(s,uid,res_module.OK)
	  return
   end

   --û���ҵ�������һ��playerʵ��
   play = player:new()
   if play == nil then
      print("player:new() is error")
	  return
   end

   --��db��ȡplayer����
   play:init(uid, s, function(status)
      if status == res_module.OK then
			online_player_map[uid] = play
			online_player_num = online_player_num + 1
			print("online_player_num:"..online_player_num)
		end
		send_logic_enter_status(s,uid,status)
   end)
end

--���ع㲥�������û�������Ϣ
function on_player_disconnect(s,msg)
    local uid = msg[3]
	print("on_player_disconnect uid:"..uid)
	--�Ȳ����Ƕ���������ֱ������online_player_mapɾ��
	if online_player_map[uid] ~= nil then
	   online_player_map[uid] = nil
	   online_player_num = online_player_num -1
	   if online_player_num < 0 then
	      online_player_num = 0
	   end  
	   print("on_player_disconnect online_player_num:"..online_player_num)
	end

end

--��gateway���ط������
--�����ضϿ��󣬻������ز෢����������
function on_gateway_disconnect(s,ctype)
	local k, v
	--����ֻɾ��player������洢��session,online_player_map�����
	--2��ԭ��:
	--1.����ͻ��˲�û�к����ضϿ���Ҳ����˵client��gateway�������Ǻõģ�
	--2.�ں�gateway�Ͽ�������Ҫ����ȥ�������صģ�������ӳɹ����ڰ�client��gateway
	--���Ӻõ�session�������õ�player����Ϳ�����
    for k, v in pairs(online_player_map) do 
		v:set_session(nil)
	end

end

--gateway���ӳɹ����������������֪ͨ��������service
function on_gateway_connect(s,stype)
	print("on_gateway_connect stype"..stype)
	 for k, v in pairs(online_player_map) do 
		v:set_session(s)
	end

end

local game_mgr = {
	login_server_enter = login_server_enter,
	on_player_disconnect = on_player_disconnect,
	on_gateway_disconnect = on_gateway_disconnect,
	on_gateway_connect = on_gateway_connect,
}

return game_mgr