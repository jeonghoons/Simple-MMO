#include "pch.h"
#include "TimerQueue.h"

shared_ptr<TimerQueue> GTimerQueue = make_shared<TimerQueue>();



TimerObject::TimerObject(TimerItem item)
{
}

HANDLE TimerObject::GetHandle()
{
	return HANDLE();
}

void TimerObject::Dispatch(IocpEvent* iocpEvent, int numBytes)
{
}
