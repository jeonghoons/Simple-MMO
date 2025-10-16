#include "pch.h"
#include "Room.h"
#include "Session.h"
#include "PacketHandler.h"
#include "PacketSerializer.h"
#include "Player.h"
#include "Monster.h"

const int NUM_MONSTER = 100;
void Room::InitRoom()
{
	for (int i = 0; i < NUM_MONSTER; ++i) {
		shared_ptr<Monster> monster = make_shared<Monster>();
		monster->SetId(MonsterIdGenerator());
		if (false == AddObject(monster))
			break;
	}

	Update();
}



void Room::EnterRoom(shared_ptr<Player> player)
{
	if (false == AddObject(player)) {
		return;
	}

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
			session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_player));
		}

		target_player->_viewList.insert(player->GetId());
		if (auto session = target_player->GetSession()) {
			session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(player));
		}
	}

	player->_viewList = std::move(newView);

	// 몬스터들 정보 전송


}

void Room::LeaveRoom(shared_ptr<Player> player)
{
	// cout << "PLayer[" << player->GetId() << "] Leave" << endl;

	int playerId = player->GetId();

	shared_ptr<SendBuffer> sendBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(player->GetId());

	for (int target_id : player->_viewList) {
		if (auto target_player = Id2Player(target_id)) {
			if (auto session = target_player->GetSession()) {
				session->Send(sendBuffer);
			}
			target_player->_viewList.erase(playerId);
		}
	}


	if (false == RemoveObject(playerId)) {
		cout << "Error" << endl;
	}

	// Broadcast(sendBuffer);
}

void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	for (const auto& p : _players)
	{
		if (auto session = p.second->GetSession())
			session->Send(sendBuffer);
	}
}

bool Room::AddObject(shared_ptr<GameObject> object)
{
	int objectId = object->GetId();

	auto [it, success] = _objects.emplace(objectId, object);
	if (!success) {
		cout << "Error" << endl;
		return false;
	}

	shared_ptr<Player> player = dynamic_pointer_cast<Player>(object);
	if (player) {
		_players.emplace(objectId, player);
	}
	else if (shared_ptr<Monster> monster = dynamic_pointer_cast<Monster>(object)) {
		_monsters.emplace(objectId, monster);
	}
	else {
		cout << "Error" << endl;
		_objects.erase(objectId);
		return false;
	}

	object->SetOwnerRoom(static_pointer_cast<Room>(shared_from_this()));
	object->SetPosition(RandomPos());

	_gameMap.UpdateObjectPosition(objectId, object->GetPosition());

	return true;
}

bool Room::RemoveObject(int objectId)
{
	_players.erase(objectId);
	_monsters.erase(objectId);

	return _objects.erase(objectId) > 0;
}

void Room::Update()
{
	// cout << "Update Room" << endl;
	
	for (const auto& [id, monster] : _monsters) {
		if (monster->_isActive == false) continue;
		// NPCMove(monster); // DO_AI
	}
		
	
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

	shared_ptr<SendBuffer> playerMoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(player);
	if (auto p = player->GetSession()) { // 자신은 이동
		p->Send(playerMoveBuffer);
	}

	unordered_set<int> oldView = player->_viewList; // 이전 시야
	bool changeCell = _gameMap.UpdateObjectPosition(player->GetId(), pos); // 셀 업데이트

	if (changeCell) {
		unordered_set<int> candidates = _gameMap.GetObjectIds(player->GetId()); // 시야 후보
		unordered_set<int> newView; // 최종 시야를 저장

		for (int id : candidates) { // 최종 시야
			if (id == player->GetId()) continue;

			newView.insert(id);
		}

		for (int old_id : oldView) { // 있었음 -> 없음 제거
			if (newView.count(old_id) == 0) {

				if (auto session = player->GetSession()) {
					if (Id2Player(old_id)) {
						session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(old_id));
					}
					else if (Id2Monster(old_id)) {
						session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(old_id)); // 몬스터
					}
				}


				if (auto target_player = Id2Player(old_id)) {
					if (auto session = target_player->GetSession()) {
						session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(player->GetId()));
					}
				}
			}
		}

		for (int new_id : newView) { // 없었음 -> 있음 추가
			if (oldView.count(new_id) == 0) {

				if (auto session = player->GetSession()) {
					if (auto target_player = Id2Player(new_id)) { // Player 포인터를 바로 가져옴
						session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_player));
					}
					else if (auto target_monster = Id2Monster(new_id)) { // Monster 포인터를 바로 가져옴
						session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_monster));
					}
				}

				if (auto target_obj = GetGameObject(new_id)) {
					if (target_obj->_viewList.insert(player->GetId()).second) {
						if (auto target_player = Id2Player(new_id)) {
							if (auto session = target_player->GetSession()) {
								session->Send(playerMoveBuffer);
							}
						}
					}
				}
			}
		}

		player->_viewList = std::move(newView);
	}

	unordered_set<int>& currentView = player->_viewList;
	for (int target_id : currentView) {
		if (auto target_player = Id2Player(target_id)) {
			if (auto session = target_player->GetSession()) {
				session->Send(playerMoveBuffer);
			}
		}
	}

}



void Room::NPCMove(shared_ptr<Monster> monster)
{

	PositionInfo& pos = monster->GetPosition();
	switch (rand() % 4)
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
	
	monster->SetPosition(pos);

	/*int dataSize = sizeof(SC_MOVE_PACKET) * _monsters.size();
	vector<BYTE> buf(dataSize);
	int bufIndex{};

	for (auto& [id, monster] : _monsters) {
		SC_MOVE_PACKET movePacket;
		movePacket.header = { sizeof(movePacket), SC_MOVE_OBJECT };
		movePacket.id = id;
		movePacket.position = monster->GetPosition();
		memcpy(&buf[bufIndex], &movePacket, sizeof(movePacket));
		bufIndex += sizeof(movePacket);
	}*/


}

shared_ptr<Player> Room::Id2Player(int pId)
{
	auto it = _players.find(pId);
	if (it == _players.end())
		return nullptr;

	return it->second;
}

shared_ptr<Monster> Room::Id2Monster(int mId)
{
	auto it = _monsters.find(mId);
	if (it == _monsters.end())
		return nullptr;

	return it->second;
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
					// player->SetPosition(room->RandomPos());

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
