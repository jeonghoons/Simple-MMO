#pragma once
// #include "pch.h"
#include "ProtocolSturct.h"

// =======================
#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	PacketHeader header;
};

struct CS_LOGOUT_PACKET {
	PacketHeader header;
};

struct CS_CHAT_PACKET {
	PacketHeader header;
	char message[1024];
};

struct CS_MOVE_PACKET {
	PacketHeader header;
	int			direction; // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned	move_time;
};

// ------------------------------------------
struct SC_LOGIN_INFO_PACKET {
	PacketHeader header;
	ObjectInfo			objectInfo;
};

struct SC_ADD_PLAYER_PACKET {
	PacketHeader header;
	ObjectInfo			objectInfo;
};

struct SC_REMOVE_PLAYER_PACKET {
	PacketHeader header;
	int			playerId;
};

struct SC_CHAT_PACKET {
	PacketHeader header;
	char message[1024];
};

struct SC_MOVE_PACKET {
	PacketHeader header;
	ObjectInfo	objectInfo;
	unsigned int move_time;
};

struct SC_ADD_OBJECT_PACKET {
	PacketHeader header;
	ObjectInfo	objectInfo;
};

struct SC_REMOVE_OBJECT_PACKET {
	PacketHeader header;
	int				objectId;
};


#pragma pack (pop)




