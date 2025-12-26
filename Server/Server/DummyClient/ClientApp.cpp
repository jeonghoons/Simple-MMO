
#include "pch.h"
#include "ClientApp.h"
#include "NetworkManager.h"
#include "GameClient.h"
#include <iostream>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

ClientApp::ClientApp()
{
    _networkManager = std::make_unique<NetworkManager>();
    _gameClient = std::make_unique<GameClient>();
}

ClientApp::~ClientApp() = default;

void ClientApp::Run()
{
    Initialize();
    MainLoop();
    Finalize();
}

void ClientApp::Initialize()
{
    _window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");

    g_window = _window.get();

    _gameClient->Initialize(WINDOW_WIDTH, WINDOW_HEIGHT);

    if (_networkManager->Connect2Server() == false)
    {
        // std::cerr << "Failed to connect to Server" << std::endl;
        exit(-1);
    }

    _networkManager->SendLoginPacket();
    
}

void ClientApp::HandleEvents()
{
    sf::Event event;
    while (_window->pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            _window->close();

        _gameClient->HandleInput(event, *_networkManager);

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            _window->close();
        }
    }
}

void ClientApp::MainLoop()
{
    sf::Clock clock;
    const sf::Time targetFrameTime = sf::seconds(1.f / 60.f);

    while (_window->isOpen())
    {
        sf::Time startTime = clock.getElapsedTime();

        HandleEvents();
        _networkManager->RecvPacket();
        _gameClient->Update();

        _window->clear();
        _gameClient->Render(); 
        _window->display();

        sf::Time elapsed = clock.getElapsedTime() - startTime;
        sf::Time sleepTime = targetFrameTime - elapsed;
        if (sleepTime > sf::Time::Zero) {
            sf::sleep(sleepTime);
        }
    }
}

void ClientApp::Finalize()
{
    _gameClient->Finalize();
    g_window = nullptr; // 전역 포인터 해제
    std::cout << "Client Finished." << std::endl;
}