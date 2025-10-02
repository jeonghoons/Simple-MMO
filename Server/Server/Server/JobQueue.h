#pragma once
#include "Job.h"
#include "ConcurrentQueue.h"
#include "IocpEvent.h"

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	JobQueue(HANDLE iocpHandle) : _iocpHandle(iocpHandle) {}
	JobQueue* GetCompletionKey() { return this; }

	/*void Push(function<void()>&& func)
	{
		shared_ptr<Job> job = make_shared<Job>(move(func));
		_jobQueue.Push(job);
		RegisterJobs();
	}*/

	
	/*void Push(shared_ptr<Job> job)
	{
		
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
		while (true)
		{
			std::shared_ptr<Job> job;
			while (_jobQueue.TryPop(job))
			{
				job->Execute();
			}

			// 모든 Job 실행 후, 플래그를 true -> false로 변경 시도 (핵심 동기화 로직)
			bool expected = true;
			if (_isProcessing.compare_exchange_strong(expected, false))
			{
				break; // 성공적으로 처리 완료
			}
			// 실패 시: 루프를 다시 돌아 새로 들어온 Job을 처리
		}
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
				(ULONG_PTR)GetCompletionKey(),
				new JobEvent(owner)
			);
		}
	}

	

private:
	
	// queue<shared_ptr<Job>>		_jobQueue;

	HANDLE						_iocpHandle;
	ConcurrentQueue<shared_ptr<Job>> _jobQueue;

	std::atomic<bool> _isProcessing = false;
};

