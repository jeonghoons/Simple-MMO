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

	// GTimerQueue->GetInstance(service);

	vector<thread> threads;
	// int num_threads = thread::hardware_concurrency();
	int num_threads = 6;
	for (int i = 0; i < num_threads; ++i) {
		
		Lthreadid++;
		threads.emplace_back(worker_thread, std::ref(service));
	}
	
	
	
	// cout << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()) << endl;
	
	// timerThread.join();
	for (thread& t : threads) {
		t.join();
	}

	delete GRoomManager;
}