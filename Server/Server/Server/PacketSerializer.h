#pragma once


class PacketSerializer
{
public:
	
public:
	static shared_ptr<SendBuffer> MAKE_SC_ADD_OBJECT(shared_ptr<GameObject> object);

	static shared_ptr<SendBuffer> MAKE_SC_REMOVE_OBJECT(int ObjectId);

	static shared_ptr<SendBuffer> MAKE_SC_MOVE_OBJECT(shared_ptr<GameObject> object);
};

