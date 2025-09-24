#pragma once

#define PORT_NUM 8888

constexpr int MAX_ROOM_CAPACITY = 60;

// Packet Key
enum SC_PACKET_LIST : unsigned char
{
	SC_LOGIN, SC_LOGOUT, SC_ADD_PLAYER, SC_REMOVE_PLAYER,
	SC_CHAT, SC_MOVE_OBJECT, SC_ADD_OBJECT, SC_REMOVE_OBJECT
};

enum CS_PACKET_LIST : unsigned char
{
	CS_LOGIN, CS_LOGOUT, CS_CHAT, CS_MOVE
};

#pragma pack(push, 1)
struct PacketHeader
{
	unsigned short size{};
	unsigned short type{};
};

struct PlayerInfo
{
	enum PLAYER_TYPE : unsigned char
	{

	};

	int id;
	std::pair<int, int> position;

};

#pragma pack (pop)

