#pragma once
namespace raft
{
	enum status_t
	{
		E_OK,
		E_NO_LEADER,
		E_TIMEOUT,
		E_UNKNOWN,
		E_WRITE_LOG_ERROR,
	};
	struct replicate_cond_t;

	struct version
	{
		explicit version(log_index_t index = 0, term_t term = 0):
			index_(index),term_(term){}

		log_index_t index_;
		term_t term_;
	};

	struct apply_callback
	{
		virtual ~apply_callback() {};

		/**
		 * \brief 
		 * \param data from leader
		 * \param ver user should save this for do snapshot.
		 * snapshot need version to store snapshot info into file
		 * \return return true when user apply data ok. and node will auto update
		 * applied index. return false, node will invoke apply data again and again.
		 */
		virtual bool apply(const std::string& data, const version& ver) = 0;
	};


	/**
	 * \brief raft node
	 */
	class node
	{
	public:
		
		node();
		/**
		 * \brief replicate data to raft cluster .when N (N > nodes/2 ) 
		 * nodes receive data,
		 * it will return {E_OK,version{ log_index, log_term}}.  
		 * log_index is index of 
		 * the data in raft cluster.M is term of 
		 * data in the raft cluster. user should safe the version(N,M) for 
		 * making snapshot in the further.
		 * \param data : to replicate to raft cluster
		 * \param timeout_millis : wait for replicate done
		 * \return if replicate done without timeout .
		 * return {E_OK,{log_index, log_term}}
		 * if is not leader,will return {E_NO_LEADER,{0, 0}}.
		 * if write data to log failed. it will return {E_WRITE_LOG_ERROR,{0, 0}}.
		 * if wait replicate timeout ,will return {E_TIMEOUT, {0, 0}}.
		 */
		std::pair<status_t, version> 
			replicate(const std::string &data, unsigned int timeout_millis);

		/**
		 * \brief interface for user to update applied_index.
		 * if node is leader and user invoke replicate(...)
		 * to replicate data to raft cluster and 
		 * return {E_OK,{log_index, log_term}}.user user should apply 
		 * data to state machine. and invoke update_applied_index(log_index).
		 * otherwise replicate_callback will
		 * be invoke to apply this data again and again.
		 * 
		 * \param index return from invoke replicate ,{E_OK, {index, _}};
		 */
		void update_applied_index(log_index_t index);

		/**
		 * \brief check if leader now;
		 * \return if node is leader now.return true, otherwise return false;
		 */
		bool is_leader();
		
		/**
		 * \brief get cluster leader id
		 * \return id of leader, it maybe empty when cluster has not leader,
		 * or this node lose connect to cluster.
		 */
		std::string leader_id();

		/**
		 * \brief give snapshot_callback handle to node .
		 * when this node do log compaction, snapshot_callback::make_snapshot(...)
		 * will be invoke. and when this node receive a snapshot file, 
		 * load_snapshot(...) will be invoke.
		 * \param callback snapshot_callback obj
		 */
		void bind_snapshot_callback(snapshot_callback *callback);

		/**
		 * \brief bind replicate_callback handle to this node.
		 * when node is not leader,it will receive data from leader
		 * and replicate_callback::invoke will be invoke after leader has commited
		 * the data
		 * \param callback replicate_callback handle,
		 */
		void bind_apply_callback(apply_callback *callback);


		/**
		 * \brief set snapshot path. and snapshot files will store in this path
		 * \param path snapshot path.
		 */
		void set_snapshot_path(const std::string &path);


		/**
		 * \brief set path to store log files. 
		 * \param path if path is empty, it will store log 
		 * file in "./log/"
		 */
		void set_log_path(const std::string &path);


		/**
		 * \brief set path to store meta data file.
		 * \param path if path is empty,it will store data file 
		 * in "./metadata/"
		 */
		void set_metadata_path(const std::string &path);


		/**
		 * \brief size of log file. when log 'size >= this size it will 
		 * create new a log to store log data.
		 * \param size one log max size. 
		 */
		void set_max_log_size(size_t size);


		/**
		 * \brief set max count of log files, when
		 * the count of log files  >= this count
		 * node will do log compaction to delete half of logs.
		 * and maybe make a snapshot if need
		 * \param count max count of log files
		 */
		void set_max_log_count(size_t count);


		/**
		 * \brief return this node 's id
		 * \return this node 's id .
		 */
		std::string raft_id()const;

		/**
		* \brief this interface should regist to server to process vote 
		* request  from other candidate
		* \param req vote_request req
		* \param resp vote_response resp
		* \return return true
		*/
		bool handle_vote_request(const vote_request &req, vote_response &resp);

		/**
		* \brief this interface should regist to server to process 
		* leader replicate data request .
		* when leader invoke replicate(...) and data will send to 
		* follower and candidate. 
		* follower and candidate will save the data into log. and will commit to
		* user to apply.
		* \param req replicate_log_entries_request send from leader
		* \param resp replicate_log_entries_response  send back to leader
		* \return return true
		*/
		bool handle_replicate_log_request(
			const replicate_log_entries_request &req,
			replicate_log_entries_response &resp);

