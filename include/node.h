#pragma once
namespace raft
{
    struct peer_info
    {
        std::string peer_id_;
        std::string addr_;
    };

    /**
     * current node status version.
     * term is raft term.
     * log_index the log index.1 is the first log.
     * snapshot will take this version in the file.
     */
    struct version
    {
        explicit version(log_index_t index = 0, term_t term = 0)
                : index_(index),
                  term_(term)
        {

        }
        log_index_t index_;
        term_t term_;
    };

    bool write(acl::ofstream &file, const std::vector<peer_info> &infos_);
	/**
	 * \brief write a version abj to file
	 * \param file file stream
	 * \param ver version to write
	 * \return retur true,if write ok,
	 * otherwise return false;
	 */
	bool write(acl::ostream &file, const version &ver);

	/**
	 * \brief read version from file
	 * \param file file to read
	 * \param ver buffer to store data
	 * \return return true if read ok,
	 * otherwise return false;
	 */
	bool read(acl::istream &file, version &ver);



	bool operator <(const version& left, const version& right);

	struct apply_callback
	{
		virtual ~apply_callback() {};

		/**
		 * \brief apply_callback operator() function
		 * \param data from leader
		 * \param ver user should save this version for doing snapshot 
		 * in the future
		 * snapshot need version to store snapshot info into file
		 * \return return true when user apply data ok. and node will
		 * auto update applied index. return false,
		 * node will invoke apply data again and again.
		 */
		virtual bool operator()(const std::string& data,
                                const version& ver) = 0;
	};


    /**
     * replicate_callback is handle to node when replicate data.
     * if replicate done or error.it's operator() function will be invoked.
     * and user should to apply the request to state machine
     */
	struct replicate_callback
	{
		enum status_t
		{
			E_OK,
			E_NO_LEADER,
			E_ERROR,
		};
		virtual ~replicate_callback(){}
        /**
         * callback function .when node replicate done or error.
         * @param status replicate status
         * @param ver current node status version.keep it .
         * @return return true.if user apply data ok.
         * return user apply data error
         */
		virtual bool operator()(status_t status, version ver) = 0;
	};

	struct load_snapshot_callback
	{
		virtual ~load_snapshot_callback() {}

		/**
		 * \brief node maybe be lost some logs.and leader has no
		 * the same logs to send to this node.leader will send
		 * snapshot file to this node.and node invoke load_snapshot_callback
		 * to give snapshot to user's state machine.user's state machine will
		 * reload this snapshot.
		 * \param filepath 
		 * \return 
		 */
		virtual bool operator()(const std::string &filepath) = 0;
	};

	
	struct make_snapshot_callback
	{
		virtual ~make_snapshot_callback() {}

		/**
		 * \brief node make snapshot to compact log.and this functor 
		 * is make snapshot callnack handle.user must give this handle
		 * to node.
		 * \param path snapshot path.snapshot file will create in this path.
		 * the path is the same to node set_snapshot_path(const std::string &path)
		 * \param filepath is snapshot file path.
		 * and it's ext name must not be ".snapshot".
		 * because "*.snapshot" is finished snapshot file path.
		 * node will rename it's extension name to ".snapshot"
		 * when operator()(...)return true.
		 * \return return true if do snapshot ok.
		 * return false mean something error happend
		 * ,making snapshot failed
		 */
		virtual bool operator()(const std::string &path, std::string &filepath) = 0;
	};

    class metadata;
    /**
	 * \brief raft node
	 */
	class node
	{
	public:
		node();
		
		~node();
		/**
		 * \brief to replicate data to cluster.when this node is leader.
		 * if majorty of nodes recevie the data. replicate_callback will be
		 * invoke. and user should apply data to state machine.
		 * \param data the data to replicate to cluster
		 * \param callback replicate result callback .when replicate done or 
		 * something error happend.eg lost leadership
		 * \return retrun false when this node is not leader or write log data 
		 * error. therwise return true to user.
		 */
		bool replicate(const std::string &data, replicate_callback *callback);

		
		/**
		 * \brief read data from node's log.
		 * if state machine lost it's data. state machine could read data from
		 * this interface.but the index of the data must less or eq ( <= ) 
		 * applied index.if index is less then node log start index.it will
		 * read data failed, at this time state machine should reload last
		 * snapshot from node first. and read the data between 
		 * [snapshot.version.index_,node.applied_index()].
		 * \param index the index of the data to read
		 * \param data buffer to store the data.
		 * \return return true if read ok.otherwise will return false;
		 */
		bool read(log_index_t index, std::string &data, version &ver);


