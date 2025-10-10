#include "pch.h"
#include "PacketHandler.h"
#include "SendBuffer.h"
#include "Room.h"
#include "Player.h"

bool Handle_CS_LOGIN(shared_ptr<Session> session, CS_LOGIN_PACKET* packet)
{
	// DB ¶Ç´Â ·£´ý
	int playerId = session->GetId();
	

	shared_ptr<Player> player = make_shared<Player>(session);
	player->SetId(playerId);
	// player->SetOwnerSession(session);
	session->_currPlayer = player;
	
	
	GRoomManager->EnterPlayer(player);

	return true;
}



bool Handle_CS_CHAT(shared_ptr<Session> session, CS_CHAT_PACKET* packet)
{
	char message[1024] = { '\0', };
	memcpy(message, packet->message, packet->header.size - sizeof(PacketHeader));
	cout << "Client [" << session->GetId() << "] : " << message << endl;
	

	
	SC_CHAT_PACKET cPacket;
	strcpy_s(cPacket.message, message);
	cPacket.header.size = packet->header.size;
	cPacket.header.type = SC_PACKET_LIST::SC_CHAT;

	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(cPacket.header.size);
	sendBuffer->CopyData(&cPacket, cPacket.header.size);

	
	// GRoom->Broadcast(sendBuffer);

	return true;
}

bool Handle_CS_MOVE(shared_ptr<Session> session, CS_MOVE_PACKET* packet)
{
	

	/*if(auto room = session->_currPlayer->GetCurrentRoom())
		room->PlayerMove(session->_currPlayer, packet->direction, packet->move_time);*/

	if (auto room = session->_currPlayer->GetCurrentRoom())
		room->PushJob(&Room::PlayerMoven, session->_currPlayer, packet->direction, packet->move_time);


	return true;
}



bool PacketHandler::ProcessPacket(shared_ptr<Session> session, BYTE* buffer, int len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	
	switch (header->type)
	{
	case CS_PACKET_LIST::CS_LOGIN:
		Handle_CS_LOGIN(session, reinterpret_cast<CS_LOGIN_PACKET*>(buffer));
		break;
	case CS_PACKET_LIST::CS_CHAT:
		Handle_CS_CHAT(session, reinterpret_cast<CS_CHAT_PACKET*>(buffer));
		break;
	case CS_PACKET_LIST::CS_MOVE:
		Handle_CS_MOVE(session, reinterpret_cast<CS_MOVE_PACKET*>(buffer));
		break;
	default:
		cout << "Unknown Packet [" << header->size<<"]bytes"  << endl;
		return false;
	}


	return true;
}
//===================================================================================
//===================================================================================
shared_ptr<SendBuffer> PacketHandler::MakePacket(shared_ptr<Player> player, SC_PACKET_LIST type)
{
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(1024);
	
	switch (type)
	{
	case SC_ADD_PLAYER:
		MAKE_SC_ADD_PLAYER(player, sendBuffer);
		break;
	case SC_MOVE_OBJECT:
		MAKE_SC_MOVE_OBJECT(player, sendBuffer);
		break;
	case SC_REMOVE_PLAYER:
		// MAKE_SC_REMOVE_PLAYER(player, sendBuffer);
		break;
	case SC_CHAT:
		break;
	
	default:
		break;
	}

	return sendBuffer;
}

bool MAKE_SC_ADD_PLAYER(shared_ptr<Player> player, shared_ptr<SendBuffer> buffer)
{
	SC_ADD_PLAYER_PACKET packet;
	packet.header = { sizeof(packet), SC_ADD_PLAYER };
	packet.objectInfo = player->GetInfo();
	buffer->CopyData(&packet, packet.header.size);

	return true;
}

shared_ptr<SendBuffer> MAKE_SC_REMOVE_PLAYER(int playerId)
{
	
	SC_REMOVE_PLAYER_PACKET packet;
	packet.header = { sizeof(packet), SC_REMOVE_PLAYER };
	packet.playerId = playerId;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

bool MAKE_SC_MOVE_OBJECT(shared_ptr<Player> player, shared_ptr<SendBuffer> buffer)
{
	SC_MOVE_PACKET packet;
	packet.header = { sizeof(packet), SC_MOVE_OBJECT };
	packet.objectInfo = player->GetInfo();
	packet.move_time = player->_last_moveTime;
	buffer->CopyData(&packet, packet.header.size);
	return true;
}
