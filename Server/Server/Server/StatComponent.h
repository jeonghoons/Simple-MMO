#pragma once
#include "pch.h"

struct StatInfo
{
	int hp = 100;
	int maxHp = 100;
	int attackDamage = 10;
	float attackSpeed = 1.0f;
	float moveSpeed = 400.0f;
};

class StatComponent
{
public:
	void Init(const StatInfo& info) { _statInfo = info; }

	int GetHp() const { return _statInfo.hp; }
	int GetMaxHp() const { return _statInfo.maxHp; }
	int GetAttackDamage() const { return _statInfo.attackDamage; }
	float GetAttackSpeed() const { return _statInfo.attackSpeed; }
	float GetMoveSpeed() const { return _statInfo.moveSpeed; }

	bool IsDead() const { return _statInfo.hp <= 0; }

	int OnDamaged(int damage)
	{
		if (IsDead()) return 0;
		int actualDamage = std::min(_statInfo.hp, damage);
		_statInfo.hp -= actualDamage;
		return actualDamage;
	}

	void OnHealed(int amount)
	{
		if (IsDead()) return;
		_statInfo.hp = std::min(_statInfo.maxHp, _statInfo.hp + amount);
	}

private:
	StatInfo _statInfo;
};

