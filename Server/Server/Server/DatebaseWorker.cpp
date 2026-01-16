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

	_dbConnectionPool->Push(dbConn);

	int playerId = session->GetId();
	shared_ptr<Player> player = make_shared<Player>(session);
	player->SetId(playerId);
	session->_currPlayer = player;

	GRoomManager->EnterPlayer(player);

}
	


