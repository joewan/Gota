local config = require("conf")
local mysql_conn = nil

function mysql_connect_auth_center()

	local auth_conf = config.auth_mysql
	mysql_wrapper.connect(auth_conf.host,auth_conf.port,auth_conf.db_name,
							auth_conf.uname,auth_conf.upwd,function(err, conn)
				if err ~= nil then
				--Logger_wrapper.
					print(err)
					--��������
					timer_wrapper.create_timer_once(check_session_connect,5000,5000)
					return
				end
				print("connect auth db sucess!!")
				mysql_conn = conn
    end)
end
--�ű������ؾ͵��øú���
--mysql_connect_auth_center()