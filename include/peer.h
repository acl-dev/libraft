#pragma once
namespace raft
{
	class peer : public acl::thread
	{
	public:
		peer(node &_node)
			:node_(_node)
		{
			match_index_ = 0;
			next_index_ = 0;
		}
	private:
		virtual void* run()
		{
			do 
			{

			} while (true);
		}
		node &node_;
		log_entry_index_t match_index_;
		log_entry_index_t next_index_;
	};
}