        /**
         * get snapshot file from raft
         * @return snapshot file path
         * if return file path not empty.mean has snapshot.
         * otherwise has not snapshot
         */
        std::string get_snapshot() const;


		/**
		* \brief return committed log index.
		* when state machine loss data.
		* it can call and reload snapshot first.
		* and then call committed_index() to get
		* committed index of log data.
		* reload logs to recovery remain data loss
		* \return
		*/
        log_index_t committed_index();

        /**
         * get applied index of node status
         * @return return log index
         */
        log_index_t applied_index();


        /**
         * update applied index status.
         * @param index index of log
         */
        void set_applied_index(log_index_t index);


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
		 * \brief return this node 's id
		 * \return this node 's id .
		 */
        std::string node_id()const;

        /**
         * \brief reload node log. metadata.
         * \return return true if reload ok.return if reload failed.
         */
        bool reload();

        /**
         * start raft node.before invoke start.
         * U need to set node config done.
         */
        void start();

        ///raft config interface
    public:

		/**
		 * \brief set peer id for this node
		 * \param peers peer id vector 
		 */
		void set_peers(const std::vector<peer_info> &peer_infos_);

        /**
         * get peer_info from node.
         * when make snapshot.state machine ,will get peer_info and
         * store it in snapshot file.
         * @return peer_info
         */
        std::vector<peer_info> get_peer_infos();
        /**
         * \brief give snapshot_callback handle to node .
         * when leader send a snapshot file to this node .it will be invoke to user
         * and user should reset state machine, and reload snapshot file to state
         * machine .
         * \param callback snapshot_callback obj
         */
		void set_load_snapshot_callback(load_snapshot_callback *callback);
		
		/**
		 * \brief set make snapshot callback handle.
		 * when node to do log compaction,it will try to make a snapshot,
		 * and delete useless log files.
		 * user must to invoke this
		 * interface to set make_snapshot_callback handle to node.
		 * otherwise it will crush when do log compaction
		 * \param callback make_snapshot_callback
		 */
		void set_make_snapshot_callback(make_snapshot_callback* callback);

