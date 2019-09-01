--ÿ�ս���ģ��
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_game = require("database/mysql_system_ugame")
local utils = require("utils")
local ugame_info_conf = require("user_game_config")

--�ж��Ƿ���Ҫ���Ž��� 36min
local function send_bonues_to_user(uid, bonues_info, ret_handler)
   --ÿ������ȡһ�ν���(�жϹ���: �ϴη���ʱ����Ƿ�<�����ʱ��� �����ͷ���)
   --������¼���жϣ������¼ʱ���>����ʱ��� && <����ʱ���
   	if bonues_info.bonues_time < utils_wrapper.timestamp_today() then
		if bonues_info.bonues_time >= utils_wrapper.timestamp_yesterday() then -- ������½
			bonues_info.days = bonues_info.days + 1
		else 
		    --���¿�ʼ����
			bonues_info.days = 1
		end
			 
		if bonues_info.days > #ugame_info_conf.login_bonues then 
		    --���¿�ʼ����
			bonues_info.days = 1
		end

		--����Ϊ���Ž���״̬
		bonues_info.status = 0
		--���Ž���ʱ��
		bonues_info.bonues_time = utils_wrapper.timestamp()
		--���õ�����ȡ��������
		bonues_info.bonues = ugame_info_conf.login_bonues[bonues_info.days]
		--�����û����ս���db����
	    mysql_game.update_login_bonues(uid, bonues_info, function (err, ret)
			if err then 
				ret_handler(err, nil)
				return
			end

			   ret_handler(nil, bonues_info)
		end)
		
		return
    end

	-- �ѵ�½������Ϣ���ظ�ugame
	ret_handler(nil, bonues_info)
end
--���ص�cb_handle(err,bonues_info)
function ckeck_login_bonues(uid,cb_handle)
	print("ckeck_login_bonues")
	if uid <= 0 then
	  if cb_handle~=nil then
	     cb_handle("uid is err",nil)     
	  end
	  return
	end

	mysql_game.get_bonues_info(uid, function (err, bonues_info)
	  if err then
			cb_handle(err, nil)
			return 
		end

		-- ����û���һ������½��Ĭ�ϸ�����һ����¼
	  if bonues_info == nil then
	     	    mysql_game.insert_bonues_info(uid, function (err, ret)
				if err ~= nil then
				    print("insert_bonues_info"..err)
					cb_handle(err, nil)
					return
				end
				--�ڴ˵����Լ�������ȡ��������
				ckeck_login_bonues(uid, cb_handle)
			end)
			return 
	  end

	  --���ͽ�����Ϣ���û�
	  send_bonues_to_user(uid, bonues_info, cb_handle)
	end)
	
end

function recv_login_bonues(s,msg)
	uid = msg[3]
	print("recv_login_bonues uid:"..uid)
	mysql_game.get_bonues_info(uid, function (err, bonues_info)
	  if err then
		 --����ϵͳ����
		  local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
	  end

		-- ����û���һ������½��Ĭ�ϸ�����һ����¼
	  if bonues_info == nil or bonues_info.status~=0 then
	      local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.InvalidOptErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
	  end

	  --������ȡ
	  mysql_game.update_login_bonues_status(uid,function(err,ret)
	        if err ~= nil then
	        	 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
			end
            
			--���½������
			mysql_game.add_chip(uid, bonues_info.bonues, nil)
			local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.OK
							}}

			 session_wrapper.send_msg(s,ret_msg)

      end)

	end)

end

local login_bonues = {
	ckeck_login_bonues = ckeck_login_bonues,
	recv_login_bonues = recv_login_bonues,
}
return login_bonues