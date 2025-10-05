#pragma once
#include "JobQueue.h"
#include <concurrent_priority_queue.h>

enum class TimerEvent : unsigned char
{
	RoomUpdate,

};

struct JobItem
{

	weak_ptr<JobQueue>	_owner;
	shared_ptr<Job>		_job;
};

struct TimerItem
{
	bool operator < (const TimerItem& other) const
	{
		return other._executeTIme < _executeTIme;
	}


	chrono::system_clock::time_point _executeTIme{};
	JobItem* _JobItem = nullptr;
};

class JobTimer
{
public:
	void		Reserve();
	

private:
	priority_queue<TimerItem> _timerQueue;
	// concurrent_priority_queue<TimerItem> _timerQueue;
};

