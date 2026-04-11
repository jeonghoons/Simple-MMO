#include "pch.h"
#include "Player.h"

Player::Player(shared_ptr<Session> ownerSession) : Character(Object_Type::Player), _ownerSession(ownerSession)
{
	_objectInfo.playerType = PlayerType::Gideon;
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
