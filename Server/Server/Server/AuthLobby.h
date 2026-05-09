#pragma once
#include "JobQueue.h"


class AuthLobby : public enable_shared_from_this<AuthLobby>
{
public:
	AuthLobby(HANDLE iocpHandle);
	~AuthLobby();

	template<typename... Arguments>
	void PushJob(void(AuthLobby::* memFunc)(Arguments...), Arguments... args)
	{
		_jobQueue->Push(shared_from_this(), memFunc, std::forward<Arguments>(args)...);
	}

	void LoginRequest(shared_ptr<Session> session, string accountId, string password);

public:
	void OnLoginSuccess(shared_ptr<Session> session, int playerType);
	void OnLoginFailed(shared_ptr<Session> session, string errorMsg);

private:
	shared_ptr<JobQueue> _jobQueue;
};

