#pragma once


class PacketSerializer
{
public:
	
public:
	static shared_ptr<SendBuffer> MAKE_SC_ADD_PLAYER(shared_ptr<Player> player);

	static shared_ptr<SendBuffer> MAKE_SC_REMOVE_PLAYER(int playerId);

	static shared_ptr<SendBuffer> MAKE_SC_MOVE_OBJECT(shared_ptr<Player> player);
};

