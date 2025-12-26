#include "pch.h"
#include "PacketHandler.h"
#include "SendBuffer.h"
#include "Room.h"
#include "Player.h"

void PacketHandler::Handle_CS_LOGIN(shared_ptr<Session> session, CS_LOGIN_PACKET* packet)
{
	int playerId = session->GetId();

	// DB ·Î±×ÀÎ
	

	shared_ptr<Player> player = make_shared<Player>(session);
	player->SetId(playerId);
	session->_currPlayer = player;

	GRoomManager->EnterPlayer(player);

}

void PacketHandler::Handle_CS_CHAT(shared_ptr<Session> session, CS_CHAT_PACKET* packet)
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

}

void PacketHandler::Handle_CS_MOVE(shared_ptr<Session> session, CS_MOVE_PACKET* packet)
{
	if (auto room = session->_currPlayer->GetCurrentRoom())
		room->PushJob(&Room::PlayerMove, session->_currPlayer, packet->direction, packet->move_time);

}

void PacketHandler::Handle_CS_CMOVE(shared_ptr<Session> session, CS_CMOVE_PACKET* packet)
{

	if (auto room = session->_currPlayer->GetCurrentRoom())
		room->PushJob(&Room::PlayerCMove, session->_currPlayer, packet->pos);
	

}



void PacketHandler::ProcessPacket(shared_ptr<Session> session, BYTE* buffer, int len)
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
	case CS_PACKET_LIST::CS_CMOVE:
		Handle_CS_CMOVE(session, reinterpret_cast<CS_CMOVE_PACKET*>(buffer));
		break;

	case CS_PACKET_LIST::CS_LOGOUT:
		session->Disconnect((const WCHAR*)"LogOut");
		break;
	default:
		cout << "Unknown Packet [" << header->size << "]bytes" << endl;
		break;
	}


}

//===================================================================================
//===================================================================================


