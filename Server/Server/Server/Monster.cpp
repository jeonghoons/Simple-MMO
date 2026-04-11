#include "pch.h"
#include "Monster.h"
#include "Player.h"
#include "Room.h"
#include "NavmeshManager.h"

Monster::Monster() : Character(Object_Type::Monster)
{
	_objectInfo.playerType = PlayerType::Monster;
	_maxSpeed = 450.0f;
}

Monster::~Monster()
{
}

void Monster::Update(float deltaTime)
{
	auto now = std::chrono::steady_clock::now();
	if (now >= _nextDecisionTick)
	{
		_nextDecisionTick = now + std::chrono::milliseconds(500);

		bool hasView = !_viewList.empty();
		if (_wakeUp != hasView)
		{
			_wakeUp = hasView;
			if (!_wakeUp) ChangeState(MonsterState::NONE);
		}

		if (_wakeUp) {
			UpdateAI(); // 여기서 타겟을 찾거나, 길(Path)을 새로 생성합니다.
		}

	}

	if (_hasPath) {
		FollowPath(deltaTime); // UpdateAI가 방금 찾은 길을 즉시 반영하여 방향을 세팅합니다.
	}
	
	Character::Update(deltaTime);

	auto room = GetCurrentRoom();
	// (매 틱마다 쏘는 패킷 폭탄을 방지)
	shared_ptr<SendBuffer> MoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(shared_from_this());
	room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), MoveBuffer);

	if (MonsterState::NONE == _monsterState) return;
	cout << "NPC[" << GetId() << "] - ";
	cout << GetPosition().x << ", " << GetPosition().y << ", vel : " << _velocity.x << ", " << _velocity.y << endl;
}

void Monster::UpdateAI()
{
	switch (_monsterState)
	{
	case MonsterState::NONE:   UpdateNone();   break;
	case MonsterState::PATROL: UpdatePatrol(); break;
	case MonsterState::TRACE:  UpdateTrace();  break;
	case MonsterState::ATTACK: UpdateAttack(); break;
	}
}

void Monster::ChangeState(MonsterState newState)
{
	if (_monsterState == newState) return;

	Stop();

	cout << "NPC[" << GetId() << "] - " << _monsterState << " -> " << newState<<endl;
	_monsterState = newState;

	if (_monsterState == MonsterState::TRACE)
	{
		_lastPathSearchTick = std::chrono::steady_clock::time_point::min();
	}
}

void Monster::UpdateNone()
{
	if (false == _viewList.empty())
		ChangeState(MonsterState::PATROL);
}

void Monster::UpdatePatrol()
{
	auto room = GetCurrentRoom();
	if (room == nullptr) return;

	shared_ptr<Player> closestPlayer = nullptr;
	float minEntryDistSq = _traceRange * _traceRange;
	float currentMinDistSq = FLT_MAX;

	for (int objectId : _viewList)
	{
		auto object = room->GetGameObject(objectId);
		if (!object || object->GetType() != Object_Type::Player) continue;

		auto player = static_pointer_cast<Player>(object);

		float diffX = player->GetPosition().x - GetPosition().x;
		float diffY = player->GetPosition().y - GetPosition().y;
		float distSq = diffX * diffX + diffY * diffY;

		if (distSq < currentMinDistSq)
		{
			currentMinDistSq = distSq;
			closestPlayer = player;
		}
	}

	if (closestPlayer && currentMinDistSq <= minEntryDistSq)
	{
		_targetPlayer = closestPlayer;
		ChangeState(MonsterState::TRACE);
	}

	
}

