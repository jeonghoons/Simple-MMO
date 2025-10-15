#include "pch.h"
#include "Room.h"
#include "Session.h"
#include "PacketHandler.h"
#include "PacketSerializer.h"
#include "Player.h"
#include "Monster.h"


void Room::InitRoom()
{
	Update();
}



void Room::EnterRoom(shared_ptr<Player> player)
{
	_players.insert({ player->GetId(), player });
	SC_LOGIN_INFO_PACKET logInPacket;
	logInPacket.header = { sizeof(SC_LOGIN_INFO_PACKET), SC_LOGIN };
	logInPacket.objectInfo = player->GetInfo();
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(logInPacket));
	sendBuffer->CopyData(&logInPacket, sizeof(logInPacket));

	if (auto pl = player->GetSession()) {
		pl->Send(sendBuffer);
	}

	_gameMap.UpdateObjectPosition(player->GetId(), player->GetPosition()); // 게임맵에 등록
	unordered_set<int> candidates = _gameMap.GetObjectIds(player->GetId()); // 후보 
	std::unordered_set<int> newView;
	
	for (int target_id : candidates) {
		if (player->GetId() == target_id) continue;

		auto it = _players.find(target_id);
		if (it == _players.end()) continue;

		newView.insert(target_id);
	}

	for (int target_id : newView) {

		auto target_it = _players.find(target_id);
		if (target_it == _players.end()) continue;
		shared_ptr<Player> target_player = target_it->second;

		player->_viewList.insert(target_id);
		if (auto session = player->GetSession()) {
			session->Send(PacketSerializer::MAKE_SC_ADD_PLAYER(target_player));
		}

		target_player->_viewList.insert(player->GetId());
		if (auto session = target_player->GetSession()) {
			session->Send(PacketSerializer::MAKE_SC_ADD_PLAYER(player));
		}
	}

}

void Room::LeaveRoom(shared_ptr<Player> player)
{
	// cout << "PLayer[" << player->GetId() << "] Leave" << endl;
	shared_ptr<SendBuffer> sendBuffer = PacketSerializer::MAKE_SC_REMOVE_PLAYER(player->GetId());
	Broadcast(sendBuffer);


	_players.erase(player->GetId());

}

void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	for (const auto& p : _players)
	{
		if (auto session = p.second->GetSession())
			session->Send(sendBuffer);
	}
}

bool Room::AddObject(shared_ptr<Monster> object)
{
	_monsters.insert(make_pair(object->GetId(), object));
	object->SetOwnerRoom(static_pointer_cast<Room>(shared_from_this()));
	// object->SetPosition(RandomPos());


	return true;
}

bool Room::RemoveObject(int objectId)
{
	return false;
}

void Room::Update()
{
	// cout << "Update Room" << endl;
	ReserveJob(1000, &Room::Update);

}


void Room::PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time)
{
	if (_players.find(player->GetId()) == _players.end())
		return;
	PositionInfo pos = player->GetPosition();

	switch (direction)
	{
	case 2: // left
		pos.x -= 1;
		break;
	case 3: // right
		pos.x += 1;
		break;
	case 0: // up
		pos.y -= 1;
		break;
	case 1: // down
		pos.y += 1;
		break;
	default:
		cout << "Move Error" << endl;
		return;
	}

	player->_last_moveTime = move_time;
	player->SetPosition(pos);

	if (auto p = player->GetSession()) { // 자신은 이동
		p->Send(PacketSerializer::MAKE_SC_MOVE_OBJECT(player));
	}

	unordered_set<int> oldView = player->_viewList; // 이전 시야
	bool changeCell = _gameMap.UpdateObjectPosition(player->GetId(), pos); // 셀 업데이트

	if (changeCell) {
		unordered_set<int> candidates = _gameMap.GetObjectIds(player->GetId()); // 시야 후보
		unordered_set<int> newView; // 최종 시야를 저장

		for (int id : candidates) { // 최종 시야
			if (id == player->GetId()) continue;

			auto it = _players.find(id);
			if (it == _players.end()) continue;

			newView.insert(id);
		}
		for (int old_id : oldView) { // 있 -> 없 제거
			if (newView.count(old_id) == 0) {

				if (auto session = player->GetSession()) {
					session->Send(PacketSerializer::MAKE_SC_REMOVE_PLAYER(old_id));
				}

				auto it = _players.find(old_id);
				if (it != _players.end() && it->second->_viewList.erase(player->GetId()) > 0) {
					if (auto session = it->second->GetSession()) {
						session->Send(PacketSerializer::MAKE_SC_REMOVE_PLAYER(player->GetId()));
					}
				}
			}
		} 
		for (int new_id : newView) { // 새로 보이면 서로 추가
			if (oldView.count(new_id) == 0) {
				if (auto session = player->GetSession()) {
					session->Send(PacketSerializer::MAKE_SC_ADD_PLAYER(_players[new_id]));
				}

				auto it = _players.find(new_id);
				if (it != _players.end() && it->second->_viewList.insert(player->GetId()).second) {
					if (auto session = it->second->GetSession()) {
						session->Send(PacketSerializer::MAKE_SC_ADD_PLAYER(player));
					}
				}
			}
		}

		for (auto id : newView) {
			if (oldView.count(id) > 0) {
				if (auto session = _players[id]->GetSession()) {
					session->Send(PacketSerializer::MAKE_SC_MOVE_OBJECT(player));
				}
			}
		}
		player->_viewList = std::move(newView);
	}

	unordered_set<int>& currentView = player->_viewList;
	for (int id : currentView) {
		auto it = _players.find(id);
		if (it != _players.end()) {
			if (auto session = it->second->GetSession()) {
				session->Send(PacketSerializer::MAKE_SC_MOVE_OBJECT(player));
			}
		}
	}

}



void Room::NPCMove()
{

	/*auto monsters = _monsters;

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
	Broadcast(sendBuffer);*/

}

bool Room::canSee(int from, int to)
{
	if (abs(_players[from]->GetPosition().x - _players[to]->GetPosition().x) > 5.f) return false;
	return abs(_players[from]->GetPosition().y - _players[to]->GetPosition().y) <= 5.f;
}

PositionInfo Room::RandomPos()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	// static std::uniform_real_distribution<float> dist(0.f, 400.f);
	static std::uniform_real_distribution<float> dist(0.f, 99.f);

	float x = dist(gen);
	float y = dist(gen);
	// float z = dist(gen);

	return { x, y, 0.f, 0.f };
}

void RoomManager::CreateRoom()
{
	int id = IdGenerator();

	shared_ptr<Room> room = make_shared<Room>(GTimer, _iocpHandle);
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

					room->PushJob(&Room::EnterRoom, player);
					return; // 성공 시 바로 종료
				}
			}

			// 빈 방이 없음 -> 새 방 생성
			CreateRoom();
		}

	}
}

int RoomManager::IdGenerator()
{
	static atomic<int> _idGenerator = 0;
	return ++_idGenerator;
}
