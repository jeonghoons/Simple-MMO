#include "pch.h"
#include "Room.h"
#include "Session.h"
#include "PacketHandler.h"
#include "Player.h"
#include "Monster.h"

const int NUM_MONSTER = 5;
Room::Room(shared_ptr<Timer> timer, HANDLE iocpHandle) : _timer(timer), _jobQueue(make_shared<JobQueue>(iocpHandle))
{
}
Room::~Room() = default;

void Room::InitRoom()
{
	if (!_gameMap.LoadMapData("ParagonSample"))
	{
		cout << "[Room] 맵 초기화(LoadMapData) 중 일부 문제가 발생했습니다." << endl;
	}
	_gameMap.Init(weak_from_this());

	const auto& spawnPoints = _gameMap.GetSpawnPoints();
	if (spawnPoints.empty())
	{
		cout << "[Room] 스폰 포인트가 없어 몬스터를 생성할 수 없습니다!" << endl;
	}
	else
	{
		for (const auto& sp : spawnPoints)
		{
			shared_ptr<Monster> monster = make_shared<Monster>();
			monster->SetId(MonsterIdGenerator());

			// 추출된 스폰 포인트 좌표를 고정적으로 주입
			PositionInfo spawnPos = { sp.X, sp.Y, sp.Z, sp.Yaw };
			monster->SetPosition(spawnPos);

			NpcEnterRoom(monster);
		}
		for (int i = 1; i < spawnPoints.size()-1; ++i) {
			shared_ptr<Monster> monster = make_shared<Monster>();
			monster->SetId(MonsterIdGenerator());

			// 추출된 스폰 포인트 좌표를 고정적으로 주입
			PositionInfo spawnPos = { spawnPoints[i].X, spawnPoints[i].Y, spawnPoints[i].Z, spawnPoints[i].Yaw};
			monster->SetPosition(spawnPos);

			NpcEnterRoom(monster);
		}
		
	}
	
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
	/*PositionInfo spawnPos = {31350.0f, 15010.0f, 100.f, 0.f}; */
	/*PositionInfo spawnPos = { Utils::GetRandom(0.f, MAP_WIDTH / 2.f),
		Utils::GetRandom(0.f, MAP_WIDTH / 2.f), 100.f};*/

	const auto& spawnPoints = _gameMap.GetSpawnPoints();
	if (object->GetType() == Object_Type::Player) {
		object->SetPosition({ spawnPoints[0].X, spawnPoints[0].Y, spawnPoints[0].Z});
		// object->SetPosition({ 0.0, 0.0, 0.0 });
	}

	PositionInfo currentPos = object->GetPosition();

	if (_gameMap.IsOutOfBounds(currentPos))
	{
		cout << "[경고] 오브젝트 스폰 위치가 맵(NavMesh) 바깥이거나 바닥이 없습니다! ID: " << objectId;
		cout << ", " << currentPos.x << ", " << currentPos.y << ", " << currentPos.z << endl;
		
	}

	object->SetPosition(currentPos);

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

		Object_Type targetType = target_obj->GetType();
		if (targetType == Object_Type::Player || targetType == Object_Type::Monster) {
			auto target_char = static_pointer_cast<Character>(target_obj);

			target_char->_viewList.push_back(player->GetId());

			if (targetType == Object_Type::Player) {
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

		Object_Type target_type = target_obj->GetType();
		if (target_type == Object_Type::Player || target_type == Object_Type::Monster) {

			auto target_char = static_pointer_cast<Character>(target_obj);
			target_char->RemoveView(playerId);

			if (target_type == Object_Type::Player) {
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

void Room::PlayerMove(shared_ptr<Player> player, PositionInfo position, bool force)
{
	if (nullptr == Id2Player(player->GetId()))
		return;

	PositionInfo currentPos = player->GetPosition();

	if (!_gameMap.CanMove(currentPos, position))
	{
		if (shared_ptr<Session> session = player->GetSession()) {
			shared_ptr<SendBuffer> correctionBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(player);
			session->Send(correctionBuffer);
		}
		cout << "비정상 이동 차단" << endl;

		return;
	}
	
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
		Object_Type target_type = target_obj->GetType();
		if (target_type == Object_Type::Player || target_type == Object_Type::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->RemoveView(player->GetId());

			if (target_type == Object_Type::Player) {
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
		Object_Type target_type = target_obj->GetType();
		if (target_type == Object_Type::Player || target_type == Object_Type::Monster) {
			shared_ptr<Character> target_char = static_pointer_cast<Character>(target_obj);
			target_char->_viewList.push_back(player->GetId());

			if (target_type == Object_Type::Player) {
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

	/*for (const int id : player->_viewList) {
		shared_ptr<Player> target_player = Id2Player(id);
		if (!target_player) continue;
		if (shared_ptr<Session> target_session = target_player->GetSession()) {
			target_session->Send(playerMoveBuffer);
		}
	}*/

	BroadcastAOI(static_pointer_cast<Character>(player), playerMoveBuffer);
	

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

void Room::BroadcastAOI(shared_ptr<Character> viewableObj, shared_ptr<SendBuffer> sendBuffer)
{
	vector<int>& currentView = viewableObj->_viewList;
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

		Object_Type target_type = target_obj->GetType();
		if (target_type != Object_Type::Player) continue;
	
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

void RoomManager::EnterPlayer(shared_ptr<Player> player)
{
	RWLock::WriteGuard lock(_lock);
	

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
