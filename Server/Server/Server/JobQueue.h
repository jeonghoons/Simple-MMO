#pragma once
#include "Job.h"
#include "IocpEvent.h"
#include "ConcurrentQueue.h"
#include "FineGrainedQueue.h"

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	JobQueue(HANDLE iocpHandle) : _iocpHandle(iocpHandle) {}
	// JobQueue* GetCompletionKey() { return this; }

	/*void Push(function<void()>&& func)
	{
		shared_ptr<Job> job = make_shared<Job>(move(func));
		_jobQueue.Push(job);
		RegisterJobs();
	}*/

	template<typename T, typename... Arguments>
	void Push(shared_ptr<T> owner, void(T::* memFunc)(Arguments...), Arguments&&... args)
	{
		shared_ptr<Job> job = make_shared<Job>(owner, memFunc, std::forward<Arguments>(args)...);
		_jobQueue.Push(job);
		
		RegisterJobs(owner);
	}
	

	void ExecuteJobs()
	{
		
		vector<shared_ptr<Job>> jobs;
		jobs = _jobQueue.PopAll();
		for (auto& job : jobs)
			job->Execute();


		_isProcessing.store(false);
	}


private:
	template<typename T>
	void RegisterJobs(shared_ptr<T> owner)
	{
		bool expected = false;
		if (_isProcessing.compare_exchange_strong(expected, true))
		{
			// PostQueuedCompletionStatus를 JobQueue 내부에서 안전하게 호출합니다.
			PostQueuedCompletionStatus(
				_iocpHandle,
				0,
				0,
				new JobEvent(owner)
			);
			
		}
				
	}

	

private:
	
	HANDLE						_iocpHandle;
	ConcurrentQueue<shared_ptr<Job>> _jobQueue;
	// FineGrainedConcurrentQueue<shared_ptr<Job>> _jobQueue;

	std::atomic<bool> _isProcessing = false;
};

