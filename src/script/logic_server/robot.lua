--��һ����˶���
local player = require("logic_server/logic_player")
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")

--�̳�player��
local robot = player:new()

function robot:new()
	local instant = {} --���ʵ��

	setmetatable(instant, {__index = self}) 
	return instant
end

function robot:init(uid, s, ret_handler)
    --���ø���init����
	player.init(self,uid,s,ret_handler)
	--����Ϊ����������
	self.is_robot = true

end

return robot

