#pragma once
#include "pch.h"
#include "JobQueue.h"
#include "ServerService.h"
#include "Timer.h"
#include "GameMap.h"

class Session;
class SendBuffer;
class GameObject;
class Player;
class Monster;

class Room : public enable_shared_from_this<Room>
{
public:
	Room() = default;
	Room(shared_ptr<Timer> timer, HANDLE iocpHandle) : _timer(timer), _jobQueue(make_shared<JobQueue>(iocpHandle)) {}

	~Room() = default;

	void InitRoom();

	template<typename... Arguments>
	void PushJob(void(Room::* memFunc)(Arguments...), Arguments... args)
	{

		_jobQueue->Push(shared_from_this(), memFunc, std::forward<Arguments>(args)...);
	}

	template<typename... Arguments>
	void ReserveJob(DWORD ectime, void(Room::* memFunc)(Arguments...), Arguments... args)
	{
		_timer->Reserve(ectime, _jobQueue, shared_from_this(), memFunc, std::forward<Arguments>(args)...);
	}

	
	
	void EnterRoom(shared_ptr<Player> player);
	void LeaveRoom(shared_ptr<Player> player);
	void Broadcast(shared_ptr<SendBuffer> sendBuffer);

	bool AddObject(shared_ptr<GameObject> object);
	bool RemoveObject(int objectId);
	shared_ptr<GameObject> GetGameObject(int objectId)
	{
		auto it = _objects.find(objectId);
		if (it == _objects.end()) {
			return nullptr;
		}
		return it->second;
	}


	unordered_set<int> GetViewList(int objectId)
	{
		unordered_set<int> candidates = _gameMap.GetObjectIds(objectId);
		unordered_set<int> newView;
		for (int id : candidates) { // 최종 시야
			if (id == objectId) continue;

			/*auto it = _players.find(id);
			if (it == _players.end()) continue;*/

			newView.insert(id);
		}
	}

	void Update();
	void PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time);
	void NPCMove(shared_ptr<Monster> monster);

	int MonsterIdGenerator()
	{
		static atomic<int> _midGenerator = 100000;
		return ++_midGenerator;
	}

	shared_ptr<Player> Id2Player(int pId);
	shared_ptr<Monster> Id2Monster(int mId);

	PositionInfo RandomPos();

	int NumPlayers() { return static_cast<int>(_players.size()); }

private:
	
	unordered_map<int, shared_ptr<GameObject>> _objects;
	unordered_map<int, shared_ptr<Player>> _players;
	unordered_map<int, shared_ptr<Monster>> _monsters;

	GameMap				_gameMap;

	shared_ptr<JobQueue> _jobQueue;

	shared_ptr<Timer>		_timer;
	
};

class RoomManager
{
public:

	void CreateRoom();
	void Remove(int roomId);
	void Remove(shared_ptr<Room> room);

	void EnterPlayer(shared_ptr<Player> player);

	int IdGenerator();

	void SetIocpHandle(shared_ptr<ServerService> service) { 
		_iocpHandle = service->GetIocpInstance()->GetHandle();
	}

public:
	
	RWLock				_lock;
	
	unordered_map<int, shared_ptr<Room>> _rooms;

	HANDLE _iocpHandle = nullptr;
};

