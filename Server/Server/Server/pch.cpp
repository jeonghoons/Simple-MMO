#include "pch.h"

thread_local unsigned int Lthreadid = 1;

unique_ptr<RoomManager> GRoomManager = make_unique<RoomManager>();
shared_ptr<Timer> GTimer = make_shared<Timer>();
unique_ptr<DBConnectionPool> GDBConnectionPool = make_unique<DBConnectionPool>();