#pragma once
#include "pch.h"
#include "RecvBuffer.h"
#include "Object.h"

class Network
{
public:
	Network();
	~Network();

	bool Connect2Server();

	void Send_packet(void* buffer)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(buffer);
		size_t sent = 0;
		_socket.send(buffer, p[0], sent);
		cout << sent << "Bytes Send" << endl;
	}

	int RecvPacket()
	{
		size_t received = 0;
		auto recv_result = _socket.receive(_recvBuffer.WritePos(), _recvBuffer.FreeSize(), received);
		if (recv_result == sf::Socket::Error)
		{
			wcout << L"Recv 에러!";
			exit(-1);
		}
		if (recv_result == sf::Socket::Disconnected) {
			wcout << L"Disconnected\n";
			exit(-1);
		}
		
		if (recv_result != sf::Socket::NotReady)
			if (received > 0) process_data(_recvBuffer.ReadPos(), received);
		
		return received;
	}

	int process_data(BYTE* net_buf, int io_byte)
	{
		int processLen = 0;

		while (true)
		{
			int dataSize = io_byte - processLen;
			if (dataSize < sizeof(PacketHeader))
				break;

			PacketHeader header = *(reinterpret_cast<PacketHeader*>(&net_buf[processLen]));
			if (dataSize < header.size)
				break;

			// 패킷 조립 성공
			ProcessPacket(&net_buf[processLen], header.size);

			processLen += header.size;

		}
		return processLen;

	}

	void ProcessPacket(BYTE* net_buf, int io_byte)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(net_buf);

		switch (header->type)
		{
		case SC_LOGIN:
		{
			cout << "LogIN " << io_byte << "Bytes " << endl;
			SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(net_buf);
			g_myid = packet->objectInfo.id;
			myPlayer.SetId(g_myid);
			int posX = (int)packet->objectInfo.position.x;
			int posY = (int)packet->objectInfo.position.y;
			myPlayer.SetPosition(posX, posY);
			g_left_x = posX - SCREEN_WIDTH / 2;
			g_top_y = posY - SCREEN_HEIGHT / 2;

		}
			break;
		case SC_PACKET_LIST::SC_ADD_PLAYER:
		{
			cout << "ADD PLAYER " << io_byte << "Bytes " << endl;

			SC_ADD_PLAYER_PACKET* packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(net_buf);
			int id = packet->objectInfo.id;
			int posX = packet->objectInfo.position.x;
			int posY = packet->objectInfo.position.y;

			if (id == g_myid) {
				myPlayer.SetPosition(posX, posY);
				g_left_x = posX - SCREEN_WIDTH / 2;
				g_top_y = posY - SCREEN_HEIGHT / 2;
			}
			else {
				players[id] = Object{ *pieces, 180-1, 0,TILE_WIDTH - 1, TILE_WIDTH - 1 };
				players[id].SetPosition(posX, posY);
			}
		}
			break;

		case SC_PACKET_LIST::SC_ADD_OBJECT:
		{
			cout << "ADD MONSTER " << io_byte << "Bytes " << endl;

			SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(net_buf);
			int id = packet->objectInfo.id;
			int posX = packet->objectInfo.position.x;
			int posY = packet->objectInfo.position.y;

			players[id] = Object{ *pieces, 88, 0,TILE_WIDTH - 1, TILE_WIDTH - 1 };
			players[id].SetPosition(posX, posY);
		}break;

		case SC_PACKET_LIST::SC_REMOVE_OBJECT:
		{
			cout << "DELETE MONSTER " << io_byte << "Bytes " << endl;
			SC_REMOVE_OBJECT_PACKET* packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(net_buf);
			int id = packet->objectId;

			auto it = players.find(id);
			if (it != players.end())
				players.erase(it);
		}break;

		case SC_PACKET_LIST::SC_REMOVE_PLAYER:
		{
			cout << "DELETE PLAYER " << io_byte << "Bytes " << endl;
			SC_REMOVE_PLAYER_PACKET* packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(net_buf);
			int id = packet->playerId;

			auto it = players.find(id);
			if (it != players.end())
				players.erase(it);
		}
			break;

		case SC_PACKET_LIST::SC_MOVE_OBJECT:
		{
			// cout << "MOVE OBJECT " << io_byte << "Bytes " << endl;
			SC_MOVE_PACKET* packet = reinterpret_cast<SC_MOVE_PACKET*>(net_buf);
			int id = packet->objectInfo.id;
			auto pos = packet->objectInfo.position;
			int posX = pos.x;
			int posY = pos.y;

			if (id == g_myid) {
				
				myPlayer.SetPosition(posX, posY);
				g_left_x = posX - SCREEN_WIDTH / 2;
				g_top_y = posY - SCREEN_HEIGHT / 2;
			}
			else {
				players[id].SetPosition(posX, posY);
			}
			cout << "OBJECT [" << id << "] MOVE " << endl;
		}
		break;

		case SC_PACKET_LIST::SC_CHAT:
			cout << "Chatting " << io_byte << "Bytes " << endl;
			break;

		default:
			break;
		}
	}

private:
	sf::TcpSocket		_socket;
	RecvBuffer	_recvBuffer;
	char			_sendBuffer[1024] = { 0, };


};

