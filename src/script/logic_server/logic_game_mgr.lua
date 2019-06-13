local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")
local room_moduel = require("logic_server/room")
local room_status = require("logic_server/room_status")

local mysql_game = require("database/mysql_system_ugame")
local redis_game = require("database/redis_game")
--���ķ���洢�ӿ�
--���ķ���洢�ӿ�
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local robot_player = require("logic_server/robot")
--uid��player�Ķ�Ӧ��ϵ
local online_player_map = {}
local online_player_num = 0

--ƥ���б�zone_wait_list
-- zone_wait_list[Zone.SGYD] = {} --> uid --> p;
--ÿ����ͼid��Ϊkey, value�������һ����match
--match = {uid,player}
local zone_wait_list = {} 

--ƥ��ɹ����������б�,Ҳ�ǰ���zid������
--room_list[zid]��ȡȫ��zid��ͼ�ķ����б�list  ���ʽ: {zid,{roomid,room}}
local room_list = {}
room_list[zone.SGYD] = {}
room_list[zone.ASSY] = {}

--�������б�,Ҳ�ǰ��յ�ͼ������ ���ʽ{zid,{uid,borot}}
local zone_robot_list = {}
zone_robot_list[zone.SGYD] = {}
zone_robot_list[zone.ASSY] = {}

local function do_new_robot_play(robotlist)
  
	if #robotlist <=0 then
	   return
	end

	--���ݵ�ͼ������robot
	local half_len = #robotlist
	local i = 1
	half_len = math.floor(half_len * 0.5)
	print("half_len:"..half_len)
	--����robot����
    --for i = 1, half_len do 
	--�Ȳ����֣���ȫ��robot�ŵ�һ����ͼ
	for i = 1, #robotlist do 
	    local v = robotlist[i] 
		local robot = robot_player:new()
		robot:init(v.uid,nil,nil)
		robot.zid = zone.SGYD
		zone_robot_list[zone.SGYD][v.uid] = robot
		
	end
	--[[
	   for i = half_len + 1, #robotlist do 
	    local v = robotlist[i] 
		local robot = robot_player:new()
		robot:init(v.uid,nil,nil)
		robot.zid = zone.ASSY
		zone_robot_list[zone.ASSY][v.uid] = robot
		print("zone_robot_list[zone.ASSY][v.uid] = robot")
	end
	]]
	

end

function do_load_robot_uinfo(now_index, robots)
    mysql_center.get_userinfo_by_uid(robots[now_index].uid, function (err, uinfo)
    if err or not uinfo then
	        print("get_userinfo_by_uid error"..err)
			return 
	end

	redis_center.set_userinfo_to_redis(robots[now_index].uid, uinfo)
	--print("uid " .. robots[now_index].uid .. " load to center reids!!!")
	now_index = now_index + 1
	if now_index > #robots then
	   --������˵��ȫ��robot���ݼ������
	   --����robot���󣬲��洢��zone_robot_list�б�
	   do_new_robot_play(robots)
	   return 
	end

	do_load_robot_uinfo(now_index, robots)
	end)
end

local function search_idle_robot(zid)
	local robots = zone_robot_list[zid]
	local k, v 
	for k, v in pairs(robots) do 
		if v.roomid == -1 then
			return v 
		end
	end

	return nil
end

--���ڶ�ʱ������������robot���뵽������
function do_push_to_match()
   
	--����ȫ����inview״̬�µķ���,Ȼ�����ҵ�һ��robot������ӽ�����
    local zid, zid_room_list
	local k, room  
	for zid, zid_room_list in pairs(room_list) do 
	    
		for k, room in pairs(zid_room_list) do
			if room.room_state == room_status.InView then  -- �ҵ���һ�����е�room
				--�ҵ�һ�����Լ����robot
				--print("room state is InView zid"..zid)
				local robot = search_idle_robot(zid)
				if robot then
					print("[".. robot.uid .."]" .. " enter match!")
					robot.zid = zid
					room:enter_room(robot) 
				end
			end 
		end
	end

end

--����һ��1sִ��һ�εĶ�ʱ���� ���������Ҫrobotģʽ
timer_wrapper.create_timer(do_push_to_match,-1,2000,1000)

function do_load_robot_ugame_info()
 
	mysql_game.get_robots_ugame_info(function(err, ret)
	    if err then
		    print("get_robots_ugame_info err"..err)
			return 
		end

		--û������
		if not ret or #ret <= 0 then 
			return
		end

		--��������ӵ�redis
		local k, v
		for k, v in pairs(ret) do
			redis_game.set_ugame_info_inredis(v.uid, v)
		end

		--���û�������Ϣ���õ�game����
		do_load_robot_uinfo(1, ret)
	end)
end

function load_robots()
    --print("load_robots")
	if not mysql_game.is_connectd() or 
	   not mysql_center.is_connectd() or 
	   not redis_center.is_connectd() or not redis_game.is_connectd() then 
	       --print("create_timer_once")
		   --����һ��һ���Զ�ʱ����
	       timer_wrapper.create_timer_once(load_robots,1000,1000)
	       return
	end

	--��ȡrobot����
	do_load_robot_ugame_info()
end

load_robots()


