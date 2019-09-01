--��ʽ��¼ģ��
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local utils = require("utils")

function uname_login_process(s,msg)
    --���ﻹû�е�¼��û��uid,ֻ��utag
	--utag�����ط������ʱsession id
	local cutag = msg[3]
	print("uname_login_process: utag:"..cutag)
	local unamelogin_req = msg[4]
	if string.len(unamelogin_req.uname) <=0 or string.len(unamelogin_req.upwd_md5) ~=32 then
	  local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.InvaildErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
	end

	--������ȷ����ѯdb���ݣ��ж��û����������Ƿ���ȷ
	print("uname:"..unamelogin_req.uname.." ".."upwd:"..unamelogin_req.upwd_md5)
	mysql_center.get_uinfo_by_uname_upwd(unamelogin_req.uname,unamelogin_req.upwd_md5,function(err,userinfo)
	     if err ~= nil then
		    local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		 end

		 --û�в�ѯ���û��������
		 if userinfo == nil then 
		  local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.UserNotFound
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--�ж��ʺ�״̬
		if userinfo.status ~= 0 then
		    local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.UserIsFreeze
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--��ʽ��¼�ɹ�
		
		redis_center.set_userinfo_to_redis(userinfo.uid,userinfo)
		--返回登录成功消息给客户端
		local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.OK,
									userinfo = {
									   --�����key��Ҫ��pbЭ������һ���������ڵײ�
									   --�������л�pb�Ҳ�����Ӧ�ֶ�
									   unick = userinfo.unick,
									   uface = userinfo.uface,
									   usex = userinfo.usex,
									   uvip = userinfo.uvip,
									   uid = userinfo.uid,
									}
							}}
		utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)
		print("uname_login_process return data")
		
    end)
end

local unamelogin = {
	uname_login_process = uname_login_process,
}

return unamelogin