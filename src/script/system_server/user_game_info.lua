--ϵͳ����ģ��
local utils = require("utils")
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_game = require("database/mysql_system_ugame")
local login_bonues = require("system_server/login_bonues")


--��ȡ��Ϸ����
function get_ugame_info(s, msg)
   local uid = msg[3];
   print("get_ugame_info call"..uid)
   
   mysql_game.get_ugame_info(uid, function (err, uinfo) 
       if err then -- ���߿ͻ���ϵͳ������Ϣ;
			 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--û�в�ѯ����˵�����״ν��룬ϵͳ�Զ���ʼ��һ������
		if uinfo == nil then 
			mysql_game.insert_ugame_info(uid, function(err, ret)
				if err then -- ���߿ͻ���ϵͳ������Ϣ;
					 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
				end
				--���µ��ñ�����
				print("recall get_ugame_info")
				get_ugame_info(s, msg)
			end)
			return
		end

		--�ж�status�Ƿ�����
		if uinfo.ustatus ~= 0 then 
		    local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.UserIsFreeze
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
		end

		login_bonues.ckeck_login_bonues(uid,function(err,bonues_info)
	       if err ~= nil or bonues_info == nil then
		       local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
		   end

		-- ������Ϸ�������ݿͻ���
        local ret_msg = {
					       stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.OK,
									uinfo = {
										uchip = uinfo.uchip,
										uexp = uinfo.uexp,
										uvip  = uinfo.uvip,
										uchip2  = uinfo.uchip2,
										uchip3 = uinfo.uchip3, 
										udata1  = uinfo.udata1,
										udata2  = uinfo.udata2,
										udata3 = uinfo.udata3,
								        }
								}
						  }
		utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)
        end)
   end)

end

local ugame_info = {
	get_ugame_info = get_ugame_info,
}
return ugame_info