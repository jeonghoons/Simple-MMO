#pragma once
#include "pch.h"
#include "Room.h"

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject();
	virtual ~GameObject() = default;

public:
	shared_ptr<Room> GetCurrentRoom() const { return _ownerRoom.lock(); }
	void SetOwnerRoom(shared_ptr<Room> room) { _ownerRoom = room; }

	PositionInfo& GetPosition() { return _objectInfo.position; }
	const PositionInfo& GetPosition() const { return _objectInfo.position; }
	void SetPosition(const PositionInfo& pos) { _objectInfo.position = pos; }


	ObjectInfo& GetInfo() { return _objectInfo; }
	const ObjectInfo& GetInfo() const { return _objectInfo; }
	void SetInfo(const ObjectInfo& info) { _objectInfo = info; }


	int GetId() const { return _objectInfo.id; }
	void SetId(int id) { _objectInfo.id = id; }

	unsigned _last_moveTime{};
	unordered_set<int>	_viewList;
protected:
	ObjectInfo		_objectInfo{};
	weak_ptr<Room>	_ownerRoom;
};

