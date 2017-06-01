#pragma once
namespace raft
{
	class server : acl::http_rpc_server
	{
	public:
		virtual void init()
		{

		}

		void new_raft_node(const std::string &raft_id)
		{
			regist_service<service>();
		}
	};
}