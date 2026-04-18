#include "pch.h"
#include "Monster.h"
#include "Player.h"
#include "Room.h"
#include "NavmeshManager.h"

Monster::Monster() : Character(Object_Type::Monster)
{
	_objectInfo.playerType = PlayerType::Monster;
	StatInfo info;
	info.hp = 100;
	info.maxHp = 100;
	info.attackDamage = 15;
	info.attackSpeed = 1.0f; // 1초에 1번 공격
	info.moveSpeed = 400.0f;
	_statInfo.Init(info);

	_maxSpeed = _statInfo.GetMoveSpeed(); // GameObject의 이동 속도와 연동
	_aiDecisionTimer.Reset(0);
}

Monster::~Monster()
{
}

void Monster::Update(float deltaTime)
{
	if (_statInfo.IsDead()) return;

	if (_aiDecisionTimer.IsReady())
	{
		_aiDecisionTimer.Reset(500); // 0.5초 주기로 시야 및 타겟 갱신

		bool hasView = !_viewList.empty();
		if (_wakeUp != hasView)
		{
			_wakeUp = hasView;
			if (!_wakeUp) ChangeState(MonsterState::NONE);
		}

		if (_wakeUp) {
			UpdateAI();
		}
	}

	if (_hasPath) {
		FollowPath(deltaTime);
	}

	if (_objectInfo.position.state == Move_State::RUN)
	{
		if (auto room = GetCurrentRoom()) {
			room->NPCMove(std::static_pointer_cast<Monster>(shared_from_this()));
		}
	}

	Character::Update(deltaTime);
}

void Monster::OnDamaged(int damage, std::shared_ptr<GameObject> attacker)
{
	if (_statInfo.IsDead()) return;

	// StatComponent를 통해 HP 차감
	int actualDamage = _statInfo.OnDamaged(damage);

	// 피격 패킷 전송 로직
	// auto room = GetCurrentRoom();
	// if (room) {
	// 	shared_ptr<SendBuffer> hitBuffer = PacketSerializer::MAKE_SC_HIT(GetId(), actualDamage);
	// 	room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), hitBuffer);
	// }

	if (_statInfo.IsDead())
	{
		OnDead(attacker);
	}
	else
	{
		// 살아있고 현재 타겟이 없다면 반격
		if (_targetPlayer.expired() && attacker->GetType() == Object_Type::Player) {
			_targetPlayer = std::static_pointer_cast<Player>(attacker);
			ChangeState(MonsterState::TRACE);
		}
	}
}

