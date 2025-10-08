#pragma once
#include "pch.h"
#include "JobQueue.h"
#include "ServerService.h"
#include "Timer.h"

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

	/*template<typename... Arguments>
	void PushJob(void(Room::* memFunc)(Arguments...), Arguments... args)
	{
		shared_ptr<Job> job = make_shared<Job>(static_pointer_cast<Room>(shared_from_this()), memFunc, std::forward<Arguments>(args)...);
		_jobQueue->Push(job);
	}*/

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
	void SendEnteredPlayer(shared_ptr<Player> player);
	void LeaveRoom(shared_ptr<Player> player);
	void Broadcast(shared_ptr<SendBuffer> sendBuffer);

	bool AddObject(shared_ptr<Monster> object);
	bool RemoveObject(int objectId);

	void Update();
	void PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time);
	void NPCMove();

	bool canSee(int from, int to);
	bool canSee(pair<int, int> from, pair<int, int> to);

	int MonsterIdGenerator()
	{
		static atomic<int> _midGenerator = 10000;
		return ++_midGenerator;
	}

	shared_ptr<Player> Id2Player(int pId) { return _players[pId]; }

	pair<int, int> RandomPos();

	int NumPlayers() { return static_cast<int>(_players.size()); }

	RWLock		_lock;
	

private:
	/// <vector<bool>> _tileMap;
	// map<int, shared_ptr<GameObject>> _objects;
	RWLock			_vlLock;
	unordered_map<int, shared_ptr<Player>> _players;
	map<int, shared_ptr<Monster>> _monsters;

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

