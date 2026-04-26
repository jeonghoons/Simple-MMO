#pragma once
#include "pch.h"

// 전방 선언 (헤더 종속성 최소화)
class Room;
class Player;
class Monster;

class CombatProcessor
{
public:
	static void ProcessSkillHit(std::shared_ptr<Room> room, std::shared_ptr<Character> attacker, int skillId);

};
