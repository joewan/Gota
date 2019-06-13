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
	 
	self.inview_players = {}  -- �Թ���ҵ��б�
	self.lhs_players = {}     -- �������ߵ����
	self.rhs_players = {}     -- �������ߵ����
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
		   print("random heroid:"..p.heroid.." uid:"..p.uid)
		   break
		end 
	end
	p.roomid = self.room_id
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
	   	local k,v
		for k,v in pairs(self.inview_players) do 
		    if v ~= nil then
			   v.state = status.Ready
			end
		end

		--ȫ����read״̬�󣬷��Ϳ�ʼ��Ϸ֪ͨ���������û�
		self:game_start()
	end
	--print("room:enter_room end")
	return true
end

--ȫ����read״̬�󣬷��Ϳ�ʼ��Ϸ֪ͨ���������û�
--����ȱ�ٸ��û�ѡhero�Ĳ���,ֱ�Ӹ��û����ѡ��[1-5]һ��Ӣ�۽�ɫ 
function room:game_start()
	local heroes = {}
	for i = 1, PLAYER_NUM_3v3 * 2 do
	    if self.inview_players[i] ~= nil then
		   table.insert(heroes, self.inview_players[i].heroid)
		end
	end

	local msgbody = {
		heroes = heroes,
	}
	
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.GameStartNotify, msgbody, nil)
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

