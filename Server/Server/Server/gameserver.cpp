#include "pch.h"
#include "thread"
#include "ServerService.h"
#include "IocpCore.h"
#include "Room.h"


void worker_thread(shared_ptr<ServerService>& service)
{
	while (true)
	{
		service->GetIocpInstance()->Dispatch();
	}
}

int main()
{
	shared_ptr<ServerService> service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", PORT_NUM),
		make_shared<IocpCore>()
	);

	if (service->Start() == false) {
		cout << "Can't Start" << endl;
		exit(-1);
	}
	else
		cout << "Service Start" << endl;

	GRoomManager->SetIocpHandle(service);
	GRoomManager->CreateRoom();

	vector<thread> threads;
	int num_threads = thread::hardware_concurrency();
	// int num_threads = 1;
	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back(worker_thread, ref(service));
	}

	{
		auto query = L"									\
			CREATE TABLE [dbo].[User_Account]					\
			(											\
				[id] INT NOT NULL PRIMARY KEY IDENTITY, \
				[account_id] NVARCHAR(20) NOT NULL UNIQUE, \
				[password] NVARCHAR(20) NOT NULL,						\
				[createDate] DATETIME NULL				\
			);";

		// GDBWorker->PushJob(&DatebaseWorker::TryLogin, session, id, pw);
	}
	
	for (thread& t : threads) {
		if(t.joinable()) 
			t.join();
	}

}