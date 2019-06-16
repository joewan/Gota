--����ģ��
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")
local status = require("logic_server/room_status")
--��ǰ�ż�ȫ��id
local g_room_id = 1
--����һ�ߵĲ�������,һ�ֵ���������*2
local PLAYER_NUM_3v3 = 2
local PLAYER_NUM_5v5 = 5

--����һ��room������
local room = {}

function room:new(instant)
	if not instant then
	   instant = {}
	end

	--�ϲ�Ԫ��
	setmetatable(instant,{__index=self})
	return instant
end

function room:init(zid)

    self.zid = zid
	self.room_id = g_room_id
	roomid = g_room_id + 1
	self.room_state = status.InView
	self.frameid = 0          --��ǰ֡id
	self.inview_players = {}  -- �Թ���ҵ��б�
	self.lhs_players = {}     -- �������ߵ����
	self.rhs_players = {}     -- �������ߵ����

	self.all_frame_list = {}  --������ʼ�������е�֡���� 
	self.next_frame_list = {}  --��ǰframeid��Ӧ��֡����(��������Ҫ���ܵ���һ֡)

end

function room:exit_room(p)
	
	if p == nil then
	   print("exit p is null")
	   return false
	end
    
	--�㲥�����û����˳�������
	local msgbody = {
	    seatid = p.seatid,
	}
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.UserExitRoomNotify, msgbody, p)

	--�ȴ��Լ��ȴ�ƥ���б��Ƴ�
	if self.inview_players[p.seatid] ~= nil then
	   self.inview_players[p.seatid] = nil   
	end
	
	--����״̬�� ֪ͨ��ң��˳��ɹ�
	p.zid = -1;
	p.matchid = -1
	p.seatid = -1
	p.side = -1
	local body = {status = res_module.OK}
	p:send_cmd(stype_module.LogicServer, cmd_module.ExitRoomRes, body)
	
end

--��ҽ���żٴ�����
function room:enter_room(p)

    if p == nil then
	   print("enter p is null")
	   return false
	end 
	
	--�жϷ�������״̬�Ƿ�Ϸ�
	--print("room_status:"..self.room_state.."pstatus"..p.status)
	if self.room_state ~= status.InView or p.status ~= status.InView then 
	    print("room:enter_room return false")
		return false
	end

	--table.insert(self.inview_players,p)
	--����һ�����е�seatid
	local i
	for i = 1,PLAYER_NUM_3v3*2 do
	    if self.inview_players[i] == nil then
		   self.inview_players[i] = p
		   p.seatid = i
		   --���ö�����Ϣ
		   if i < PLAYER_NUM_3v3  then
		      p.side = 0
		   else
		      p.side = 1
		   end

		   --���û����һ��heroid
		   p.heroid = math. floor(math.random()*5 + 1)
		   --print("random heroid:"..p.heroid.." uid:"..p.uid)
		   break
		end 
	end
	p.roomid = self.room_id
	p.cur_sync_frame_id = 1
	--֪ͨ������ȴ�����ң��ҽ�����
	--[[local ret_msg = {
	                  --���ﴫstype_module.LogicServer���е�����,����Ƕ����򻮷֣�����Ͳ���ֱ�Ӵ���
				      stype=stype_module.LogicServer,ctype=cmd_module.EnterPlayNotify,utag=uid,
				      body={
							
	}}]]
                  
    local body = { 
		zid = self.zid,
		roomid = self.room_id,
		seatid = p.seatid,
		side = p.side,
	}  
	--�����Ϣ��֪ͨ���Լ������뷿��ȴ��б��ص���Ϣ
	 p:send_cmd(stype_module.LogicServer,cmd_module.EnterPlayNotify,body)	 
	
	--֪ͨ�����ڵȴ��������û�����Ҳ����ȴ�״̬
	msgbody = p:get_user_arrived()
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.EnterArriveNotify, msgbody, p)
	
	--�Ѿ��ڷ�������ҵ����ݣ�֪ͨ���ս��뷿������
	local k,v
	--for i = 1, #self.inview_players do
	  for k,v in pairs(self.inview_players) do 
		--�����ݲ��Ҳ��Ǹոս������ң��ͷ���֪ͨ
		if v ~= nil and v ~= p then 
			local body = v:get_user_arrived()
			p:send_cmd(stype_module.LogicServer, cmd_module.EnterArriveNotify, body)
		end
	end

	-- �ж����ǵ�ǰ�Ƿ񼯽���ҽ�����
	if #self.inview_players >= PLAYER_NUM_3v3 * 2 then 
	    print("room is read!!!")
	    self.room_state = status.Ready
		--�������״̬
	    self:update_play_status(status.Ready)

		--ȫ����read״̬�󣬷��Ϳ�ʼ��Ϸ֪ͨ���������û�
		self:game_start()
	end
	--print("room:enter_room end")
	return true
