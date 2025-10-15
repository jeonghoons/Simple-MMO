#pragma once
#include "Object.h"
#include <map>
class NetworkManager;


class GameClient
{

public:
    GameClient() = default;
    ~GameClient() = default;

    void Initialize(int windowWidth, int windowHeight);
    void Finalize();

    void Update(); // 네트워크 수신 후 객체 상태 업데이트
    void Render(); 
    void HandleInput(const sf::Event& event, NetworkManager& network);

    
private:
    // 기존 전역 변수였던 리소스와 게임 상태
    std::unique_ptr<sf::Texture> _board;
    std::unique_ptr<sf::Texture> _pieces;
    std::unique_ptr<sf::Font> _font;

    std::vector<std::vector<Object>> _tileMap;
};

