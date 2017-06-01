#pragma once
namespace raft
{
	class node
	{
	public:
		node()
		{

		}
	private:
		acl::locker locker_;
		std::string id_;
	};
}