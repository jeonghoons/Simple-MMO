#pragma once
#include "ProtocolSturct.h"

// =======================
#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	PacketHeader header;
	char accountID[20];
	char accountPW[20];
};

struct CS_SIGNUP_PACKET {
	PacketHeader header;
	char accountID[20];
	char accountPW[20];
};

struct CS_LOGOUT_PACKET {
	PacketHeader header;
};

struct CS_CHAT_PACKET {
	PacketHeader header;
	char message[1024];
};

struct CS_ENTER_ROOM_PACKET {
	PacketHeader header;
};

struct CS_MOVE_PACKET {
	PacketHeader header;
	PositionInfo posInfo;
	bool force = false;
};

struct CS_CMOVE_PACKET {
	PacketHeader header;
	PositionInfo posInfo;
	bool force = false;
};

// ------------------------------------------
struct SC_LOGIN_INFO_PACKET {
	PacketHeader header;
	ObjectInfo			objectInfo;
};

struct SC_SIGNUP_PACKET {
	PacketHeader header;
	bool result;
};

struct SC_ADD_OBJECT_PACKET {
	PacketHeader header;
	ObjectInfo	objectInfo;
};

struct SC_REMOVE_OBJECT_PACKET {
	PacketHeader header;
	int				objectId;
};

struct SC_CHAT_PACKET {
	PacketHeader header;
	char message[1024];
};

struct SC_MOVE_PACKET {
	PacketHeader header;
	ObjectInfo objectInfo;
	unsigned int move_time;
};






#pragma pack (pop)




