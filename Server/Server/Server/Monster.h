#pragma once
#include "pch.h"
#include "GameObject.h"

enum CoolDown : long long
{
	Cool_Move = 1000,
};

class Monster : public GameObject
{
public:
	Monster() = default;
	virtual ~Monster() = default;

	
public:
	long long			_nextMoveTime{};
	atomic<bool>			_wakeUp = false;
};

