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
		NpcEnterRoom(monster);
	}
	_gameMap.Init(weak_from_this());
	long long currentTick = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
	Update(currentTick);
}

void Room::Update(long long elapsedTime)
{
	// cout << "Update Room" << endl;
	long long currentTick = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
	ReserveJob(100, &Room::Update, currentTick);

	float dt = (currentTick - elapsedTime) / 1000.f;
	// if (dt <= 0.f || dt > 0.5f) dt = 0.1f;

	for (const auto& object : _objects) {
		object.second->Update(dt);
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
	object->SetOwnerRoom(shared_from_this());
	object->SetPosition({ Utils::GetRandom(0.f, MAP_WIDTH / 2.f),
		Utils::GetRandom(0.f, MAP_WIDTH / 2.f), 500.f, 0.f });

	return true;
}

bool Room::RemoveObject(int objectId)
{
	return _objects.erase(objectId) > 0;
}

void Room::PlayerEnterRoom(shared_ptr<Player> player)
{
	if (false == AddObject(player)) {
		return;
	}
	_players.emplace(player->GetId(), player);

	SC_LOGIN_INFO_PACKET logInPacket;
	logInPacket.header = { sizeof(SC_LOGIN_INFO_PACKET), SC_LOGIN };
	logInPacket.objectInfo = player->GetInfo();
	shared_ptr<SendBuffer> loginInfoBuffer = make_shared<SendBuffer>(sizeof(logInPacket));
	loginInfoBuffer->CopyData(&logInPacket, sizeof(logInPacket));

	if (auto session = player->GetSession()) {
		session->Send(loginInfoBuffer);
	}

	cout << "Client[" << player->GetId() << "] Enter Room" << endl;

	shared_ptr<SendBuffer> objectAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(player);
	if (auto session = player->GetSession()) {
		session->Send(objectAddBuffer);
	}

	ViewUpdate result = _gameMap.EnterMap(player->GetId(), player->GetPosition());
	for (const int target_id : result.entered) {
		if (target_id == player->GetId()) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;
		
		player->_viewList.push_back(target_id);
		if (auto player_session = player->GetSession()) {
			player_session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_obj));
		}

		ObjectInfo::Object_Type targetType = target_obj->GetType();
		if (targetType == ObjectInfo::Player || targetType == ObjectInfo::Monster) {
			auto target_char = static_pointer_cast<Character>(target_obj);

			target_char->_viewList.push_back(player->GetId());

			if (targetType == ObjectInfo::Player) {
				auto target_player = static_pointer_cast<Player>(target_char);
				if (auto target_session = target_player->GetSession()) {
					target_session->Send(objectAddBuffer);
				}
			}
		}
	}
}

void Room::PlayerLeaveRoom(shared_ptr<Player> player)
{
	cout << "PLayer[" << player->GetId() << "] Leave" << endl;

	int playerId = player->GetId();

	shared_ptr<SendBuffer> playerRemoveBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(playerId);

	ViewUpdate result = _gameMap.LeaveMap(playerId);

	for (const int target_id : result.leaved) {
		if (target_id == playerId) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type == ObjectInfo::Object_Type::Player || target_type == ObjectInfo::Object_Type::Monster) {

			auto target_char = static_pointer_cast<Character>(target_obj);
			target_char->RemoveView(playerId);

			if (target_type == ObjectInfo::Object_Type::Player) {
				auto target_player = static_pointer_cast<Player>(target_char);
				if (shared_ptr<Session> target_session = target_player->GetSession()) {
					target_session->Send(playerRemoveBuffer);
				}
			}
		}
	}


	if (false == RemoveObject(playerId)) {
		cout << "Error" << endl;
	}

	_players.erase(playerId);

}

