--�ο�ģ���߼�
---- {stype, ctype, utag, body}
--�ο͵�¼

--����ģ��ʱ��������֤������
--17min
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
		  return 
		end

		--�ж��Ƿ��Ƿ����
		if user_info==nil then
		  --û�����������������
		    mysql_center.insert_guest_user_info(guest_key,function (err,ret)
					if err then
						--���ظ��ͻ��˴�����Ϣ
						print(err)
						return 
					 end
					 --����ɹ������µ����Լ�
					 guest_login(s,msg)
            end)
			return 
		end
		--����鵽�ˣ����ж�״̬
		--print_r(user_info)
		utils.print_table(user_info)
		if user_info.status ~= 0 then
		     --״̬����ȷ ,������Ϣ���ͻ���
			 print("user status error status:"..user_info.status)
		     return
		end
		--�ж��û��Ƿ�Ϊ�ο�״̬
		if user_info.is_guest ~=1 then
		    --�����ο�״̬�޷�ʹ���ο�key��¼
			print("user is_guest error"..user_info.is_guest)
			return 
		end
		print("user data"..user_info.uid,user_info.unick,user_info.status)
		--���ص�¼�ɹ���Ϣ���ͻ���
    end)
end

local guest = {
	login = guest_login
}

return guest