		/**
		 * \brief bind replicate_callback handle to this node.
		 * when node is not leader,it will receive data from leader
		 * and apply_callback::apply(...) will be invoke after leader has commited
		 * the index of data. and then in the callback
		 * function apply_callback::apply()
		 * user can recevie the data. and apply to user state machine.
		 * \param callback replicate_callback handle,
		 */
		void set_apply_callback(apply_callback *callback);

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
		 * in "metadata/"
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
         * set node id.
         * @param id node id.unique in the cluster
         */
        void set_node_id(const std::string &id);
		///raft rpc interface///
	public:
		/**
		* \brief this interface should regist to server to process vote
		* request  from other candidate
		* \param req vote_request req
		* \param resp vote_response resp
		* \return return true
		*/
		bool handle_vote_request(const vote_request &req,
                                 vote_response &resp);

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
		* \brief this interface should regist to server to process
		 * install_snapshot_request
		* when the follower miss logs and leader has not log to
		 * replicate to this node leader will send snapshot file to
		 * follower instead.
		* \param req install_snapshot_request send from leader.
		* \param resp install_snapshot_response to send back to leader
		* \return return true
		*/
		bool handle_install_snapshot_request(
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

		void load_last_snapshot_info();

		//for peer
		/**
		 * \brief check is candidate
		 * \return true if candidate,otherwise return false;
		 */
		bool is_candidate();

		log_index_t last_log_index() const;

        term_t last_log_term()const;

		log_index_t last_snapshot_index();

		void set_last_snapshot_index(log_index_t index);

		term_t last_snapshot_term();

		void set_last_snapshot_term(term_t term);

		term_t current_term();
		
		void set_current_term(term_t term);

		void set_leader_id(const std::string &leader_id);

		void set_committed_index(log_index_t index);

		int role();

        const char *role_str();

		void set_role(int _role);

		std::string vote_for();

		void set_vote_for(const std::string &vote_for);

        bool log_ok();

        void set_log_ok(bool ok);

		bool build_replicate_log_request(
			replicate_log_entries_request &request,
			log_index_t index,
			int entry_size = 0);

		std::vector<log_index_t> get_peers_match_index();

		void replicate_log_callback();

		void build_vote_request(vote_request &req);

		void clear_vote_response();

		void vote_response_callback(const std::string &peer_id, 
			const vote_response &response);

		int peers_count();

		void handle_new_term(term_t term);

		/**
   		 * 
		 * \brief scan snapshot path,and find snapshot files
		 * \return a map,first is snapshot last index, send is filepath
		 */
		std::map<log_index_t, std::string> scan_snapshots() const;

		/**
		 * \brief check should do log compaction now.
		 * \return return true should to do log compaction,
		 * otherwise return false
		 */
		bool should_compact_log();
		
		void async_compaction_log();

        void remove_old_snapshot()const;

		bool make_snapshot() const;

		void do_compaction_log() const;
		//

		void become_leader();

		void set_election_timer();

		void cancel_election_timer();

		void election_timer_callback();


		log_index_t start_log_index()const;

		//notify peers function
		void notify_peers_replicate_log();

		void notify_peers_to_election();

		void update_peers_next_index(log_index_t index);
		//end

		void step_down();

		void load_snapshot_file();

		acl::fstream *get_snapshot_tmp(const snapshot_info &);

		void close_snapshot();

		void invoke_apply_callbacks();

		void invoke_replicate_callback(replicate_callback::status_t status);

        void notify_replicate_failed();

		void make_log_entry(const std::string &data, log_entry &entry);

		bool write_log(const std::string &data,
                       log_index_t &index,
                       term_t &term);

		void add_replicate_callback(const version& version, 
									replicate_callback* callback);

		void update_peers_match_index(log_index_t index);

        void init_peers();
	private:
		/**
		 * \brief apply log thread
		 */
		class apply_log : public acl::thread
		{
		public:
			explicit apply_log(node &);
			~apply_log();
			void to_apply ();
			virtual void *run();
		private:
			bool wait_to_apply();
			node &node_;
			bool to_stop_;
			acl_pthread_mutex_t mutex_;
			acl_pthread_cond_t cond_;
		};

		/**
		 * \brief do log compaction work thread
		 */
		class log_compaction : public acl::thread
		{
		public:
			explicit log_compaction(node &_node);
			~log_compaction();
			void do_compact_log();
		private:
			virtual void* run();
			acl_pthread_mutex_t mutex_;
			acl_pthread_cond_t cond_;
			node &node_;
			bool do_compact_log_;
            bool stop_;
		};

		class election_timer : acl::thread
		{
		public:
			election_timer(node &_node);

			~election_timer();

			void set_timer(unsigned int delays_mills);
			void cancel_timer();
		private:
			virtual void* run();
			node &node_;
			bool cancel_;
			bool stop_;
			unsigned int delay_;
			acl_pthread_mutex_t mutex_;
			acl_pthread_cond_t cond_;
		};
	private:
		typedef std::map<version, replicate_callback*> replicate_callbacks_t;
		typedef std::map<std::string, vote_response>   vote_responses_t;

		log_manager *log_manager_;

		unsigned int election_timeout_;

		int		role_;

        bool        start_;
		std::string node_id_;
		std::string leader_id_;
		std::string vote_for_;
        bool        log_ok_;
        std::vector<peer_info> peer_infos_;
		acl::locker	metadata_locker_;


		replicate_callbacks_t replicate_callbacks_;
		acl::locker replicate_callbacks_locker_;


		std::map<std::string, peer*> peers_;
		acl::locker peers_locker_;


		load_snapshot_callback	*load_snapshot_callback_;
		make_snapshot_callback  *make_snapshot_callback_;
		std::string			    snapshot_path_;
		snapshot_info		    *snapshot_info_;
		acl::fstream		    *snapshot_tmp_;
		acl::locker			    snapshot_locker_;
		log_index_t			    last_snapshot_index_;
		term_t				    last_snapshot_term_;

		std::string log_path_;
		std::string metadata_path_;

		size_t max_log_size_;
		size_t max_log_count_;
        size_t mini_log_count_;
        size_t max_snapshot_size_;


		vote_responses_t vote_responses_;
		acl::locker		 vote_responses_locker_;

		election_timer     election_timer_;
		log_compaction     log_compaction_worker_;
		apply_callback     *apply_callback_;
		apply_log          apply_log_;
        metadata           *metadata_;
	};
}
