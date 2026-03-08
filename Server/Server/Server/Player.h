#pragma once
#include "pch.h"
#include "Character.h"
#include "Session.h"
#include "Room.h"

class Player : public Character
{
public:
	// Player();
	Player(shared_ptr<Session> ownerSession);
	virtual ~Player();

public:
	virtual void Update(float deltaTime) override;

	void SetOwnerSession(shared_ptr<Session> session) { _ownerSession = session; }
	shared_ptr<Session> GetSession() {return _ownerSession.lock();	}
	
	
private:
	weak_ptr<Session> _ownerSession;
	
};