void Room::PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time)
{
	if (nullptr == Id2Player(player->GetId()))
		return;

	PositionInfo pos = player->GetPosition();
	PositionInfo new_pos{};

	switch (direction)
	{
	case 2: // left
		// new_pos.x -= 1;
		new_pos.inputX = -1.f;
		break;
	case 3: // right
		// new_pos.x += 1;
		new_pos.inputX = 1.f;
		break;
	case 0: // up
		// new_pos.y -= 1;
		new_pos.inputY = -1.f;
		break;
	case 1: // down
		// new_pos.y += 1;
		new_pos.inputY = 1.f;
		break;
	default:
		cout << "Move Error" << endl;
		return;
	}

	// player->_last_moveTime = move_time;
	player->SetPosition(new_pos);
	shared_ptr<SendBuffer> playerMoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(player);
	if (shared_ptr<Session> player_session = player->GetSession()) {
		player_session->Send(playerMoveBuffer);
	}
	ViewUpdate result = _gameMap.UpdateMap(player->GetId(), new_pos);

	shared_ptr<SendBuffer> playerRemoveBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(player->GetId());
	for (const int target_id : result.leaved) {
		if (target_id == player->GetId()) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		player->RemoveView(target_id);
		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type == ObjectInfo::Player || target_type == ObjectInfo::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->RemoveView(player->GetId());
			if (shared_ptr<Session> player_session = player->GetSession()) {
				player_session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(target_id));
			}

			if (target_type == ObjectInfo::Player) {
				shared_ptr<Player> target_player = static_pointer_cast<Player>(target_char);
				if (shared_ptr<Session> target_session = target_player->GetSession()) {
					target_session->Send(playerRemoveBuffer);
				}
				
			}
		}
	}

	shared_ptr<SendBuffer> playerAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(player);
	for (const int target_id : result.entered) {
		if (target_id == player->GetId()) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		player->_viewList.push_back(target_id);
		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type == ObjectInfo::Player || target_type == ObjectInfo::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->_viewList.push_back(player->GetId());
			if (shared_ptr<Session> player_session = player->GetSession()) {
				player_session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_obj));
			}

			if (target_type == ObjectInfo::Player) {
				shared_ptr<Player> target_player = static_pointer_cast<Player>(target_char);
				if (shared_ptr<Session> target_session = target_player->GetSession()) {
					target_session->Send(playerAddBuffer);
				}
				
			}
		}
	}

	for (const int id : player->_viewList) {
		shared_ptr<Player> target_player = Id2Player(id);
		if (!target_player) continue;
		if (shared_ptr<Session> target_session = target_player->GetSession()) {
			target_session->Send(playerMoveBuffer);
		}
	}

}

void Room::PlayerCMove(shared_ptr<Player> player, PositionInfo position, bool force)
{
	if (nullptr == Id2Player(player->GetId()))
		return;

	player->SetPosition(position);

	shared_ptr<SendBuffer> playerMoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(player);
	if (shared_ptr<Session> player_session = player->GetSession()) {
		player_session->Send(playerMoveBuffer);
	}

	ViewUpdate result = _gameMap.UpdateMap(player->GetId(), position);

	shared_ptr<SendBuffer> playerRemoveBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(player->GetId());
	for (const int target_id : result.leaved) {
		if (target_id == player->GetId()) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		player->RemoveView(target_id);
		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type == ObjectInfo::Player || target_type == ObjectInfo::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->RemoveView(player->GetId());

			if (target_type == ObjectInfo::Player) {
				shared_ptr<Player> target_player = static_pointer_cast<Player>(target_char);
				if (shared_ptr<Session> target_session = target_player->GetSession()) {
					target_session->Send(playerRemoveBuffer);
				}
				if (shared_ptr<Session> player_session = player->GetSession()) {
					player_session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(target_id));
				}
			}
		}
	}

	shared_ptr<SendBuffer> playerAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(player);
	for (const int target_id : result.entered) {
		if (target_id == player->GetId()) continue;
		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		player->_viewList.push_back(target_id);
		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type == ObjectInfo::Player || target_type == ObjectInfo::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->_viewList.push_back(player->GetId());

			if (target_type == ObjectInfo::Player) {
				shared_ptr<Player> target_player = static_pointer_cast<Player>(target_char);
				if (shared_ptr<Session> target_session = target_player->GetSession()) {
					target_session->Send(playerAddBuffer);
				}
				if (shared_ptr<Session> player_session = player->GetSession()) {
					player_session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_obj));
				}
			}
		}
	}

	for (const int id : player->_viewList) {
		shared_ptr<Player> target_player = Id2Player(id);
		if (!target_player) continue;
		if (shared_ptr<Session> target_session = target_player->GetSession()) {
			target_session->Send(playerMoveBuffer);
		}
	}

	BroadcastAOI(player, playerMoveBuffer);


	// printf("Player[%d] - (%f, %f) Yaw: %f)\n", player->GetId(), position.x, position.y, position.yaw);
}

