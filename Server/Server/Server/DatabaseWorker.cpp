#include "pch.h"
#include "DatabaseWorker.h"
#include "Player.h"

DatabaseWorker::DatabaseWorker(HANDLE iocpHandle, int num_connections) : _iocpHandle(iocpHandle)
{
	// const WCHAR* connectionPath = L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=SimpleMMO;Trusted_Connection=Yes;";
	const WCHAR* connectionPath = L"Driver={ODBC Driver 17 for SQL Server};Server=DESKTOP-0I9E8CG\\SQLEXPRESS;Database=SimpleMMO;Trusted_Connection=Yes;";
	_dbConnectionPool = make_unique<DBConnectionPool>();
	if (false == _dbConnectionPool->Connect(num_connections, connectionPath))
	{
		std::cout << "DB Connect 오류 !" << std::endl;
		exit(-1);
	}
	std::cout << "DB 서버 Connected" << std::endl;

	for (int i = 0; i < num_connections; ++i) {
		_threads.emplace_back(&DatabaseWorker::Run, this);
	}
}

DatabaseWorker::~DatabaseWorker()
{
	for (auto& t : _threads) {
		if (t.joinable()) t.join();
	}
}

void DatabaseWorker::Run()
{
	while (true)
	{
		shared_ptr<Job> job = nullptr;

		_dbJobQueue.WaitPop(job);

		if (job) {
			job->Execute();
		}
	}
}


void DatabaseWorker::TryLogin(shared_ptr<Session> session, string recvId, string recvPw)
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

	int outPlayerType = 0;
	SQLLEN outPlayerTypeLen = 0;
	dbConn->BindCol(4, &outPlayerType, &outPlayerTypeLen);

	if (dbConn->Execute(L"SELECT id, account_id, password, playerType FROM [dbo].[User_Account] WHERE account_id = (?)"))
	{
		if (dbConn->Fetch())
		{
			WCHAR clpassword[20] = {};
			MultiByteToWideChar(CP_ACP, 0, recvPw.c_str(), -1, clpassword, _countof(clpassword));
			if (lstrcmpW(clpassword, outpassword) == 0)
			{
				GLobby->PushJob(&AuthLobby::OnLoginSuccess, session, wstring(outName), outPlayerType);
			}
			else
			{
				GLobby->PushJob(&AuthLobby::OnLoginFailed, session, (string)"비밀 번호 오류");
			}
		}
		else
		{
			GLobby->PushJob(&AuthLobby::OnLoginFailed, session, (string)"DB에 존재하지 않는 ID");
		}
	}
	_dbConnectionPool->Push(dbConn);
}

void DatabaseWorker::TrySignUP(shared_ptr<Session> session, string recvId, string recvPw, int playerType)
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

	SQLLEN typeLen = 0;
	dbConn->BindParam(3, &playerType, &typeLen);

	if (dbConn->Execute(L"INSERT INTO [dbo].[User_Account]([account_id], [password], [playerType], [createDate]) VALUES(?, ?, ?, GETDATE())"))
	{
		cout << "[DB] Sign Up 성공 - ID : " << recvId << ", PW : " << recvPw << std::endl;
	}
	else
	{
		cout << "[DB] 중복 아이디" << endl;
	}

	_dbConnectionPool->Push(dbConn);
}
	


