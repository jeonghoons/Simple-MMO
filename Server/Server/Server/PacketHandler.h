#pragma once
#include "TestProtocol.h"

#include "Session.h"
#include "SendBuffer.h"



class PacketHandler
{
public:
	static void ProcessPacket(shared_ptr<Session> session, BYTE* buffer, int len);
	
private:
	static void Handle_CS_LOGIN(shared_ptr<Session> session, CS_LOGIN_PACKET* packet);
	static void Handle_CS_CHAT(shared_ptr<Session> session, CS_CHAT_PACKET* packet);
	static void Handle_CS_MOVE(shared_ptr<Session> session, CS_MOVE_PACKET* packet);

};


