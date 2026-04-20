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

	if (!_wakeUp) return;

	if (_aiDecisionTimer.IsReady())
	{
		_aiDecisionTimer.Reset(500);
		UpdateAI();
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


	int actualDamage = _statInfo.OnDamaged(damage);


	auto room = GetCurrentRoom();
	if (room) {
		shared_ptr<SendBuffer> damageBuffer = PacketSerializer::MAKE_SC_DAMAGE(GetId(), attacker->GetId(), actualDamage);
		room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), damageBuffer);
	}

	if (_statInfo.IsDead())
	{
		OnDead(attacker);
	}
	else
	{
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
	std::cout << "NPC[" << GetId() << "] State Changed: " << _monsterState << " -> " << newState << std::endl;
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
		// 1. 공격 속도에 따른 쿨타임 재설정
		float attackSpeed = _statInfo.GetAttackSpeed();
		long long nextAttackCooldown = static_cast<long long>(1000.f / (attackSpeed > 0.f ? attackSpeed : 1.f));
		_attackTimer.Reset(nextAttackCooldown);

		// 2. 공격 시 타겟을 바라보도록 Yaw 갱신 (애니메이션 방향 동기화)
		_objectInfo.position.yaw = XMConvertToDegrees(atan2f(diffY, diffX));

		auto room = GetCurrentRoom();
		if (room) {

			shared_ptr<SendBuffer> attackBuffer = PacketSerializer::MAKE_SC_ATTACK(static_pointer_cast<Character>(shared_from_this()), target);
			room->BroadcastAOI(static_pointer_cast<Character>(shared_from_this()), attackBuffer);

			// 4. 선딜레이 적용: 애니메이션의 타격 타이밍(예: 400ms) 후에 실제 판정 함수가 불리도록 Job 예약
			room->ReserveJob(400, &Room::ProcessHitCheck, GetId(), target->GetId());
		}

		std::cout << "NPC[" << GetId() << "] Started Attack Animation toward Player[" << target->GetId() << "]" << std::endl;
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

void Monster::WakeUpByPlayer(std::shared_ptr<Player> player)
{
	if (_statInfo.IsDead()) return;

	if (!_wakeUp)
	{
		_wakeUp = true;
		ChangeState(MonsterState::PATROL); // 깨어나면 패트롤(색적) 상태로 돌입
		cout << "NPC[" << GetId() << "] Woke up by Player[" << player->GetId() << "]" << endl;
	}
}

void Monster::SleepIfNoPlayer()
{
	if (_statInfo.IsDead()) return;

	// Room 로직에 의해 _viewList에는 오직 플레이어만 남으므로, 비어있으면 유저가 없다는 뜻
	if (_viewList.empty())
	{
		_wakeUp = false;
		ChangeState(MonsterState::NONE);
		cout << "NPC[" << GetId() << "] Goes to sleep." << endl;
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
