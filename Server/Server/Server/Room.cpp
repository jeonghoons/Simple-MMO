#include "pch.h"
#include "Room.h"
#include "Session.h"
#include "PacketHandler.h"
#include "Player.h"
#include "Monster.h"



HANDLE Room::GetHandle()
{
	return HANDLE();
}

void Room::Dispatch(IocpEvent* iocpEvent, int numBytes)
{

	if (iocpEvent->_type == EventType::Job)
		_jobQueue.ExecuteJobs();

	
	iocpEvent->_owner = nullptr;
	delete iocpEvent;
}

void Room::InitRoom()
{
	Update();
}



void Room::EnterRoom(shared_ptr<Player> player)
{

	/*player->SetOwnerRoom(static_pointer_cast<Room>(shared_from_this()));
	player->SetPosition(RandomPos());
	_players[player->GetId()] = player;


	PushJob(&Room::SendEnteredPlayer, player->GetId());*/
	//SendEnteredPlayer(player->GetId());




	/*int dataSize = sizeof(SC_ADD_OBJECT_PACKET) * _monsters.size();
	vector<BYTE> buf(dataSize);
	int bufIndex{};
	
	for (auto& [id, monster] : _monsters) {
		SC_ADD_OBJECT_PACKET addPacket;
		addPacket.header = { sizeof(addPacket), SC_ADD_OBJECT };
		addPacket.objectId = id;
		addPacket.position = monster->GetPosition();
		memcpy(&buf[bufIndex], &addPacket, sizeof(addPacket));
		bufIndex += sizeof(addPacket);
	}

	shared_ptr<SendBuffer> npcSendBuffer = make_shared<SendBuffer>(dataSize);
	npcSendBuffer->CopyData(buf.data(), dataSize);

	player->GetSession()->Send(npcSendBuffer);*/

	
}

void Room::SendEnteredPlayer(shared_ptr<Player> player)
{
	// RWLock::WriteGuard lock(_lock);
	_players.insert({ player->GetId(), player });
	
	SC_LOGIN_INFO_PACKET logInPacket;
	logInPacket.header = { sizeof(SC_LOGIN_INFO_PACKET), SC_LOGIN };
	logInPacket.player.id = player->GetId();
	logInPacket.player.position = player->GetPosition();

	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(logInPacket));
	sendBuffer->CopyData(&logInPacket, sizeof(logInPacket));

	if (auto pl = player->GetSession()) {
		pl->Send(sendBuffer);
	}

	{ // 들어온 플레이어 시야처리
		for (auto& [id, oldplayer] : _players) {
			if (player->GetId() == id) continue;
			if (false == canSee(player->GetId(), id)) continue;
			

			_players[id]->_viewList.insert(player->GetId());
			oldplayer->GetSession()->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_ADD_PLAYER));

			player->_viewList.insert(id);
			player->GetSession()->Send(PacketHandler::MakePacket(oldplayer, SC_PACKET_LIST::SC_ADD_PLAYER));

		}
	}
}

void Room::LeaveRoom(shared_ptr<Player> player)
{
	// cout << "PLayer[" << player->GetId() << "] Leave" << endl;
	shared_ptr<SendBuffer> sendBuffer = PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_REMOVE_PLAYER);
	Broadcast(sendBuffer);

	
	_players.erase(player->GetId());

}

void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	// RWLock::WriteGuard lock(_lock);
	for (const auto& p : _players)
	{
		if(auto session = p.second->GetSession())
			session->Send(sendBuffer);
	}
}

bool Room::AddObject(shared_ptr<Monster> object)
{
	_monsters.insert(make_pair(object->GetId(), object));
	object->SetOwnerRoom(static_pointer_cast<Room>(shared_from_this()));
	object->SetPosition(RandomPos());

	
	return true;
}

bool Room::RemoveObject(int objectId)
{
	return false;
}

void Room::Update()
{
	cout << "Update Room" << endl;

}

