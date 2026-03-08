#pragma once

#define PORT_NUM 8888

constexpr int MAX_ROOM_CAPACITY = 100;

// Packet Key
enum SC_PACKET_LIST : unsigned char
{
	SC_LOGIN, SC_SIGNUP, SC_LOGOUT,
	SC_ADD_OBJECT, SC_REMOVE_OBJECT,
	SC_CHAT,
	SC_MOVE_OBJECT, SC_CMOVE_OBJECT,
};

enum CS_PACKET_LIST : unsigned char
{
	CS_LOGIN, CS_SIGNUP, CS_LOGOUT,
	CS_CHAT,
	CS_MOVE, CS_CMOVE
};

#pragma pack(push, 1)
struct PacketHeader
{
	unsigned short size{};
	unsigned short type{};
};

enum Move_State : unsigned char
{
	NONE, IDLE, RUN, JUMP
};

struct PositionInfo
{
	float x{};
	float y{};
	float z{};
	float yaw{};
	Move_State state = Move_State::NONE;
	float inputX; // аб©Л
	float inputY; // ╬у╣з
};

struct ObjectInfo
{
	enum Object_Type : unsigned char
	{
		NONE, Player, Monster, Npc, Item, ENVIRONMENT
	};

	Object_Type		type;
	int id{};
	PositionInfo			position{};

};
#pragma pack (pop)

