#include "pch.h"
#include "Character.h"

void Character::Update(float deltaTime)
{
	MovableObject::Update(deltaTime);
}

void Character::RemoveView(int objectId)
{
	auto it = find(_viewList.begin(), _viewList.end(), objectId);
	if (it != _viewList.end()) {
		*it = _viewList.back();
		_viewList.pop_back();
		cout << "Object[" << GetId() << "]에서 " << objectId << "제거" << endl;
	}
}
