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

	/*const WCHAR* connectionPath = L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\MSSQLLocalDB;Database=SimpleMMO;Trusted_Connection=Yes;";
	if (false == GDBConnectionPool->Connect(num_threads, connectionPath))
	{
		std::cout << "DB Connect 오류 !" << std::endl;
		exit(-1);
	}
	else
	{
		std::cout << "DB 서버 Connected" << std::endl;
	}*/
	
	for (thread& t : threads) {
		if(t.joinable()) 
			t.join();
	}

}