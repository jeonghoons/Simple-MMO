#pragma once
#include "pch.h"
#include <concurrent_priority_queue.h>
#include "Room.h"
#include "ServerService.h"

enum Timer_EventType { EV_NPC_MOVE, EV_UPDATE_ROOM };

struct TimerItem
{
	bool operator < (const TimerItem& other) const
	{
		return (other._excuteTime < _excuteTime);
	}

	std::chrono::system_clock::time_point _excuteTime;
	Timer_EventType _type{};

	shared_ptr<Room>		_ownerRoom;

};

class TimerObject : IocpObject
{
	using clock = std::chrono::system_clock::time_point;
public:
	TimerObject() {};
	TimerObject(TimerItem item);
	~TimerObject() = default;

	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int numBytes = 0) override;

public:
	TimerEvent timerEvent;

	TimerItem _item;
};


class TimerQueue
{
public:
	void TimerRun(shared_ptr<ServerService>& service)
	{
		while (true) {
			TimerItem event;
			auto currentTime = std::chrono::system_clock::now();
			if (_timerQueue.try_pop(event)) {
				// 큐가 비었거나, 팝 성공
				if (event._excuteTime > currentTime) {
					// 실행시간이 아직 안되었으므로 잠시 대기
					_timerQueue.push(event);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				// 팝 성공
				switch (event._type)
				{

				case Timer_EventType::EV_UPDATE_ROOM: {
					// shared_ptr<TimerObject> obj = event._room;
					// RoomUpdateEvent* roomEvent = new RoomUpdateEvent();
					IocpEvent* over = new IocpEvent(EventType::Timer);
					over->Init();
					// over->owner = this;

					PostQueuedCompletionStatus(_iocpInstance->GetHandle(), 1, 0, reinterpret_cast<LPOVERLAPPED>(over));
				}	break;

				case Timer_EventType::EV_NPC_MOVE: {
					/*shared_ptr<Room> room = make_shared<Room>();
					NpcMoveEvent* moveEvent = new NpcMoveEvent();
					moveEvent->Init();
					moveEvent->owner = room;

					PostQueuedCompletionStatus(_iocpInstance->GetHandle(), 1, 0, reinterpret_cast<LPOVERLAPPED>(moveEvent));
				}	break;*/


				default:
					break;
				}
												 continue; // 다음 작업 꺼내기
				}
			}

			std::this_thread::sleep_for(1ms);
		}
	}

	void GetInstance(shared_ptr<ServerService>& service)
	{
		_iocpInstance = service->GetIocpInstance();
	}

public:

	shared_ptr<IocpCore> _iocpInstance;
	concurrency::concurrent_priority_queue<TimerItem> _timerQueue;
};

extern shared_ptr<TimerQueue> GTimerQueue;


