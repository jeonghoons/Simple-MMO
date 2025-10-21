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
	Update();
}

void Room::PlayerEnterRoom(shared_ptr<Player> player)
{
	if (false == AddObject(player)) {
		return;
	}

	SC_LOGIN_INFO_PACKET logInPacket;
	logInPacket.header = { sizeof(SC_LOGIN_INFO_PACKET), SC_LOGIN };
	logInPacket.objectInfo = player->GetInfo();
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(logInPacket));
	sendBuffer->CopyData(&logInPacket, sizeof(logInPacket));

	if (auto session = player->GetSession()) {
		session->Send(sendBuffer);
	}

	_gameMap.UpdateObjectPosition(player->GetId(), player->GetPosition()); // 게임맵에 등록
	unordered_set<int> newView = GetViewList(player->GetId());
	player->_viewList = std::move(newView);

	shared_ptr<SendBuffer> objectAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(player);

	for (int target_id : newView) {

		shared_ptr<GameObject> target_obj = GetGameObject(target_id);
		if (!target_obj) continue;

		if (auto session = player->GetSession()) {
			session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_obj));
		}

		target_obj->_viewList.insert(player->GetId());
		if (auto target_player = Id2Player(target_id)) {
			if (auto target_session = target_player->GetSession()) {
				target_session->Send(objectAddBuffer);
			}
		}
		
	}

}

void Room::NpcEnterRoom(shared_ptr<Monster> monster)
{
	if (false == AddObject(monster)) {
		return;
	}

	_gameMap.UpdateObjectPosition(monster->GetId(), monster->GetPosition());
	std::unordered_set<int> newView = GetNPCViewList(monster->GetId());
	monster->_viewList = newView;

	shared_ptr<SendBuffer> monsterAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(monster);

	for (int target_id : newView) {
		if (shared_ptr<Player> target_player = Id2Player(target_id)) {
			if (auto target_session = target_player->GetSession()) {
				target_session->Send(monsterAddBuffer);
			}

			target_player->_viewList.insert(monster->GetId());
		}
	}
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

	return true;
}

bool Room::RemoveObject(int objectId)
{
	_players.erase(objectId);
	_monsters.erase(objectId);

	return _objects.erase(objectId) > 0;
}

std::optional<PositionInfo> Room::GetObjectPosition(int objectId) const
{
	if (shared_ptr<GameObject> object = GetGameObject(objectId)) {
		return object->GetPosition();
	}

	return nullopt;
}

void Room::Update()
{
	// cout << "Update Room" << endl;

	long long current_tick = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();

	for (const auto& [id, monster] : _monsters) {
		if (monster->_wakeUp == false) continue;
		NpcAI(monster, current_tick);
	}


	ReserveJob(100, &Room::Update);

}


void Room::PlayerMove(shared_ptr<Player> player, int direction, unsigned move_time)
{
	if (nullptr == Id2Player(player->GetId()))
		return;

	PositionInfo pos = player->GetPosition();
	PositionInfo new_pos = pos;

	switch (direction)
	{
	case 2: // left
		new_pos.x -= 1;
		break;
	case 3: // right
		new_pos.x += 1;
		break;
	case 0: // up
		new_pos.y -= 1;
		break;
	case 1: // down
		new_pos.y += 1;
		break;
	default:
		cout << "Move Error" << endl;
		return;
	}

	int moveResult = _gameMap.ValidateMove(player->GetId(), new_pos); // 경계, 충돌 감지
	if (moveResult == static_cast<int>(MoveResult::OutOfBounds)) {// 경계밖으로 나간경우
		new_pos = pos; 
	}
	if (moveResult > static_cast<int>(MoveResult::Validate)) { // 충돌을 감지
		// cout << "player[" << player->GetId() << "]가 [" << moveResult << "] 충돌" << endl;
	}

	player->_last_moveTime = move_time;
	player->SetPosition(new_pos);

	shared_ptr<SendBuffer> playerMoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(player);
	if (auto p = player->GetSession()) { // 자신은 이동
		p->Send(playerMoveBuffer);
	}

	unordered_set<int> oldView = player->_viewList; // 이전 시야
	bool changeCell = _gameMap.UpdateObjectPosition(player->GetId(), new_pos); // 셀 업데이트
	if (changeCell) {
		unordered_set<int> newView = GetViewList(player->GetId());

		shared_ptr<SendBuffer> playerRemoveBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(player->GetId());
		for (int old_id : oldView) { // 있었음 -> 없음 제거
			if (newView.count(old_id) == 0) {

				if (auto session = player->GetSession()) {
					session->Send(PacketSerializer::MAKE_SC_REMOVE_OBJECT(old_id));
				}


				if (shared_ptr<GameObject> target_obj = GetGameObject(old_id)) {
					target_obj->_viewList.erase(player->GetId());
					if (auto target_player = Id2Player(old_id)) {
						if (auto target_session = target_player->GetSession()) {
							target_session->Send(playerRemoveBuffer);
						}
					}
				}
			}
		}

		shared_ptr<SendBuffer> playerAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(player);
		for (int new_id : newView) { // 없었음 -> 있음 추가
			if (oldView.count(new_id) == 0) {
				if (shared_ptr<GameObject> target_obj = GetGameObject(new_id)) {
					if (auto session = player->GetSession()) {
						session->Send(PacketSerializer::MAKE_SC_ADD_OBJECT(target_obj));
					}

					if (target_obj->_viewList.insert(player->GetId()).second) {
						if (auto target_player = Id2Player(new_id)) {
							if (auto target_session = target_player->GetSession()) {
								target_session->Send(playerAddBuffer);
							}
						}
					}
				}
			}
		}
		player->_viewList = std::move(newView);
	}

	unordered_set<int>& currentView = player->_viewList;
	for (int target_id : currentView) { // 시야 리스트에만 BroadCast
		if (shared_ptr<Player> target_player = Id2Player(target_id)) {
			if (auto session = target_player->GetSession()) {
				session->Send(playerMoveBuffer);
			}
		}
		else if (shared_ptr<Monster> target_monster = Id2Monster(target_id)) {
			if (false == target_monster->_wakeUp) {
				target_monster->_wakeUp = true;
			}
		}
	}

}



