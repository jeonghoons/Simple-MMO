#pragma once
#include "pch.h"
#include "JobQueue.h"

class Session;
class SendBuffer;
class GameObject;
class Player;
class Monster;

class Room : public JobQueue
{
public:
	Room() = default;
	~Room() = default;

	virtual void FlushJob() override;

	void InitRoom();
	
	void EnterRoom(shared_ptr<Player> player);
	void LeaveRoom(shared_ptr<Player> player);
	void Broadcast(shared_ptr<SendBuffer> sendBuffer);

	bool AddObject(shared_ptr<Monster> object);
	bool RemoveObject(int objectId);

	void Update();
	void PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time);
	void NPCMove();

	bool canSee(int from, int to);

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
	map<int, shared_ptr<Player>> _players;
	map<int, shared_ptr<Monster>> _monsters;
	
};

// extern shared_ptr<Room> GRoom;

class RoomManager
{
public:
	void CreateRoom();
	void Remove(int roomId);
	void Remove(shared_ptr<Room> room);

	void EnterPlayer(shared_ptr<Player> player);

	int IdGenerator();

public:
	
	RWLock				_lock;
	unordered_map<int, shared_ptr<Room>> _rooms;
};

extern shared_ptr<RoomManager> GRoomManager;
