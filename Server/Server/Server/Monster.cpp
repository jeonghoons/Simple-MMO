#include "pch.h"
#include "Monster.h"
#include "Player.h"

Monster::Monster() : Character(ObjectInfo::Object_Type::Monster)
{
}

Monster::~Monster()
{
	
}

void Monster::Update(float deltaTime)
{
	Character::Update(deltaTime);
	auto now = std::chrono::steady_clock::now();
	if (now < _nextDecisionTick)
		return;

	_nextDecisionTick = now + std::chrono::milliseconds(100);

	bool hasView = !_viewList.empty();
	if (_wakeUp != hasView)
	{
		_wakeUp = hasView;
		if (!_wakeUp) ChangeState(MonsterState::NONE);
	}
	if (_wakeUp)
	{
		UpdateAI();
	}

	
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
	_monsterState = newState;

	if (_monsterState != MonsterState::TRACE && _monsterState != MonsterState::PATROL)
	{
		_currentSpeed = 0.0f;
		_velocity = { 0.f, 0.f, 0.f };
		_moveDir = { 0.f, 0.f, 0.f };

		_objectInfo.position.v_x = 0.f;
		_objectInfo.position.v_y = 0.f;
		_objectInfo.position.v_z = 0.f;
		_objectInfo.position.state = Move_State::IDLE;
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
	float minEntryDistSq = _traceRange * _traceRange; // 제곱값 미리 계산
	float currentMinDistSq = FLT_MAX;

	for (int objectId : _viewList)
	{
		auto object = room->GetGameObject(objectId);
		if (!object || object->GetType() != ObjectInfo::Player)
			continue;

		auto player = static_pointer_cast<Player>(object);
		if (player == nullptr) continue;

		// 제곱 거리 계산 (sqrt 회피)
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

	// 위치 차이 및 제곱 거리 계산
	float diffX = target->GetPosition().x - _objectInfo.position.x;
	float diffY = target->GetPosition().y - _objectInfo.position.y;
	float distSq = diffX * diffX + diffY * diffY;

	// 1. 공격 사거리 체크 (제곱 비교)
	if (distSq <= _attackRange * _attackRange)
	{
		ChangeState(MonsterState::ATTACK);
		return;
	}

	XMFLOAT3 targetPos = { target->GetPosition().x, target->GetPosition().y, target->GetPosition().z };
	Move(targetPos);
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

	// 공격 사거리를 벗어나면 다시 추격 상태로
	if (distSq > _attackRange * _attackRange)
	{
		ChangeState(MonsterState::TRACE);
	}
}