--�ҵ�һ��״̬��InView�ķ���
local function search_inview_match_mgr(zid)
    if zid < 0 then
	   return nil
	end
	
	--��ȡzid��ͼȫ��list
	zid_room_list = room_list[zid]
	if zid_room_list == nil then
	    print("search_inview_match_mgr zid_room_list is nil")
	    return nil
	end
    
	local k,v
	for k,v in pairs(zid_room_list) do
	   if v.room_state == room_status.InView then
	      return v
	   end

	   --�жϵ�ǰ�����Ƿ�����
	end
	--�Ҳ���,����һ����room���󷵻�
	local croom = room_moduel:new()
	croom:init(zid)
	table.insert(zid_room_list,croom)
	return croom
end

--ƥ�䶨ʱ���������жϵȴ��б�zone_wait_listƥ���߼�
--һ����ʱ�����ú�������һ����ͼ��ƥ��
function do_match_sgyd_map()
 
	--�ȴ�zone.SGYD��ͼ��ȫ������б�
	if #zone_wait_list == 0  then
	   return
	end

	local zid, wait_list
	--�������еĵȴ��б�
	for zid, wait_list in pairs(zone_wait_list) do 
	    local k,v
	    for k,v in pairs(wait_list) do
	        --k:uid v:player
	        local room =  search_inview_match_mgr(zid)
	        if room then
	           if not room:enter_room(v) then
			      --����
				  print("room:enter_room error! v.status:"..v.status)
			   else
			      --����ɹ����ӵȴ��б��Ƴ�
				  print("add success wait_list remove! uid:"..k)
				  zone_wait_list[zid][k] = nil
			   end
		    else
			   print("room is nil")
	        end
	    end
	end

end


--1sһ��ƥ��,ע����һ����Ҫ�ŵ�������������
init_count = 0
if init_count==0 then
   timer_wrapper.create_timer(do_match_sgyd_map,-1,1000,1000)
   init_count = init_count + 1
end


local function send_logic_enter_status(s,uid,sstype,sstatus)	
 
  local ret_msg = {
				   stype=sstype,ctype=cmd_module.LoginLogicRes,utag=uid,
				   body={
							status = sstatus
					    }}
                    
					--utils.print_table(ret_msg)
			        session_wrapper.send_msg(s,ret_msg)
end

--������Ϸ�߼�������
function login_server_enter(s,msg)
  
   local uid = msg[3]
   --[[
   	 logic��Ϊ��ս��ͼ��������������������
	 1.һ��logic������һ�������Ķ�ս������������֧��N����ͬ�ĵ�ͼ��N�ֲ�ͬ���淨
	 2.�Ѳ�ͬ�ĵ�ͼ���ֵ���ͬ��logic_server�ϣ�Ȼ��ʹ��ÿ��logic��stype��ͬ������Ϊ���صķ�ʽ
   ]]
   --�ͻ��˽����stype
   local stype = msg[1] 
   print("login_server_enter uid:"..uid.."stype"..stype)
   
   local play = online_player_map[uid]
   if play ~= nil then
      --����session,�����session�ǿͻ��˺�gateway�����Ӷ���
      play:set_session(s)
	  send_logic_enter_status(s,uid,stype,res_module.OK)
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
	        if online_player_map[uid] == nil then
			   online_player_map[uid] = play
			   online_player_num = online_player_num + 1
			   print("...online_player_num:"..online_player_num)
			end
		end
		send_logic_enter_status(s,uid,stype,status)
   end)
end

--���ع㲥�������û�������Ϣ
function on_player_disconnect(s,msg)
    local uid = msg[3]
	print("on_player_disconnect uid:"..uid)
	
	play = online_player_map[uid]
	if play == nil then
	   return
	end

	--�ж�����Ƿ��ڵȴ��б�
	if play.zid ~= -1 then
	   --�Ƴ��ȴ��б�
	   zone_wait_list[play.zid][uid] = nil
	   play.zid = -1
	end
	--�Ȳ����Ƕ���������ֱ������online_player_mapɾ��
	if online_player_map[uid] ~= nil then
	   print("on_player_disconnect online_player_map remove uid:"..uid)
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
	--print("on_gateway_connect stype"..stype)
	 for k, v in pairs(online_player_map) do 
		v:set_session(s)
	end

end

function logic_enter_zone(s,msg)
    local stype =  msg[1]
	local uid = msg[3]
	local zid = msg[4].zid
	print("logic_enter_zone stype:"..stype.." uid:"..uid.." zid:"..zid)
	if uid <= 0 then
	     local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--�жϽ���ĵ�ͼ�Ƿ�Ϸ�
	if zid ~= zone.SGYD and zid ~= zone.ASSY then
		 local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		return
	end

	play = online_player_map[uid]
	--�Ҳ��������Ѿ��ڵ�ͼ�У��޷��ڽ���
	if play == nil or play.zid ~= -1 then
	    print("play.zid:"..play.zid)
	    local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvalidOptErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--��һ�����һ���յ��б�
	if not zone_wait_list[zid] then
	   zone_wait_list[zid] = {}
	end
	--���play����ƥ���б�
	play.zid = zid
	zone_wait_list[zid][uid] = play
	print("zone_wait_list[zid][uid] zid:"..zid.." uid:"..uid)
	local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.ok
					    }}
                    
    session_wrapper.send_msg(s,ret_msg)

end

local game_mgr = {
	login_server_enter = login_server_enter,
	on_player_disconnect = on_player_disconnect,
	on_gateway_disconnect = on_gateway_disconnect,
	on_gateway_connect = on_gateway_connect,
	logic_enter_zone = logic_enter_zone,
}

return game_mgr