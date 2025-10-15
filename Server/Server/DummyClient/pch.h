#pragma once
#define SFML_STATIC 1

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <Windows.h>

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <utility>
using namespace std;

#include "..//Server/TestProtocol.h"


using BYTE = unsigned char;

#define PORT_NUM 8888

extern sf::RenderWindow* g_window;
extern sf::Font* g_font;       // GameClient::_font의 포인터를 담을 변수
extern sf::Texture* pieces;    // GameClient::_pieces의 포인터를 담을 변수
extern int g_left_x;           // 맵 시야 기준점 X
extern int g_top_y;            // 맵 시야 기준점 Y
extern int g_myid;             // 내 캐릭터 ID
extern const int MAP_GRID;
extern const int TILE_WIDTH;   // 타일 크기 (상수)
extern const int SCREEN_WIDTH; // 화면 너비 (상수)
extern const int SCREEN_HEIGHT;// 화면 높이 (상수)



