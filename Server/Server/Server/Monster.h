#pragma once
#include "pch.h"
#include "GameObject.h"


class Monster : public GameObject
{
public:
	Monster() = default;
	virtual ~Monster() = default;

public:
	atomic<bool>			_wakeUp = false;
};

