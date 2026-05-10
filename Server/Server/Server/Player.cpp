#include "pch.h"
#include "Player.h"
#include "ServerData.h"

Player::Player(shared_ptr<Session> ownerSession) : Character(Object_Type::Player), _ownerSession(ownerSession)
{
	const CharacterData* statData = DataManager::GetCharacterData((int)_objectInfo.playerType);
	if (statData) {
		_statInfo.Init({
			statData->hp,
			statData->maxHp,
			statData->attackDamage,
			statData->attackSpeed,
			statData->moveSpeed
			});
	}

	_maxSpeed = _statInfo.GetMoveSpeed();
}

Player::~Player()
{
	cout << "~Player[" << _objectInfo.id << "]" << endl;
	_ownerSession.reset();
}

void Player::Update(float deltaTime)
{
	Character::Update(deltaTime);
}

void Player::OnDamaged(int damage, std::shared_ptr<GameObject> attacker)
{
	if (_statInfo.IsDead()) return;

	int actualDamage = _statInfo.OnDamaged(damage);

	// 피격 패킷 전송 로직
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
	}
}

void Player::OnDead(std::shared_ptr<GameObject> attacker)
{
}
