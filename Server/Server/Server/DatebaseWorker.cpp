#include "pch.h"
#include "DatebaseWorker.h"
#include "Player.h"

DatebaseWorker::DatebaseWorker(int num_connections)
{
	const WCHAR* connectionPath = L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=SimpleMMO;Trusted_Connection=Yes;";
	_dbConnectionPool = make_unique<DBConnectionPool>();
	if (false == _dbConnectionPool->Connect(num_connections, connectionPath))
	{
		std::cout << "DB Connect 오류 !" << std::endl;
		exit(-1);
	}
	std::cout << "DB 서버 Connected" << std::endl;

	for (int i = 0; i < num_connections; ++i) {
		_threads.emplace_back(&DatebaseWorker::Run, this);
	}
}

void DatebaseWorker::Run()
{
	while (true)
	{
		shared_ptr<Job> job;
		if (_dbJobQueue.try_pop(job)) {
			job->Execute();
		}
		else {
			std::this_thread::yield();
		}
	}
}


void DatebaseWorker::TryLogin(shared_ptr<Session> session, string recvId, string recvPw)
{
	DBConnection* dbConn = _dbConnectionPool->Pop();
	dbConn->Unbind();

	WCHAR name[20] = {};
	MultiByteToWideChar(CP_ACP, 0, recvId.c_str(), -1, name, _countof(name));
	SQLLEN nameLen = 0;
	dbConn->BindParam(1, name, &nameLen);


	int outId = 0;
	SQLLEN outIdlen = 0;
	dbConn->BindCol(1, &outId, &outIdlen);

	WCHAR outName[20] = {};
	SQLLEN outNameLen = 0;
	dbConn->BindCol(2, outName, static_cast<int>(sizeof(outName) / sizeof(outName[0])), &outNameLen);


	WCHAR outpassword[20] = {};
	SQLLEN outpasswordLen = 0;
	dbConn->BindCol(3, outpassword, static_cast<int>(sizeof(outpassword) / sizeof(outpassword[0])), &outpasswordLen);

	if (dbConn->Execute(L"SELECT id, account_id, password FROM [dbo].[User_Account] WHERE account_id = (?)"))
	{

	}

	if (dbConn->Fetch())
	{
		WCHAR clpassword[20] = {};
		MultiByteToWideChar(CP_ACP, 0, recvPw.c_str(), -1, clpassword, _countof(clpassword));
		if (lstrcmpW(clpassword, outpassword) == 0)
		{
			cout << "로그인 성공" << std::endl;
			GRoomManager->EnterPlayer(session);			
		}
		else
		{
			cout << "비밀번호가 다릅니다." << std::endl;
		}
	}
	else
	{
		cout << "없는 아이디" << std::endl;
	}

	

	_dbConnectionPool->Push(dbConn);


	
}

void DatebaseWorker::TrySignUP(shared_ptr<Session> session, string recvId, string recvPw)
{
	DBConnection* dbConn = _dbConnectionPool->Pop();
	dbConn->Unbind();

	WCHAR name[20] = {};
	MultiByteToWideChar(CP_ACP, 0, recvId.c_str(), -1, name, _countof(name));
	SQLLEN nameLen = 0;
	dbConn->BindParam(1, name, &nameLen);

	WCHAR password[20] = {};
	MultiByteToWideChar(CP_ACP, 0, recvPw.c_str(), -1, password, _countof(password));
	SQLLEN passwordLen = 0;
	dbConn->BindParam(2, password, &passwordLen);

	TIMESTAMP_STRUCT ts = { 1998, 10, 01 };
	SQLLEN tsLen = 0;
	dbConn->BindParam(3, &ts, &tsLen);

	if (dbConn->Execute(L"INSERT INTO [dbo].[User_Account]([account_id], [password], [createDate]) VALUES(?, ?, ?)"))
	{
		std::cout << "Sign Up" << std::endl;
	}

	_dbConnectionPool->Push(dbConn);
}
	


