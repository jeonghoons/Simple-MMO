#pragma once
#include "GameObject.h"

class Character : public MovableObject
{
public:
	Character(ObjectInfo::Object_Type type) : MovableObject(type) {}
	virtual ~Character() = default;

public:
	void RemoveView(int objectId)
	{
		auto it = find(_viewList.begin(), _viewList.end(), objectId);
		if (it != _viewList.end()) {
			*it = _viewList.back();
			_viewList.pop_back();
		}
	}

	vector<int>	_viewList;
public:
	virtual void Update(float deltaTime) override;
};

