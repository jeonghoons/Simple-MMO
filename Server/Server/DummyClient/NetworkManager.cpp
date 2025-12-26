#include "pch.h"
#include "NetworkManager.h"
#include "GameClient.h"



NetworkManager::NetworkManager()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        exit(-1);
    }
}

NetworkManager::~NetworkManager()
{
    WSACleanup();
}

bool NetworkManager::Connect2Server()
{
    wcout.imbue(locale("korean"));
    sf::Socket::Status status = _socket.connect("127.0.0.1", PORT_NUM);
    _socket.setBlocking(false);

    if (status != sf::Socket::Done) {
        wcout << L"서버와 연결할 수 없습니다.\n";
        return false;
    }

    return true;
}

void NetworkManager::Send_packet(void* buffer)
{
    unsigned char* p = reinterpret_cast<unsigned char*>(buffer);
    size_t sent = 0;
    _socket.send(buffer, p[0], sent);
    std::cout << sent << "Bytes Send" << std::endl;
}



int NetworkManager::RecvPacket()
{
    size_t received = 0;
    auto recv_result = _socket.receive(_recvBuffer.WritePos(), _recvBuffer.FreeSize(), received);

    if (recv_result == sf::Socket::Error) {
        std::wcout << L"Recv 에러!";
        exit(-1);
    }
    if (recv_result == sf::Socket::Disconnected) {
        std::wcout << L"Disconnected\n";
        exit(-1);
    }

   /* if (received > 0) {
        _recvBuffer.OnWrite(received);
        int processedLen = process_data(_recvBuffer.ReadPos(), _recvBuffer.DataSize());
        _recvBuffer.OnRead(processedLen);
    }*/


    if (received > 0) {
        if (_recvBuffer.OnWrite(received) == false) {
            return -1;
        }
        int dataSize = _recvBuffer.DataSize();

        int processLen = process_data(_recvBuffer.ReadPos(), dataSize);
        if (_recvBuffer.OnRead(processLen) == false)
        {
            return -1;
        }
    }
    _recvBuffer.CleanCheck();
    

    return received;
}

int NetworkManager::process_data(BYTE* net_buf, int io_byte)
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

        ProcessPacket(&net_buf[processLen], header.size);

        processLen += header.size;
    }
    return processLen;
}

void NetworkManager::ProcessPacket(BYTE* net_buf, int io_byte)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(net_buf);

    switch (header->type)
    {
    case SC_PACKET_LIST::SC_LOGIN: 
    {
        std::cout << "LogIN " << io_byte << "Bytes " << std::endl;
        SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(net_buf);

        g_myid = packet->objectInfo.id;
        myPlayer.SetId(g_myid);
        int posX = (int)packet->objectInfo.position.x;
        int posY = (int)packet->objectInfo.position.y;
        myPlayer.SetPosition(posX, posY);
        g_left_x = posX - SCREEN_WIDTH / 2;
        g_top_y = posY - SCREEN_HEIGHT / 2;
    }break;
    case SC_PACKET_LIST::SC_ADD_OBJECT:
    {
        std::cout << "ADD PLAYER " << io_byte << "Bytes " << std::endl;

        SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(net_buf);
        int id = packet->objectInfo.id;
        int posX = (int)packet->objectInfo.position.x;
        int posY = (int)packet->objectInfo.position.y;

        if (id == g_myid) { // 자신
            myPlayer.SetPosition(posX, posY);
            g_left_x = posX - SCREEN_WIDTH / 2;
            g_top_y = posY - SCREEN_HEIGHT / 2;
        }
        else if (id >= 100000) { // 몬스터
            players[id] = Object{ *pieces, 32, 0, TILE_WIDTH, TILE_WIDTH};
            players[id].SetPosition(posX, posY);
        }
        else { // 다른 클라이언트
            players[id] = Object{ *pieces, 64, 0, TILE_WIDTH, TILE_WIDTH };
            players[id].SetPosition(posX, posY);
        }
    } break;
    case SC_PACKET_LIST::SC_REMOVE_OBJECT:
    {
        cout << "DELETE PLAYER " << io_byte << "Bytes " << endl;
        SC_REMOVE_OBJECT_PACKET* packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(net_buf);
        int id = packet->objectId;

        auto it = players.find(id);
        if (it != players.end())
            players.erase(it);
    }break;
    case SC_MOVE_OBJECT:
    {
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
    } break;

   
    default:
        cout << "Unkown Packet" << endl;
        break;
    }
}

void NetworkManager::SendLoginPacket()
{
    CS_LOGIN_PACKET p;
    p.header.size = sizeof(p);
    p.header.type = CS_PACKET_LIST::CS_LOGIN;

    // 일반 송신 함수 호출
    Send_packet(&p);
}

void NetworkManager::SendMovePacket(int direction)
{
    CS_MOVE_PACKET p;
    p.header.size = sizeof(p);
    p.header.type = CS_PACKET_LIST::CS_MOVE; // 클라이언트가 서버로 요청하는 패킷 타입
    p.direction = direction;
    // 일반 송신 함수 호출
    Send_packet(&p);
}

