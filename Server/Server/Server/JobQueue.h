#pragma once
#include "Job.h"
#include "IocpEvent.h"
#include "ConcurrentQueue.h"
#include "FineGrainedQueue.h"
#include <concurrent_queue.h>

class JobQueue : public IocpObject
{
public:
	virtual HANDLE GetHandle() override;

	virtual void Dispatch(class IocpEvent* iocpEvent, int numBytes = 0) override;

	JobQueue(HANDLE iocpHandle) : _iocpHandle(iocpHandle) {}

	void Push(shared_ptr<Job> job)
	{
		_jobQueue.push(job);
		RegisterJobs();
	}

	template<typename T, typename... Arguments>
	void Push(shared_ptr<T> owner, void(T::* memFunc)(Arguments...), Arguments&&... args)
	{
		shared_ptr<Job> job = make_shared<Job>(owner, memFunc, std::forward<Arguments>(args)...);
		_jobQueue.push(job);
		RegisterJobs();
	}	

	void ExecuteJobs()
	{

		while (true) {
			shared_ptr<Job> job;
			while (_jobQueue.try_pop(job)) {
				job->Execute();
			}
			
			bool expected = true;
			if(_isProcessing.compare_exchange_strong(expected, false))
				break;

		}
		
	}


private:
	
	void RegisterJobs()
	{
		bool expected = false;
		if (_isProcessing.compare_exchange_strong(expected, true))
		{
			// PostQueuedCompletionStatus를 JobQueue 내부에서 안전하게 호출합니다.
			PostQueuedCompletionStatus(
				_iocpHandle,
				0,
				0,
				new JobEvent(shared_from_this())
			);
			
		}
				
	}

	

private:
	
	HANDLE						_iocpHandle;
	// ConcurrentQueue<shared_ptr<Job>> _jobQueue;
	// FineGrainedConcurrentQueue<shared_ptr<Job>> _jobQueue;
	concurrency::concurrent_queue<shared_ptr<Job>> _jobQueue;

	std::atomic<bool> _isProcessing = false;
};


