#pragma once
#include "Job.h"

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void PushJob(function<void()>&& func)
	{
		shared_ptr<Job> job = make_shared<Job>(move(func));
	}


	template<typename T, typename... Arguments>
	void PushJob(void(T::* memFunc)(Arguments...), Arguments&&... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		shared_ptr<Job> job = make_shared<Job>(owner, memFunc, args...);
		_lock.lock();
		_jobQueue.push(job);
		_lock.unlock();
	}

	virtual void	FlushJob() abstract;

protected:
	mutex					_lock;
	queue<shared_ptr<Job>>		_jobQueue;
};

