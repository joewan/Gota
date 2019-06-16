local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local mysql_game = require("database/mysql_system_ugame")
local redis_center = require("database/redis_auth_center")
local room_status = require("logic_server/room_status")

--����һ������player��
local player = {}

function player:new(instant) 
	if not instant then 
		instant = {} --���ʵ��
	end

	setmetatable(instant, {__index = self}) 
	return instant
end


function player:init(uid, s, ret_handler)
	self.session = s
	self.uid = uid
	--zid��ҽ���ĵ�ͼ����ʼ��Ϊ-1,��ʾΪ�����κε�ͼ��
	self.zid = -1
	--����Ψһ��ʶ��
	self.roomid = -1
	--ƥ�����Ϊid
	self.seatid = -1
	--������ڵĶ��� 0��߻�1�ұ�
	self.side= -1
	--���ѡ�õ�Ӣ�� ��ʱȡֵ[1-5]
	self.heroid = -1
	--�û��ڷ������״̬
	self.status = room_status.InView
	--�Ƿ�Ϊ���������
	self.is_robot = false
	self.client_udp_ip = nil
	self.client_udp_port = 0
	--��ǰͬ������һ֡
	self.cur_sync_frame_id = 0   --��ǰͬ������һ֡
	-- ���ݿ������ȡ��ҵĻ�����Ϣ;
	mysql_game.get_ugame_info(uid, function (err, ugame_info)
		if err then
		    if ret_handler then
			   ret_handler(res_module.SystemErr) 
			end
			return
		end

		self.ugame_info = ugame_info
		--ret_handler(res_module.OK) 

	    -- ...������Ϣ�����ٶ�
	    --redis��ȡ�û���Ϣ ��Ҫ��ȡunkci,uface,usex
	    redis_center.get_userinfo_to_redis(self.uid,function(err,user_uinfo)
	    if err ~= nil then
		   print("get_userinfo_to_redis err:"..err)
		   if ret_handler then
		     ret_handler(res_module.SystemErr)
		   end	  
		   return
		end

		--print("play read redis unick:"..user_uinfo.unick.." uface:"..user_uinfo.uface)
		self.uinfo = user_uinfo
		if ret_handler then
		   ret_handler(res_module.OK)
		end 
    end)
 end)
	-- end
end

function player:set_udp_addr(ip,port)
	if ip == nil or port<=0 then
	   print("set_udp_addr invalid parament")
	   return
	end

	self.client_udp_ip = ip
	self.client_udp_port = port
	--print("------------player:set_udp_addr ip:"..self.client_udp_ip.." port:"..self.client_udp_port)
end

function player:set_session(s)
	self.session = s
end

function player:send_cmd(sstype, cctype, cbody)
	if not self.session or self.is_robot==true then 
		return
	end

	local ret_msg = {stype = sstype,ctype = cctype,utag = self.uid, body=cbody}
	--utils.print_table(ret_msg)
    session_wrapper.send_msg(self.session,ret_msg)
end

function player:udp_send_cmd(sstype, cctype, cbody)
	if not self.session or self.is_robot==true then 
		return
	end

	if self.client_udp_ip == nil or self.client_udp_port == 0 then
	    --print("client_udp_ip nil or client_udp_port==0")
	    return
	end
	local ret_msg = {stype = sstype,ctype = cctype,utag = self.uid, body=cbody}
	--utils.print_table(ret_msg)
	session_wrapper.udp_send_msg(self.client_udp_ip,self.client_udp_port,ret_msg)
end

--���ﷵ�ظ��������û���Ϣ����Ҫʲô���������
function player:get_user_arrived()
	local body = {
	    --�ڷ�����չʾ����Ϣ
		unick = self.uinfo.unick,
		uface = self.uinfo.uface,
		usex =  self.uinfo.usex,
		seatid = self.seatid,
		side = self.side,
	}

	return body
end

return player