end

function room:update_play_status(status)
	local k,v
	for k,v in pairs(self.inview_players) do 
		if v ~= nil then
		   v.status = status
	    end
	end

end

--ȫ����read״̬�󣬷��Ϳ�ʼ��Ϸ֪ͨ���������û�
--����ȱ�ٸ��û�ѡhero�Ĳ���,ֱ�Ӹ��û����ѡ��[1-5]һ��Ӣ�۽�ɫ 
function room:game_start()
	--local heroes = {}
	local players_match_info = {}
	for i = 1, PLAYER_NUM_3v3 * 2 do
	    if self.inview_players[i] ~= nil then
		   local p = self.inview_players[i]
		   local info = {
			heroid = p.heroid,
			seatid = p.seatid,
			side = p.side,
		}

		table.insert(players_match_info, info)
		--table.insert(heroes, self.inview_players[i].heroid)
		end
	end
	--������ȫ�����ƥ����Ϣ
	local msgbody = {
	      players_match_info = players_match_info,
	}
	
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.GameStartNotify, msgbody, nil)
	--֪ͨ��Ϸ��ʼ��״̬����ΪStart
	self.room_state = status.Playing
	self:update_play_status(status.Playing)

	--�ӵ�һ֡��ʼ
	self.frameid = 1
    self.all_frame_list = {}  --������ʼ�������е�֡���� 
	self.next_frame_list = {frameid=self.frameid,opts = {}}  --��ǰframeid��Ӧ��֡����(�����ܵ���һ֡)
	--�ͻ������յ�GameStartNotify��Ϣ��Ὺʼloading������Դ
	--�ȴ�3s�󣬿�ʼ֡ͬ����20fps,ÿ�μ������50ms(����һ�����50ms��ʱִ�еĶ�ʱ��)
	--ע������create_timer��Ҫ���һ���������������������������ڵ������Ա���� 
	self.frame_time = timer_wrapper.create_timer(function()
           self:do_logic_frame_sync()
		   end,-1,3000,50)

end

function room:push_next_frame(next_frame_opts)
    local seatid = next_frame_opts.seatid
	--print(seatid, next_frame_opts.frameid, #next_frame_opts.opts)

	local p = self.inview_players[seatid]
	if not p then 
		return
	end

	--g����play��ǰͬ��֡
	if p.cur_sync_frame_id <  next_frame_opts.frameid - 1 then
	   p.cur_sync_frame_id = next_frame_opts.frameid - 1
	end

	--�жϵ�ǰ�����֡�Ƿ��Ǻͷ��������յ�frameidƥ��
    if  next_frame_opts.frameid ~= self.frameid then
	    print("next_frame_opts.frameid ~= self.frameid")
		return
	end

	for i = 1, #next_frame_opts.opts do 
		table.insert(self.next_frame_list.opts, next_frame_opts.opts[i])
	end

end

--����Ϊͬ����֡
function room:send_unsync_frame(p)
     if p == nil then
	    return 
	 end
	
	 if #self.all_frame_list == 0 then
	    return
	 end
	 --�����ͬ��֡(ֻͬ��δͬ����֡����)
	 local unsync_opt_frame = {}  --�洢��ǰδͬ����֡ 
     local i
	 --��play�ĵ�ǰ֡��room�����֡ͬ�������
	 for i = (p.cur_sync_frame_id + 1),#self.all_frame_list do
	      table.insert(unsync_opt_frame, self.all_frame_list[i])
	 end
	 --�����û�֡frameid??
	 --
	 --p.cur_sync_frame_id = self.frameid
	 if #unsync_opt_frame > 0 then
	     local body = {frameid = self.frameid,unsync_frames = unsync_opt_frame}
         p:udp_send_cmd(stype_module.LogicServer, cmd_module.LogicFrame,body)      
	 end
    
end

function room:do_logic_frame_sync()
    --��һ֡�����ݣ��洢��all_frame_list
	table.insert(self.all_frame_list,self.next_frame_list)

	--�����ȫ����ң�����һ֡�����ݷ��ͳ�ȥ
	local k,p
	for k,p in pairs(self.inview_players) do 
		if p then
		  self:send_unsync_frame(p)
		end
	end

	--��������ǰ֡��Ҳ����Ҫ���ܵ���һ��frameid
	self.frameid = self.frameid + 1
	--��һ֡���ݸ��ǵ���һ֡
	self.next_frame_list = {frameid = self.frameid,opts = {}}
	--print("do_logic_frame_sync self.frameid: "..self.frameid)
end

function room:broadcast_cmd_inview_players(cstype, cctype, cbody, not_to_player)
	local k,v 
	--for i = 1, #self.inview_players do 
	for k,v in pairs(self.inview_players) do 
		if v ~= not_to_player then 
			v:send_cmd(cstype, cctype, cbody)
		end
	end
end


return room

