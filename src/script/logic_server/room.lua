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
local PLAYER_NUM_3v3 = 3
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

	print("room:init(zid)")
end

function room:enter_room(p)

	print("room:enter_room start")
    if p == nil then
	   print("enter p is null")
	   return false
	end 
	--�жϷ�������״̬�Ƿ�Ϸ�
	print("room_status:"..self.room_state.."pstatus"..p.status)
	if self.room_state ~= status.InView or p.status ~= status.InView then 
	    print("room:enter_room return false")
		return false
	end

	table.insert(self.inview_players,p)
	p.roomid = self.room_id
	--֪ͨ������ȴ�����ң��ҽ�����
	--[[local ret_msg = {
	                  --���ﴫstype_module.LogicServer���е�����,����Ƕ����򻮷֣�����Ͳ���ֱ�Ӵ���
				      stype=stype_module.LogicServer,ctype=cmd_module.EnterPlayNotify,utag=uid,
				      body={
							
	}}]]
                  
    local body = { 
		zid = self.zid,
		roomid = self.room_id
	}  
	--�����Ϣ��֪ͨ���Լ������뷿��ȴ��б��ص���Ϣ
	 p:send_cmd(stype_module.LogicServer,cmd_module.EnterPlayNotify,body)	 
	
	--֪ͨ�����ڵȴ��������û�����Ҳ����ȴ�״̬
	msgbody = p:get_user_arrived()
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.EnterArriveNotify, msgbody, p)
	
	--�Ѿ��ڷ�������ҵ����ݣ�֪ͨ���ս��뷿������
	for i = 1, #self.inview_players do 
		if self.inview_players[i] ~= p then 
			local body = self.inview_players[i]:get_user_arrived()
			p:send_cmd(stype_module.LogicServer, cmd_module.EnterArriveNotify, body)
		end
	end

	-- �ж����ǵ�ǰ�Ƿ񼯽���ҽ�����
	if #self.inview_players >= PLAYER_NUM_3v3 * 2 then 
	   self.room_state = status.Ready
	   	for i = 1, #self.inview_players do 
			self.inview_players[i].state = status.Ready
		end
	end
	print("room:enter_room end")
	return true
end

function room:broadcast_cmd_inview_players(cstype, cctype, cbody, not_to_player)
	local i 
	for i = 1, #self.inview_players do 
		if self.inview_players[i] ~= not_to_player then 
			self.inview_players[i]:send_cmd(cstype, cctype, cbody)
		end
	end
end


return room

