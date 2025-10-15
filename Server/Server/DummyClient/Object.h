#pragma once
#include "pch.h"

class Object
{
public:
	Object() = default;
	Object(sf::Texture& t, int x, int y, int x2, int y2);

	void a_move(int x, int y);
	void SetPosition(int x, int y);
	void SetScale(float x);

	void draw();
	
	std::pair<int, int> GetPosition() { return { m_x, m_y }; }

	void set_name(const char str[]);
	int GetId() const { return _id; }
	void SetId(int id) { _id = id; }

private:
	sf::Sprite m_sprite;
	sf::Text m_name;
	sf::Text m_hp;
	sf::Text m_chat;
	std::chrono::system_clock::time_point m_mess_end_time;

	int		_id = 0;
	int m_x = 0;
	int m_y = 0;
	char name[20];
};

extern Object myPlayer;              
extern std::unordered_map<int, Object> players;

