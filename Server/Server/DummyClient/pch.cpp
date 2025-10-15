#include "pch.h"

sf::RenderWindow* g_window = nullptr;
sf::Font* g_font = nullptr;
sf::Texture* pieces = nullptr;
int g_left_x = 0;
int g_top_y = 0;
int g_myid = 0;

// 상수 정의 (임의의 값)
const int MAP_GRID = 4;
const int TILE_WIDTH = 32;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;