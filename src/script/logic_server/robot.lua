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
