#include "pch.h"
#include "PacketSerializer.h"
#include "Player.h"

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_ADD_PLAYER(shared_ptr<Player> player)
{
	SC_ADD_PLAYER_PACKET packet;
	packet.header = { sizeof(packet), SC_ADD_PLAYER };
	packet.objectInfo = player->GetInfo();
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);

	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_REMOVE_PLAYER(int playerId)
{

	SC_REMOVE_PLAYER_PACKET packet;
	packet.header = { sizeof(packet), SC_REMOVE_PLAYER };
	packet.playerId = playerId;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_MOVE_OBJECT(shared_ptr<Player> player)
{
	SC_MOVE_PACKET packet;
	packet.header = { sizeof(packet), SC_MOVE_OBJECT };
	packet.objectInfo = player->GetInfo();
	packet.move_time = player->_last_moveTime;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

