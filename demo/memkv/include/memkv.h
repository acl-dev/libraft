#pragma once

class memkv :public acl::http_rpc_server
{
public:
	memkv()
	{
	}
	~memkv()
	{

	}
private:
	virtual void init()
	{
		regist_service<memkv_service>();
	}
};


