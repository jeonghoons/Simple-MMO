#pragma once
#include "GameObject.h"
#include	"StatComponent.h"

class Character : public MovableObject
{
public:
	Character(Object_Type type) : MovableObject(type) {}
	virtual ~Character() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void OnDamaged(int damage, std::shared_ptr<GameObject> attacker) {}
	virtual void OnDead(std::shared_ptr<GameObject> attacker) {}

public:
	const StatComponent& GetStat() const { return _statInfo; }


	void RemoveView(int objectId);	
	vector<int>	_viewList;
protected:
	StatComponent _statInfo;
};