void Room::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
	for (const auto& p : _players)
	{
		if (auto session = p.second->GetSession())
			session->Send(sendBuffer);
	}
}

void Room::BroadcastAOI(shared_ptr<Player> player, shared_ptr<SendBuffer> sendBuffer)
{
	vector<int>& currentView = player->_viewList;
	for (const int& id : currentView) {
		shared_ptr<Player> target_player = Id2Player(id);
		if (target_player == nullptr) continue;
		if (shared_ptr<Session> session = target_player->GetSession()) {
			session->Send(sendBuffer);
		}
	}
}

void Room::NpcEnterRoom(shared_ptr<Monster> monster)
{
	if (false == AddObject(monster)) {
		return;
	}

	_monsters.emplace(monster->GetId(), monster);

	ViewUpdate result = _gameMap.EnterMap(monster->GetId(), monster->GetPosition());
	shared_ptr<SendBuffer> monsterAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(monster);
	for (const int target_id : result.entered) {
		if (target_id == monster->GetId()) continue;

		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		ObjectInfo::Object_Type target_type = target_obj->GetType();
		if (target_type != ObjectInfo::Object_Type::Player) continue;
	
		monster->_viewList.push_back(target_id);

		shared_ptr<Player> target_player = static_pointer_cast<Player>(target_obj);
		target_player->_viewList.push_back(target_id);
		if (shared_ptr<Session> target_session = target_player->GetSession()) {
			target_session->Send(monsterAddBuffer);
		}
	}

}

void Room::NPCMove(shared_ptr<Monster> monster)
{
	if (GetGameObject(monster->GetId()) == nullptr)
		return;
}

std::optional<PositionInfo> Room::GetObjectPosition(int objectId) const
{
	if (shared_ptr<GameObject> object = GetGameObject(objectId)) {
		return object->GetPosition();
	}

	return nullopt;
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

shared_ptr<Room> RoomManager::CreateRoom()
{
	int id = IdGenerator();

	shared_ptr<Room> room = make_shared<Room>(GTimer, _iocpHandle);
	_rooms.insert({ id, room });
	room->InitRoom();

	return room;
}

void RoomManager::Remove(int roomId)
{
	RWLock::WriteGuard lock(_lock);
	_rooms.erase(roomId);
}

void RoomManager::Remove(shared_ptr<Room> room)
{
	return;
}

void RoomManager::EnterPlayer(shared_ptr<Session> session)
{
	RWLock::WriteGuard lock(_lock);
	int playerId = session->GetId();
	shared_ptr<Player> player = make_shared<Player>(session);
	player->SetId(playerId);
	session->_currPlayer = player;

	for (auto& [id, room] : _rooms) {
		if (room->NumPlayers() < MAX_ROOM_CAPACITY) {
			player->SetOwnerRoom(room);
			room->PushJob(&Room::PlayerEnterRoom, player);
			return;
		}
	}

	shared_ptr<Room> newRoom = CreateRoom();
	newRoom->PushJob(&Room::PlayerEnterRoom, player);
	
}

int RoomManager::IdGenerator()
{
	static atomic<int> _idGenerator = 0;
	return ++_idGenerator;
}
