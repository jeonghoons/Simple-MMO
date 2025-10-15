#include "pch.h"
#include "Object.h"

Object myPlayer;
std::unordered_map<int, Object> players;

// 기존 코드와 동일
Object::Object(sf::Texture& t, int x, int y, int x2, int y2) {
    m_sprite.setTexture(t);
    m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
    set_name("NONAME");
    m_mess_end_time = std::chrono::system_clock::now();
}

void Object::a_move(int x, int y) {
    m_sprite.setPosition((float)x, (float)y);
}

void Object::SetPosition(int x, int y) { m_x = x; m_y = y; }

void Object::SetScale(float x) { m_sprite.setScale(x, x); }

void Object::draw() {
    float rx = (m_x - g_left_x) * TILE_WIDTH + 1;
    float ry = (m_y - g_top_y) * TILE_WIDTH + 1;
    m_sprite.setPosition(rx, ry); // 

    if (g_window) {
        g_window->draw(m_sprite);
        // auto size = m_name.getGlobalBounds();
        // m_hp.setPosition(rx + 32 - size.width / 2, ry - 30);
        // g_window->draw(m_hp);
        /*if (m_mess_end_time < chrono::system_clock::now()) {
            m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
            g_window->draw(m_name);
        }
        else {
            m_chat.setPosition(rx + 32 - size.width / 2, ry - 10);
            g_window->draw(m_chat);
        } */
    }
}

void Object::set_name(const char str[]) {
    m_name.setFont(*g_font);
    m_name.setFillColor(sf::Color(255, 255, 0));
    m_name.setStyle(sf::Text::Bold);
}