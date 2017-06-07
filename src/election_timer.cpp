#include "raft.hpp"
namespace raft
{
	election_timer::election_timer(node &_node)
		:node_(_node),
		cancel_(true)
	{
		start();
	}

	election_timer::~election_timer()
	{
		stop_ = true;
		cancel_timer();
		wait();
	}

	void election_timer::cancel_timer()
	{
		acl_pthread_mutex_lock(mutex_);
		cancel_ = true;
		delay_ = 1000*60;
		acl_pthread_cond_signal(cond_);
		acl_pthread_mutex_unlock(mutex_);
	}

	void election_timer::set_timer(unsigned int delay)
	{
		acl_pthread_mutex_lock(mutex_);
		delay_ = delay;
		cancel_ = false;
		acl_pthread_cond_signal(cond_);
		acl_pthread_mutex_unlock(mutex_);
	}

	void *election_timer::run()
	{
		while (!stop_)
		{
			timespec timeout;
			timeval now;
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec;
			timeout.tv_nsec = now.tv_usec * 1000;
			timeout.tv_sec += delay_ / 1000;
			timeout.tv_nsec += (delay_ % 1000) * 1000* 1000;

			acl_assert(!acl_pthread_mutex_lock(mutex_));

			int status = acl_pthread_cond_timedwait(
				cond_, 
				mutex_, 
				&timeout);

			if (cancel_ || status  != ACL_ETIMEDOUT)
			{
				acl_assert(!acl_pthread_mutex_unlock(mutex_));
				continue;
			}
			acl_assert(!acl_pthread_mutex_unlock(mutex_));

			node_.election_timer_callback();
		}

		return NULL;
	}
}