void Monster::UpdateTrace()
{
	auto target = _targetPlayer.lock();
	if (target == nullptr)
	{
		_targetPlayer.reset();
		ChangeState(MonsterState::PATROL);
		return;
	}

	float diffX = target->GetPosition().x - _objectInfo.position.x;
	float diffY = target->GetPosition().y - _objectInfo.position.y;
	float distSq = diffX * diffX + diffY * diffY;

	if (distSq > _traceRange * _traceRange)
	{
		_targetPlayer.reset();
		ChangeState(MonsterState::PATROL);
		return;
	}

	// 공격 사거리 내 진입
	if (distSq <= _attackRange * _attackRange)
	{
		ChangeState(MonsterState::ATTACK);
		return;
	}

	// [길찾기 최적화] 타겟이 크게 이동했거나 시간이 경과했을 때만 연산
	float targetMovedDistSq = pow(target->GetPosition().x - _lastTargetPos.x, 2) + pow(target->GetPosition().y - _lastTargetPos.y, 2);

	auto now = chrono::steady_clock::now();
	auto timeSinceLastSearch = chrono::duration_cast<chrono::milliseconds>(now - _lastPathSearchTick).count();

	if (targetMovedDistSq > 10000.f || timeSinceLastSearch > 1000)
	{
		auto room = GetCurrentRoom();
		if (room && room->GetNavManager())
		{
			std::vector<PositionInfo> newPath;
			if (room->GetNavManager()->FindPath(GetPosition(), target->GetPosition(), newPath))
			{
				SetPath(newPath);
				_lastTargetPos = target->GetPosition();
				_lastPathSearchTick = now;

			}
		}
	}
}

void Monster::UpdateAttack()
{
	auto target = _targetPlayer.lock();
	if (target == nullptr)
	{
		ChangeState(MonsterState::PATROL);
		return;
	}

	float diffX = target->GetPosition().x - _objectInfo.position.x;
	float diffY = target->GetPosition().y - _objectInfo.position.y;
	float distSq = diffX * diffX + diffY * diffY;

	if (distSq > _attackRange * _attackRange)
	{
		ChangeState(MonsterState::TRACE);
	}
}

void Monster::SetPath(const std::vector<PositionInfo>& path)
{
	_path = path;
	_pathIndex = 0;
	if (_path.empty()) {
		Stop();
	}
	else {
		_hasPath = true;
		_objectInfo.position.state = Move_State::RUN;
	}
}

void Monster::Stop()
{
	_hasPath = false;
	_currentSpeed = 0.0f;
	_velocity = { 0.f, 0.f, 0.f };
	_moveDir = { 0.f, 0.f, 0.f };
	_objectInfo.position.v_x = 0.f;
	_objectInfo.position.v_y = 0.f;
	_objectInfo.position.v_z = 0.f;
	_objectInfo.position.state = Move_State::IDLE;
}

void Monster::FollowPath(float deltaTime)
{
	if (!_hasPath || _pathIndex >= _path.size()) return;

	PositionInfo& pos = _objectInfo.position;
	PositionInfo& targetPos = _path[_pathIndex];

	XMVECTOR vCurr = XMVectorSet(pos.x, pos.y, 0.f, 0);
	while (_pathIndex < _path.size())
	{
		PositionInfo& targetPos = _path[_pathIndex];
		XMVECTOR vDest = XMVectorSet(targetPos.x, targetPos.y, 0.0f, 0.0f);
		XMVECTOR vDir = XMVectorSubtract(vDest, vCurr);
		float dist = XMVectorGetX(XMVector3Length(vDir));

		if (dist < 50.0f)
		{
			// 경유지에 도착했으면 인덱스를 늘리고 다시 while문 처음으로 돌아가 다음 경유지와의 거리를 잼
			_pathIndex++;
		}
		else
		{
			// 아직 도착하지 않은 경유지를 찾았다면 방향/속도 세팅 후 탈출!
			XMStoreFloat3(&_moveDir, XMVector3Normalize(vDir));
			_currentSpeed = _maxSpeed;
			pos.yaw = XMConvertToDegrees(atan2f(_moveDir.y, _moveDir.x));
			return; // 정상적으로 이동할 방향을 찾았을 때만 return
		}
	}

	// while문을 빠져나왔다 == 모든 경로(_path)를 다 돌았다 == 최종 목적지 도착
	Stop();
}
