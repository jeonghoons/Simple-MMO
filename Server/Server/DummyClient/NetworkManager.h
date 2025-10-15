#pragma once
#include "pch.h"
#include "RecvBuffer.h"
#include "Object.h"

class GameClient;

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	bool Connect2Server();
	void Send_packet(void* buffer);
	int RecvPacket();

   
    void SendLoginPacket(); 
    void SendMovePacket(int direction);

private:
	int process_data(BYTE* net_buf, int io_byte);
	void ProcessPacket(BYTE* net_buf, int io_byte);

private:
	sf::TcpSocket		_socket;
	RecvBuffer	_recvBuffer{ 65536 };
	char			_sendBuffer[1024] = { 0, };
};

