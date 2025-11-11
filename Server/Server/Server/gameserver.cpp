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

	if (false == GDBConnectionPool->Connect(6, L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=SimpleMMO;Trusted_Connection=Yes;"))
	{
		std::cout << "DB Connect ¿À·ù !" << std::endl;
		exit(-1);
	}

	GRoomManager->SetIocpHandle(service);
	GRoomManager->CreateRoom();

	vector<thread> threads;
	int num_threads = thread::hardware_concurrency();
	// int num_threads = 1;
	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back(worker_thread, std::ref(service));
	}


	
	for (thread& t : threads) {
		if(t.joinable()) 
			t.join();
	}

	delete GRoomManager;
}