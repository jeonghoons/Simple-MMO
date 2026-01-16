#pragma once
#include "DBConnectionPool.h"
#include <concurrent_queue.h>
#include "Job.h"

struct UserAccount
{
	unsigned int uniqueId;
	WCHAR accountPassword[20];
	WCHAR accountId[20];
};

class DatebaseWorker : public enable_shared_from_this<DatebaseWorker>
{
public:
	DatebaseWorker(int num_connections);
public:
	void Run();

	template<typename... Arguments>
	void PushJob(void(DatebaseWorker::* memFunc)(Arguments...), Arguments... args)
	{
		shared_ptr<Job> job = make_shared<Job>(shared_from_this(), memFunc, std::forward<Arguments>(args)...);
		_dbJobQueue.push(job);
	}

	void TryLogin(shared_ptr<Session> session, string recvId, string recvPw);

private:
	concurrency::concurrent_queue<shared_ptr<Job>> _dbJobQueue;
	vector<thread> _threads;
	unique_ptr<DBConnectionPool> _dbConnectionPool;
};