void Room::NPCMove(shared_ptr<Monster> monster)
{
	if (GetGameObject(monster->GetId()) == nullptr)
		return;

	PositionInfo pos = monster->GetPosition();
	PositionInfo new_pos = pos;
	switch (rand() % 4)
	{
	case 2: // left
		new_pos.x -= 1;
		break;
	case 3: // right
		new_pos.x += 1;
		break;
	case 0: // up
		new_pos.y -= 1;
		break;
	case 1: // down
		new_pos.y += 1;
		break;
	default:
		cout << "Move Error" << endl;
		return;
	}

	int moveResult = _gameMap.ValidateMove(monster->GetId(), new_pos);
	if (moveResult == static_cast<int>(MoveResult::OutOfBounds)) {
		new_pos = pos;
	}
	if (moveResult > static_cast<int>(MoveResult::Validate) && Id2Player(moveResult) != nullptr) {
		// cout << "monster[" << monster->GetId() << "]가 [" << moveResult << "] 충돌" << endl;
	}

	monster->SetPosition(new_pos);

	shared_ptr<SendBuffer> monsterMoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(monster);

	unordered_set<int> oldView = monster->_viewList;
	bool changeCell = _gameMap.UpdateObjectPosition(monster->GetId(), new_pos);
	if (changeCell) {
		unordered_set<int> newView = GetNPCViewList(monster->GetId());

		shared_ptr<SendBuffer> monsterRemoveBuffer = PacketSerializer::MAKE_SC_REMOVE_OBJECT(monster->GetId());
		for (int old_id : oldView) {
			if (newView.count(old_id) == 0) {
				if (shared_ptr<GameObject> target_obj = GetGameObject(old_id)) {
					target_obj->_viewList.erase(monster->GetId());

					if (auto target_player = Id2Player(old_id)) {
						if (auto target_session = target_player->GetSession()) {
							target_session->Send(monsterRemoveBuffer); 
						}
					}
				}
			}
		}

		shared_ptr<SendBuffer> monsterAddBuffer = PacketSerializer::MAKE_SC_ADD_OBJECT(monster);
		for (int new_id : newView) { // 없었음 -> 있음 추가
			if (oldView.count(new_id) == 0) {

				if (shared_ptr<GameObject> target_obj = GetGameObject(new_id)) {
					if (target_obj->_viewList.insert(monster->GetId()).second) {
						if (auto target_player = Id2Player(new_id)) {
							if (auto target_session = target_player->GetSession()) {
								target_session->Send(monsterAddBuffer);
							}
						}
					}
				}
			}
		}

		monster->_viewList = std::move(newView);
	}

	unordered_set<int>& currentView = monster->_viewList;
	for (int target_id : currentView) {
		if (auto target_player = Id2Player(target_id)) {
			if (auto session = target_player->GetSession()) {
				session->Send(monsterMoveBuffer);
			}
		}
	}
}

void Room::NpcAI(shared_ptr<Monster> monster, long long curr_tick)
{
	if (GetGameObject(monster->GetId()) == nullptr)
		return;

	if (monster->_nextMoveTime < curr_tick) {
		monster->_nextMoveTime = curr_tick + CoolDown::Cool_Move;
		NPCMove(monster);
	}
	
	if (monster->_viewList.size() == 0) {
		monster->_wakeUp = false;
	}
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
	{
		RWLock::ReadGuard lock(_lock);
		for (auto& [id, room] : _rooms) {
			if (room->NumPlayers() < MAX_ROOM_CAPACITY) {
				player->SetOwnerRoom(room);
				room->PushJob(&Room::PlayerEnterRoom, player);
				return;
			}
		}
	} 

	{
		RWLock::WriteGuard lock(_lock);
		shared_ptr<Room> newRoom = CreateRoom();
		player->SetOwnerRoom(newRoom);
		newRoom->PushJob(&Room::PlayerEnterRoom, player);
		return;
	}
}

int RoomManager::IdGenerator()
{
	static atomic<int> _idGenerator = 0;
	return ++_idGenerator;
}
