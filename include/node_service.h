#pragma once
namespace raft
{
	class node_service : public acl::service_base
	{
	public:
		node_service(acl::http_rpc_server &ser)
			:acl::service_base(ser)
		{

		}
	protected:
		virtual void init()
		{

		}

	private:

	};
}