void Monster::OnDead(std::shared_ptr<GameObject> attacker)
{
	StopMove();
	ChangeState(MonsterState::NONE);

	auto room = GetCurrentRoom();
	if (room) {
		// 사망 패킷
		// shared_ptr<SendBuffer> deadBuffer = PacketSerializer::MAKE_SC_DEAD(GetId(), attacker->GetId());
		// room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), deadBuffer);

		// Room 관리 목록에서 안전하게 제거하기 위해 JobQueue 활용
		// room->PushJob(&Room::RemoveObject, GetId());
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

	StopMove();
	// std::cout << "NPC[" << GetId() << "] State Changed: " << _monsterState << " -> " << newState << std::endl;
	_monsterState = newState;

	if (_monsterState == MonsterState::TRACE) {
		_pathSearchTimer.Reset(0);
	}
	else if (_monsterState == MonsterState::PATROL) {
		_patrolTimer.Reset(1000);
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

	std::shared_ptr<Player> closestPlayer = nullptr;
	float currentMinDistSq = FLT_MAX;
	float traceRangeSq = _traceRange * _traceRange;

	for (int objectId : _viewList)
	{
		auto object = room->GetGameObject(objectId);
		if (!object || object->GetType() != Object_Type::Player) continue;

		auto player = std::static_pointer_cast<Player>(object);

		float diffX = player->GetPosition().x - GetPosition().x;
		float diffY = player->GetPosition().y - GetPosition().y;
		float distSq = diffX * diffX + diffY * diffY;

		// 추가: 플레이어가 살아있을 때만 추적 (Player 클래스에도 StatComponent 연동 필요)
		if (distSq < currentMinDistSq && distSq <= traceRangeSq /* && !player->GetStat().IsDead() */)
		{
			currentMinDistSq = distSq;
			closestPlayer = player;
		}
	}

	if (closestPlayer)
	{
		_targetPlayer = closestPlayer;
		ChangeState(MonsterState::TRACE);
		return;
	}

	if (!_hasPath && _patrolTimer.IsReady())
	{
		PositionInfo randomDest = room->GetGameMap().GetRandomPosInCell(GetPosition());

		std::vector<PositionInfo> newPath;
		if (room->GetNavManager()->FindPath(GetPosition(), randomDest, newPath))
		{
			cout << "NPC[" << _objectInfo.id << "] : " << GetPosition().x << ", " << GetPosition().y << ", " << GetPosition().z
				<< " -> " << randomDest.x << ", " << randomDest.y << "," << randomDest.z << endl;
			SetPath(newPath);
			long long waitTime = 1000;
			_patrolTimer.Reset(waitTime);
		}
		else
		{
			_patrolTimer.Reset(500);
		}
	}	
}

void Monster::UpdateTrace()
{
	auto target = _targetPlayer.lock();
	if (target == nullptr /* || target->GetStat().IsDead() */)
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

	if (distSq <= _attackRange * _attackRange)
	{
		ChangeState(MonsterState::ATTACK);
		return;
	}

	float targetMovedDistSq = pow(target->GetPosition().x - _lastTargetPos.x, 2) + pow(target->GetPosition().y - _lastTargetPos.y, 2);

	if (targetMovedDistSq > 10000.f || _pathSearchTimer.IsReady())
	{
		auto room = GetCurrentRoom();
		if (room && room->GetNavManager())
		{
			std::vector<PositionInfo> newPath;
			if (room->GetNavManager()->FindPath(GetPosition(), target->GetPosition(), newPath))
			{
				SetPath(newPath);
				_lastTargetPos = target->GetPosition();
				_pathSearchTimer.Reset(1000);
			}
		}
	}
}

void Monster::UpdateAttack()
{
	auto target = _targetPlayer.lock();
	if (target == nullptr /* || target->GetStat().IsDead() */)
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
		return;
	}

	if (_attackTimer.IsReady())
	{
		// StatComponent에서 공격 속도를 가져와 쿨타임 계산
		float attackSpeed = _statInfo.GetAttackSpeed();
		long long nextAttackCooldown = static_cast<long long>(1000.f / (attackSpeed > 0.f ? attackSpeed : 1.f));
		_attackTimer.Reset(nextAttackCooldown);

		int damage = _statInfo.GetAttackDamage();

		// 데미지 처리 (현재 스케줄링 모델 환경에서 Room의 싱글스레드 Update 순회 중이므로 직접 호출 안전함)
		target->OnDamaged(damage, shared_from_this());

		// 공격 패킷 전송 (애니메이션 동기화용)
		// auto room = GetCurrentRoom();
		// if (room) {
		// 	shared_ptr<SendBuffer> attackBuffer = PacketSerializer::MAKE_SC_ATTACK(GetId(), target->GetId());
		// 	room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), attackBuffer);
		// }

		std::cout << "NPC[" << GetId() << "] Attacked Player[" << target->GetId() << "] / Dmg: " << damage << std::endl;
	}
}

void Monster::SetPath(const std::vector<PositionInfo>& path)
{
	_path = path;
	_pathIndex = 0;
	if (_path.empty()) {
		StopMove();
	}
	else {
		_hasPath = true;
		_objectInfo.position.state = Move_State::RUN;
	}
}

void Monster::StopMove()
{
	if (_objectInfo.position.state == Move_State::IDLE) return;

	_hasPath = false;
	MovableObject::StopMove();

	auto room = GetCurrentRoom();
	if (room != nullptr) {
		shared_ptr<SendBuffer> MoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(shared_from_this());
		room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), MoveBuffer);
	}
}

void Monster::FollowPath(float deltaTime)
{
	if (!_hasPath || _pathIndex >= _path.size()) return;

	PositionInfo& pos = _objectInfo.position;

	while (_pathIndex < _path.size())
	{
		PositionInfo& targetPos = _path[_pathIndex];

		XMVECTOR vCurr = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);
		XMVECTOR vDest = XMVectorSet(targetPos.x, targetPos.y, targetPos.z, 0.0f);
		float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(vDest, vCurr)));

		if (dist < 50.0f)
		{
			_pathIndex++;
		}
		else
		{
			XMFLOAT3 desPos = { targetPos.x, targetPos.y, targetPos.z };

			if (Move(desPos))
			{
				auto room = GetCurrentRoom();
				if (room != nullptr) {
					shared_ptr<SendBuffer> MoveBuffer = PacketSerializer::MAKE_SC_MOVE_OBJECT(shared_from_this());
					room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), MoveBuffer);
				}
			}
			return;
		}
	}
	StopMove();
}
