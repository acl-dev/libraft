#pragma once
namespace raft
{
	class node;

	/**
	 * \brief peer mean other node in the cluster
	 * one peer connect to one node, and it has one thread
	 * to do replicate data,election,install snapshot request
	 * node control peer to talk to other node
	 * match_index of the peer mean that the node has recevie
	 * the index of log entry. 
	 * next_index mean next index to replicate to the node
	 * normal next_index = match_index + 1
	 */
	class peer :private acl::thread
	{
	public:
		peer(node &_node, const std::string &peer_id);

		~peer();

		/**
		 * \brief notify peer thread to replicate leader's logs to 
		 * peer node.
		 */
		void notify_replicate();
		
		/**
		 * \brief notify peer thread to 
		 * send delection request to peer 
		 */
		void notify_election();

		/**
		 * \brief get peer match index.
		 * match log mean that,peer log index reach the index.
		 * \return return index of log
		 */
		log_index_t match_index();

		/**
		 * \brief 
		 * \param index 
		 */
		void set_next_index(log_index_t index);

		/**
		 * \brief 
		 * \param index 
		 */
		void set_match_index(log_index_t index);

        void start();
	private:
		void notify_stop();

		void do_replicate();

		bool do_install_snapshot();

		void do_election();

		bool wait_event(int &event);

		/**
		 * \brief thread runtine
		 * \return 
		 */
		virtual void* run();

	private:
		node		&node_;
		std::string peer_id_;
		acl::locker locker_;

		log_index_t match_index_;
		log_index_t next_index_;

		int event_;

		acl_pthread_cond_t cond_;
		acl_pthread_mutex_t mutex_;
		
		timeval last_heartbeat_time_;
		long long heart_inter_;
		
		acl::string replicate_service_path_;
		acl::string election_service_path_;
		acl::string install_snapshot_service_path_;

		acl::http_rpc_client &rpc_client_;
		size_t rpc_fails_;
		size_t req_id_;
		
	};
}