#pragma once
namespace raft
{
	class service : public acl::service_base
	{
	public:
		service(acl::http_rpc_server &ser)
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