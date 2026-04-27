#include "pch.h"
#include "Character.h"

Character::Character(Object_Type type) : MovableObject(type)
{
}

void Character::Update(float deltaTime)
{
	MovableObject::Update(deltaTime);
}

bool Character::Attack(int& skillId)
{
	if (_statInfo.IsDead()) return false;

	if (skillId == 0)
	{
		skillId = (int)_objectInfo.playerType * 100 + 1; // 몬스터 기본 공격
	}

	return true;
}

void Character::RemoveView(int objectId)
{
	auto it = find(_viewList.begin(), _viewList.end(), objectId);
	if (it != _viewList.end()) {
		*it = _viewList.back();
		_viewList.pop_back();
		// cout << "Object[" << GetId() << "]에서 " << objectId << "제거" << endl;
	}
}