		/**
		* \brief this interface should regist to server to process install_snapshot_request 
		* when the follower miss logs and leader has not log to replicate to this node,
		* leader will send snapshot file to follower instead.
		* \param req install_snapshot_request send from leader.
		* \param resp install_snapshot_response to send back to leader
		* \return return true
		*/
		bool handle_install_snapshot_requst(
			const install_snapshot_request &req, 
			install_snapshot_response &resp);

	private:
		enum role_t
		{
			E_LEADER,//leader
			E_FOLLOWER,//follower
			E_CANDIDATE//candidate
		};

		friend class peer;
		friend class log_compaction;
		friend class election_timer;

		void init();

		//for peer
		/**
		 * \brief check is candicate
		 * \return true if candicate,otherwise return false;
		 */
		bool is_candicate();

		log_index_t gen_log_index();

		log_index_t last_log_index() const;

		void set_last_log_index(log_index_t index);

		log_index_t last_snapshot_index();

		void set_last_snapshot_index(log_index_t index);

		term_t last_snapshot_term();

		void set_last_snapshot_term(term_t term);

		term_t current_term();
		
		void set_current_term(term_t term);

		void set_leader_id(const std::string &leader_id);

		log_index_t committed_index();

		void set_committed_index(log_index_t index);

		role_t role();

		void set_role(role_t _role);

		std::string vote_for();

		void set_vote_for(const std::string &vote_for);

		bool build_replicate_log_request(
			replicate_log_entries_request &requst, 
			log_index_t index,
			int entry_size = 0) const;

		std::vector<log_index_t> get_peers_match_index();

		void replicate_log_callback();

		void build_vote_request(vote_request &req);

		void clear_vote_response();

		void vote_response_callback(const std::string &peer_id, 
			const vote_response &response);

		int peers_count();

		void handle_new_term(term_t term);

		bool get_snapshot(std::string &path) const;
		
		/**
   		 * 
		 * \brief scan snapshot path,and find snapshot files
		 * \return a map,first is snapshot last index, send is filepath
		 */
		std::map<log_index_t, std::string> scan_snapshots() const;

		/**
		 * \brief check should do log compaction now.
		 * \return return true should to do log compaction,otherwise return false
		 */
		bool should_compact_log();
		
		bool check_compacting_log();

		void async_compact_log();

		bool make_snapshot() const;

		void do_compaction_log() const;
		//

		void become_leader();

		void set_election_timer();

		void cancel_election_timer();

		void election_timer_callback();


		log_index_t start_log_index()const;

		//about peers function
		void notify_peers_replicate_log();

		void notify_peers_to_election();

		void update_peers_next_index(log_index_t index);
		//end
		void step_down();

		void load_snapshot_file();

		acl::fstream *get_snapshot_tmp(const snapshot_info &);

		void close_snapshot();

		void do_apply_log();

		void add_replicate_cond(replicate_cond_t *cond);

		void remove_replicate_cond(replicate_cond_t *cond);

		void make_log_entry(const std::string &data, log_entry &entry);

		bool write_log(const std::string &data, 
			log_index_t &index, term_t &term);

		void notify_replicate_conds(log_index_t index, 
			status_t = status_t::E_OK);

		void update_peers_match_index(log_index_t index);
	private:
		/**
		 * \brief apply log thread
		 */
		class apply_log : private acl::thread
		{
		public:
			explicit apply_log(node &);
			~apply_log();
			void do_apply();
			virtual void *run();
		private:
			bool wait_to_apply();
			node &node_;
			bool do_apply_;
			bool to_stop_;
			acl_pthread_mutex_t *mutex_;
			acl_pthread_cond_t *cond_;
		};

		/**
		 * \brief do log compaction work thread
		 */
		class log_compaction : private acl::thread
		{
		public:
			explicit log_compaction(node &_node);
			void do_compact_log();
		private:
			virtual void* run();
			node &node_;
		};
	private:
		log_manager *log_manager_;

		unsigned int election_timeout_;
		log_index_t last_log_index_;
		log_index_t committed_index_;
		log_index_t applied_index_;
		term_t		current_term_;
		role_t		role_;
		std::string raft_id_;
		std::string leader_id_;
		std::string vote_for_;
		acl::locker	metadata_locker_;

		typedef std::map<log_index_t, 
			replicate_cond_t*> replicate_conds_t;

		replicate_conds_t replicate_conds_;
		acl::locker replicate_conds_locker_;


		std::map<std::string, peer*> peers_;
		acl::locker peers_locker_;


		snapshot_callback	*snapshot_callback_;
		std::string			snapshot_path_;
		snapshot_info		*snapshot_info_;
		acl::fstream		*snapshot_tmp_;
		acl::locker			snapshot_locker_;
		log_index_t			last_snapshot_index_;
		term_t				last_snapshot_term_;

		std::string log_path_;
		std::string metadata_path_;

		size_t max_log_size_;
		size_t max_log_count_;

		bool		compacting_log_;
		acl::locker compacting_log_locker_;

		typedef std::map<std::string, 
			vote_response> vote_responses_t;

		vote_responses_t vote_responses_;
		acl::locker		 vote_responses_locker_;

		election_timer     election_timer_;
		log_compaction     log_compaction_worker_;
		apply_callback     *apply_callback_;
		apply_log          apply_log_;
	};
}
