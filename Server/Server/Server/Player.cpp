#include "pch.h"
#include "Player.h"

Player::Player(shared_ptr<Session> ownerSession) : Character(ObjectInfo::Object_Type::Player), _ownerSession(ownerSession)
{
	
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
