#include "pch.h"

thread_local unsigned int Lthreadid = 1;

RoomManager* GRoomManager = new RoomManager();
shared_ptr<Timer> GTimer = make_shared<Timer>();