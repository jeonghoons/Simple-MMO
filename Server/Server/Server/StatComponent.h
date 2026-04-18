#pragma once
#include "pch.h"

struct StatInfo
{
	int hp = 100;
	int maxHp = 100;
	int attackDamage = 10;
	float attackSpeed = 1.5f; // 초당 공격 횟수
	float moveSpeed = 400.0f;
};

class StatComponent
{
public:
	void Init(const StatInfo& info) { _stat = info; }

	int GetHp() const { return _stat.hp; }
	int GetMaxHp() const { return _stat.maxHp; }
	int GetAttackDamage() const { return _stat.attackDamage; }
	float GetAttackSpeed() const { return _stat.attackSpeed; }
	float GetMoveSpeed() const { return _stat.moveSpeed; }

	bool IsDead() const { return _stat.hp <= 0; }

	// 실제 적용된 데미지 반환
	int OnDamaged(int damage)
	{
		if (IsDead()) return 0;
		int actualDamage = std::min(_stat.hp, damage);
		_stat.hp -= actualDamage;
		return actualDamage;
	}

	void OnHealed(int amount)
	{
		if (IsDead()) return;
		_stat.hp = std::min(_stat.maxHp, _stat.hp + amount);
	}

private:
	StatInfo _stat;
};

