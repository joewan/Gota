--�ο�ģ���߼�
---- {stype, ctype, utag, body}
--�ο͵�¼

--����ģ��ʱ��������֤������ --20min
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
mysql_center = require("database/mysql_auth_center")
utils = require("utils")

function guest_login(s,msg)
	print(msg[4].guest_key)
	local guest_key = msg[4].guest_key

	--db��ӿڷ��ص�������lua��table��ʽ
	mysql_center.get_guest_user_info(guest_key,function(err,user_info)
		if err then
		--���ظ��ͻ��˴�����Ϣ
		  print(err)
		  --local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=200}}
		  local ret_msg = {
					  stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
					  body={
					      status = res_module.SystemErr
					    }}

					  session_wrapper.send_msg(s,ret_msg)
		  return 
		end

		--�ж��Ƿ��Ƿ����
		if user_info==nil then
		  --û�����������������
		    mysql_center.insert_guest_user_info(guest_key,function (err,ret)
					if err then
						--���ظ��ͻ��˴�����Ϣ
						print(err)
						local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.SystemErr
							}}

						session_wrapper.send_msg(s,ret_msg)
						return 
					 end
					 --����ɹ������µ����Լ�
					 guest_login(s,msg)
            end)
			return 
		end
		--����鵽�ˣ����ж�״̬
		--utils.print_table(user_info)
		if user_info.status ~= 0 then
		     --״̬����ȷ ,������Ϣ���ͻ���
			 print("user status error status:"..user_info.status)
			 local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.UserIsFreeze
							}}

			session_wrapper.send_msg(s,ret_msg)
		     return
		end
		--�ж��û��Ƿ�Ϊ�ο�״̬
		if user_info.is_guest ~=1 then
		    --�����ο�״̬�޷�ʹ���ο�key��¼
			print("user is_guest error"..user_info.is_guest)
			local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.UserIsNotGuest
							}}

			session_wrapper.send_msg(s,ret_msg)

			return 
		end
		print("user data"..user_info.uid,user_info.unick,user_info.status)
		--���ص�¼�ɹ���Ϣ���ͻ���
		local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.OK,
									userinfo = {
									   unick=user_info.unick,
									   sex=user_info.usex,
									   face=user_info.uface,
									   uvip=user_info.uvip,
									   uid=user_info.uid
									}
							}}
		--utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)

    end)
end

local guest = {
	login = guest_login
}

return guest

