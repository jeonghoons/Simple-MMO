#include "pch.h"
#include "AuthLobby.h"
#include "Session.h"
#include "Player.h"

AuthLobby::AuthLobby(HANDLE iocpHandle) : _jobQueue(make_shared<JobQueue>(iocpHandle))
{
}

AuthLobby::~AuthLobby() = default;

void AuthLobby::OnLoginSuccess(shared_ptr<Session> session, wstring userId, int playerType)
{
	if (session->IsConnected() == false) return;

	int playerId = session->GetId();
	shared_ptr<Player> player = make_shared<Player>(session);
	player->SetId(playerId);
	if (userId == L"dummy")
		player->isDummy = true;
	player->SetPlayerType(static_cast<PlayerType>(playerType));
	session->_currPlayer = player;

	SC_LOGIN_INFO_PACKET logInPacket;
	logInPacket.header = { sizeof(SC_LOGIN_INFO_PACKET), SC_LOGIN };
	logInPacket.objectInfo = player->GetInfo();
	shared_ptr<SendBuffer> loginInfoBuffer = make_shared<SendBuffer>(sizeof(logInPacket));
	loginInfoBuffer->CopyData(&logInPacket, sizeof(logInPacket));
	session->Send(loginInfoBuffer);

	wcout << L"[" << userId << L"] - 로그인 성공" << std::endl;
}

void AuthLobby::OnLoginFailed(shared_ptr<Session> session, string errorMsg)
{
	if (session->IsConnected() == false) return;

	
	cout << "Login Failed: " << errorMsg << endl;
}