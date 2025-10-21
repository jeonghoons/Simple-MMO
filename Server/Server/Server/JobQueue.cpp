#include "pch.h"
#include "JobQueue.h"

HANDLE JobQueue::GetHandle()
{
	return HANDLE();
}

void JobQueue::Dispatch(IocpEvent* iocpEvent, int numBytes)
{
	if (iocpEvent->_type == EventType::Job)
		ExecuteJobs();

	if (--_evCount != 0) cout << _evCount << endl;
	iocpEvent->_owner = nullptr;
	delete iocpEvent;
	
}
