#pragma once
#include "pch.h"

class NetworkManager;
class GameClient;

class ClientApp
{
public:
    ClientApp();
    ~ClientApp();

    void Run();

private:
    void Initialize();
    void HandleEvents();
    void MainLoop();
    void Finalize();

private:
    std::unique_ptr<sf::RenderWindow> _window;
    std::unique_ptr<NetworkManager> _networkManager;
    std::unique_ptr<GameClient> _gameClient;

    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
};

