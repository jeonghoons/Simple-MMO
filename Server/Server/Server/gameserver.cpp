#include "pch.h"
#include "thread"
#include "ServerService.h"
#include "IocpCore.h"
#include "Room.h"

shared_ptr<DatabaseWorker> GDBWorker = nullptr;
unique_ptr<RoomManager> GRoomManager = nullptr;

void worker_thread(shared_ptr<ServerService> service)
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

	
	GRoomManager = make_unique<RoomManager>(service->GetIocpInstance()->GetHandle());
	GRoomManager->CreateRoom();

	GDBWorker = make_shared<DatabaseWorker>(service->GetIocpInstance()->GetHandle(), 6);

	vector<thread> threads;
	int num_threads = thread::hardware_concurrency();
	// int num_threads = 1;
	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back(worker_thread, service);
	}

	for (thread& t : threads) {
		if(t.joinable()) 
			t.join();
	}

}