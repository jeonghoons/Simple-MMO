#pragma once
#include "pch.h"
#include "Character.h"

class Player;

enum class MonsterState
{
	NONE,   
	PATROL,
	TRACE,  
	ATTACK  
};

class Monster : public Character
{
public:
	Monster();
	virtual ~Monster();

public:
	virtual void Update(float deltaTime) override;

	// FSM
	void UpdateAI();
	void ChangeState(MonsterState newState);

	void UpdateNone();
	void UpdatePatrol();
	void UpdateTrace();
	void UpdateAttack();
	
public:
	MonsterState _monsterState = MonsterState::NONE;
	atomic<bool>			_wakeUp = false;
	std::chrono::steady_clock::time_point _nextDecisionTick;
	weak_ptr<Player>	_targetPlayer;
	float _traceRange = 500.f;
	float _attackRange = 100.f;

	XMFLOAT3 _nextPos;
};

