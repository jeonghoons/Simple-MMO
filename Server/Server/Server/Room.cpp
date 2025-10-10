#include "pch.h"
#include "Room.h"
#include "Session.h"
#include "PacketHandler.h"
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
	unordered_set<int> candidates = _gameMap.GetAOIObjectIds(player->GetId()); // 후보 
	std::unordered_set<int> newView;
	const PositionInfo& my_pos = player->GetPosition();

	for (int target_id : candidates) {
		if (player->GetId() == target_id) continue;

		auto it = _players.find(target_id);
		if (it == _players.end()) continue;

		const PositionInfo& target_pos = it->second->GetPosition();

		float dx = abs(my_pos.pos_x - target_pos.pos_x);
		float dy = abs(my_pos.pos_y - target_pos.pos_y);

		if (dx <= VIEW_RANGE && dy <= VIEW_RANGE) {
			newView.insert(target_id);
		}
	}

	for (int target_id : newView) {

		auto target_it = _players.find(target_id);
		if (target_it == _players.end()) continue;
		shared_ptr<Player> target_player = target_it->second;

		player->_viewList.insert(target_id);
		if (auto session = player->GetSession()) {
			session->Send(PacketHandler::MakePacket(target_player, SC_PACKET_LIST::SC_ADD_PLAYER));
		}

		target_player->_viewList.insert(player->GetId());
		if (auto session = target_player->GetSession()) {
			session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_ADD_PLAYER));
		}
	}



	//{ // 들어온 플레이어 시야처리
	//	for (auto& [id, oldplayer] : _players) {
	//		if (player->GetId() == id) continue;
	//		if (false == canSee(player->GetId(), id)) continue;


	//		_players[id]->_viewList.insert(player->GetId());
	//		oldplayer->GetSession()->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_ADD_PLAYER));

	//		player->_viewList.insert(id);
	//		player->GetSession()->Send(PacketHandler::MakePacket(oldplayer, SC_PACKET_LIST::SC_ADD_PLAYER));

	//	}
	//}
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
	player->_last_moveTime = move_time;

	switch (direction)
	{
	case 2: // left
		// pos.first -= 1;
		pos.pos_x -= 1;
		break;
	case 3: // right
		// pos.first += 1;
		pos.pos_x += 1;
		break;
	case 0: // up
		// pos.second -= 1;
		pos.pos_y -= 1;
		break;
	case 1: // down
		// pos.second += 1;
		pos.pos_y += 1;
		break;

	default:
		cout << "Move Error" << endl;
		break;
	}

	player->SetPosition(pos);

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
			if (auto session = _players[id]->GetSession())
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

void Room::PlayerMoven(shared_ptr<Player> player, int direction, unsigned move_time)
{
	if (_players.find(player->GetId()) == _players.end())
		return;
	PositionInfo pos = player->GetPosition();

	switch (direction)
	{
	case 2: // left
		pos.pos_x -= 1;
		break;
	case 3: // right
		pos.pos_x += 1;
		break;
	case 0: // up
		pos.pos_y -= 1;
		break;
	case 1: // down
		pos.pos_y += 1;
		break;
	default:
		cout << "Move Error" << endl;
		return;
	}

	player->_last_moveTime = move_time;
	player->SetPosition(pos);

	if (auto p = player->GetSession()) {
		p->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_MOVE_OBJECT));
	}

	unordered_set<int> oldView = player->_viewList; // 이전 시야
	bool changeCell = _gameMap.UpdateObjectPosition(player->GetId(), pos);


	unordered_set<int> candidates = _gameMap.GetAOIObjectIds(player->GetId()); // 시야 후보
	unordered_set<int> newView; // 최종 시야를 저장

	for (int id : candidates) { // 최종 시야
		if (id == player->GetId()) continue;

		auto it = _players.find(id);
		if (it == _players.end()) continue;

		const PositionInfo& target_pos = it->second->GetPosition();

		float dx = abs(pos.pos_x - target_pos.pos_x);
		float dy = abs(pos.pos_y - target_pos.pos_y);

		if (dx <= VIEW_RANGE && dy <= VIEW_RANGE) {
			newView.insert(id);
		}
	}

	for (int old_id : oldView) { // 있 -> 없 제거
		if (newView.count(old_id) == 0) {
			// 1) 나(player)의 시야에서 객체 old_id 제거 -> 나에게 REMOVE(old_id) 전송
			if (auto session = player->GetSession()) {
				session->Send(MAKE_SC_REMOVE_PLAYER(old_id)); // 올바른 대상 ID
			}

			// 2) 객체 B(old_id)의 시야에서 나(player->GetId()) 제거 -> B에게 REMOVE(Player ID) 전송
			auto it = _players.find(old_id);
			if (it != _players.end() && it->second->_viewList.erase(player->GetId()) > 0) {
				if (auto session = it->second->GetSession()) {
					session->Send(MAKE_SC_REMOVE_PLAYER(player->GetId())); // 올바른 대상 ID
				}
			}
		}
	}

	for (int new_id : newView) { // 새로 보이면 서로 추가
		if (oldView.count(new_id) == 0) {
			// 1) 나(player)의 시야에 객체 B(new_id) 추가 -> 나에게 ADD(B 정보) 전송
			if (auto session = player->GetSession()) {
				session->Send(PacketHandler::MakePacket(_players[new_id], SC_PACKET_LIST::SC_ADD_PLAYER));
			}

			// 2) 객체 B(new_id)의 시야에 나(player) 추가 -> B에게 ADD(Player 정보) 전송
			auto it = _players.find(new_id);
			if (it != _players.end() && it->second->_viewList.insert(player->GetId()).second) {
				if (auto session = it->second->GetSession()) {
					session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_ADD_PLAYER));
				}
			}
		}
	}

	for (auto id : newView) { // 
		if (oldView.count(id) > 0) { // KEEP 대상에게만 MOVE 패킷 전송
			if (auto session = _players[id]->GetSession()) {
				session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_MOVE_OBJECT));
			}
		}
	}
	player->_viewList = std::move(newView);



	for (int old_id : oldView) {
		auto it = _players.find(old_id);
		if (it != _players.end()) {
			if (auto session = it->second->GetSession()) {
				session->Send(PacketHandler::MakePacket(player, SC_PACKET_LIST::SC_MOVE_OBJECT));
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
	if (abs(_players[from]->GetPosition().pos_x - _players[to]->GetPosition().pos_x) > 5.f) return false;
	return abs(_players[from]->GetPosition().pos_y - _players[to]->GetPosition().pos_y) <= 5.f;
}

PositionInfo Room::RandomPos()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	// static std::uniform_real_distribution<float> dist(0.f, 400.f);
	static std::uniform_real_distribution<float> dist(0.f, 32.f);

	float x = dist(gen);
	float y = dist(gen);
	float z = dist(gen);

	return { x, y, z, 0.f };
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

					// room->SendEnteredPlayer(player);
					room->PushJob(&Room::EnterRoom, player);
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
