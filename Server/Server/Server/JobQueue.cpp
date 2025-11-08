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

	iocpEvent->_owner = nullptr;
	delete iocpEvent;
	
}
