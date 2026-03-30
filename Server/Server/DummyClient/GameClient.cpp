#include "pch.h"
#include "GameClient.h"
#include "NetworkManager.h"
#include "Object.h"

void GameClient::Initialize(int windowWidth, int windowHeight)
{
    _board = std::make_unique<sf::Texture>();
    _pieces = std::make_unique<sf::Texture>();
    _font = std::make_unique<sf::Font>();

    if (false == _board->loadFromFile("Gamemap.PNG")) {
        std::cerr << "Board Loading Error!\n";
        exit(-1);
    }
    if (false == _pieces->loadFromFile("charactersheet_128x32.PNG")) {
        std::cerr << "Pieces Loading Error!\n";
        exit(-1);
    }
    if (false == _font->loadFromFile("cour.ttf")) {
        std::cerr << "Font Loading Error!\n";
        exit(-1);
    }
    
    ::pieces = _pieces.get();
    ::g_font = _font.get();

    myPlayer = Object{ *_pieces, 0, 0, TILE_WIDTH, TILE_WIDTH };
    // myPlayer = Object{ *_pieces, 44, 0, 44, 44 };
    // myPlayer.SetPosition(0, 0);

    // 맵 초기화
    //for (int i = 0; i < 32; i++) { // MAP_SIZE 대신 32로 가정
    //    _tileMap.emplace_back(std::vector<Object>());
    //    for (int j = 0; j < 32; j++) {
    //        int tile_x = i * (512 / 32);
    //        int tile_y = j * (512 / 32);

    //        Object tile(*_board, tile_x, tile_y, 32, 32);
    //        tile.SetPosition(i, j);
    //        // tile.SetScale(float(TILE_WIDTH) / 16.f);
    //        _tileMap[i].emplace_back(tile);
    //    }
    //}

    
    for (int i = 0; i < 100; i++) { 
        _tileMap.emplace_back(std::vector<Object>());
        for (int j = 0; j < 100; j++) {
            int tile_x = i * MAP_GRID;
            int tile_y = j * MAP_GRID;

            Object tile(*_board, tile_x, tile_y, MAP_GRID, MAP_GRID);
            tile.SetPosition(i, j);
            tile.SetScale(TILE_WIDTH / (float)MAP_GRID);
            
            _tileMap[i].emplace_back(tile);
        }
    }

   

}

void GameClient::Finalize()
{
    _tileMap.clear();
   
    players.clear();
    // g_font, pieces 전역 포인터는 여기서 해제하지 않습니다.
}

void GameClient::Update()
{
    // (로직 없음)
}

void GameClient::Render()
{
    for (auto& tiles : _tileMap)
        for (auto& tile : tiles)
            tile.draw();

    myPlayer.draw();

  
    for (auto& pair : players) 
        pair.second.draw();

    if (g_window && g_font)
    {
        sf::Text text;
        text.setFont(*g_font);
        char buf[100];

        auto pos = myPlayer.GetPosition();
        sprintf_s(buf, "(%d, %d)", pos.first, pos.second);

        text.setString(buf);
        g_window->draw(text);
    }
}

void GameClient::HandleInput(const sf::Event& event, NetworkManager& network)
{
    if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased) {
        bool isPressed = (event.type == sf::Event::KeyPressed);
        static bool up = false, down = false, left = false, right = false;
        switch (event.key.code) {
        case sf::Keyboard::Up:    up = isPressed;    break;
        case sf::Keyboard::Down:  down = isPressed;  break;
        case sf::Keyboard::Left:  left = isPressed;  break;
        case sf::Keyboard::Right: right = isPressed; break;
        default: return;
        }

        PositionInfo& pos = myPlayer.GetPositionInfo();
        float inputX = 0.f;
        float inputY = 0.f;

        if (left)  inputX -= 1.f;
        if (right) inputX += 1.f;
        if (up)    inputY -= 1.f; // 좌표계에 따라 -1.f 일 수도 있음
        if (down)  inputY += 1.f;

        pos.inputX = inputX;
        pos.inputY = inputY;

        network.SendMovePacket(pos);
    }
    
}