void Room::PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time)
{
	
	if (_players.find(player->GetId()) == _players.end())
		return;
	pair<int, int> pos = player->GetPosition();
	player->_last_moveTime = move_time;

	switch (direction)
	{
	case 2: // left
		pos.first -= 1;
		break;
	case 3: // right
		pos.first += 1;
		break;
	case 0: // up
		pos.second -= 1;
		break;
	case 1: // down
		pos.second += 1;
		break;

	default:
		cout << "Move Error" << endl;
		break;
	}

	player->SetPosition(pos);

	/*SC_MOVE_PACKET packet;
	packet.header = { sizeof(packet), SC_MOVE_OBJECT };
	packet.position = pos;
	packet.id = player->GetId();
	packet.move_time = move_time;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, sizeof(packet));
	Broadcast(sendBuffer);*/

	if (auto p = player->GetSession()) {
		p->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_MOVE_OBJECT));
	}

	unordered_set<int> near_list;
	unordered_set<int> old_list = player->_viewList;
	unordered_map<int, shared_ptr<Player>> players = _players;


	for (auto& [id, pl] : _players) { // 현재의 시야 생성
		if (id == player->GetId()) continue;
		if (canSee(player->GetId(), id))
			near_list.insert(id);
	}

	
	
	for (int id : near_list) {
		if (_players[id]->_viewList.count(player->GetId())) {// 상대에 내가있으면 move
			if (auto session = _players[id]->GetSession()) {
				session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_MOVE_OBJECT));
			}
		}
		else {// 없으면 add
			{
				
				_players[id]->_viewList.insert(player->GetId());
			}
			if(auto session = _players[id]->GetSession())
				session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_ADD_PLAYER));
		}
		if (old_list.count(id) == 0) {// 전 시야엔 없지만 지금은 있으면 move
			{
				
				player->_viewList.insert(id);
			}
			if (auto p = player->GetSession()) {
				p->Send(PacketHandler::MakePacket(_players[id], SC_PACKET_LIST::SC_ADD_PLAYER));
			}
		}
	}

	for (int id : old_list) {
		if (near_list.count(id) == 0) { // 현재시야에도 전시야사람이 없으면 서로 remove


			if (auto p = player->GetSession()) {
				player->_viewList.erase(id);
				p->Send(MAKE_SC_REMOVE_PLAYER(id));
			}

			auto it = _players.find(id);
			if (it != _players.end()) {
				if (auto p = _players[id]->GetSession()) {
					_players[id]->_viewList.erase(player->GetId());
					p->Send(MAKE_SC_REMOVE_PLAYER(player->GetId()));
				}
			}
		}
	}

	
	
}

void Room::NPCMove()
{

	auto monsters = _monsters;

	for (auto monster : monsters) {
		
		int x = monster.second->GetPosition().first;
		int y = monster.second->GetPosition().second;
		switch (rand() % 4) {
		case 0:
			if (x < (16 - 1)) x++; break;
		case 1:
			if (x > 0) x--; break;
		case 2:
			if (y < (16 - 1)) y++; break;
		case 3:
			if (y > 0) y--; break;
		}
		monster.second->SetPosition({ x, y });
	}
		

	int dataSize = sizeof(SC_MOVE_PACKET) * _monsters.size();
	vector<BYTE> buf(dataSize);
	int bufIndex{};

	for (auto& [id, monster] : _monsters) {
		SC_MOVE_PACKET movePacket;
		movePacket.header = { sizeof(movePacket), SC_MOVE_OBJECT };
		movePacket.id = id;
		movePacket.position = monster->GetPosition();
		memcpy(&buf[bufIndex], &movePacket, sizeof(movePacket));
		bufIndex += sizeof(movePacket);
	}

	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(bufIndex);
	Broadcast(sendBuffer);

}

bool Room::canSee(int from, int to)
{
	if (abs(_players[from]->GetPosition().first - _players[to]->GetPosition().first) > 5) return false;
	return abs(_players[from]->GetPosition().second - _players[to]->GetPosition().second) <= 5;
}

bool Room::canSee(pair<int, int> from, pair<int, int> to)
{
	if (abs(from.first - to.first) > 5) return false;
	return abs(from.second - to.second) <= 5;
}

pair<int, int> Room::RandomPos()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<int> dist(0, 400);

	int x = dist(gen);
	int y = dist(gen);

	return { x, y };
}

void RoomManager::CreateRoom()
{
	int id = IdGenerator();

	shared_ptr<Room> room = make_shared<Room>(_iocpHandle);
	_rooms.insert({ id, room });
	room->InitRoom();
}

void RoomManager::Remove(int roomId)
{
	// RWLock::WriteGuard lock(_lock);
	_rooms.erase(roomId);
}

void RoomManager::Remove(shared_ptr<Room> room)
{
	return;
}

void RoomManager::EnterPlayer(shared_ptr<Player> player)
{
	while (true) {
		{
			RWLock::WriteGuard lock(_lock);
			for (auto& [id, room] : _rooms) {
				if (room->NumPlayers() < MAX_ROOM_CAPACITY) {
					player->SetOwnerRoom(room);
					player->SetPosition(room->RandomPos());

					// room->SendEnteredPlayer(player);
					room->PushJob(&Room::SendEnteredPlayer, player);
					return; // 성공 시 바로 종료
				}
			}

			// 빈 방이 없음 -> 새 방 생성
			CreateRoom();
		}

		// lock 해제된 상태에서 다시 while 재시도
		// (CreateRoom에서 _rooms에 새로운 방 추가됨)
		// 다음 루프에서 새로 생성된 방에 들어가게 됨
	}
}

int RoomManager::IdGenerator()
{
	static atomic<int> _idGenerator = 0;
	return ++_idGenerator;
}
