#include "pch.h"
#include "CombatProcessor.h"
#include "Room.h"
#include "Player.h"
#include "Monster.h"
#include "ServerData.h"

void CombatProcessor::ProcessSkillHit(std::shared_ptr<Room> room, std::shared_ptr<Character> attacker, int skillId)
{
	if (attacker == nullptr /* || attacker->GetStat().IsDead() */) return;

	const SkillData* skillData = DataManager::GetSkillData(skillId);
	if (skillData == nullptr) return;

	Object_Type targetTypeToFind = (attacker->GetType() == Object_Type::Player) ? Object_Type::Monster : Object_Type::Player;

	for (int viewId : attacker->_viewList)
	{
		auto targetObj = room->GetGameObject(viewId);
		if (!targetObj || targetObj->GetType() != targetTypeToFind) continue;

		auto targetChar = std::static_pointer_cast<Character>(targetObj);
		if (targetChar->GetStat().IsDead()) continue;

		bool bIsHit = false;

		// 4. 데이터에 정의된 판정 도형(Shape)에 맞춰 수학 연산 라우팅
		if (skillData->hitShape == HitShape::Sector)
		{
			bIsHit = MathUtils::CheckSector(attacker->GetPosition(), targetChar->GetPosition(), skillData->radius, skillData->angle);
		}
		else if (skillData->hitShape == HitShape::Line)
		{
			bIsHit = MathUtils::CheckLine(attacker->GetPosition(), targetChar->GetPosition(), skillData->width, skillData->range);
		}

		// 5. 타격 성공 시 데미지 처리 및 벽 뒤(장애물) 판정
		if (bIsHit)
		{
			bool hitRayCast = room->GetNavManager()->RayCast(attacker->GetPosition(), targetChar->GetPosition());

			if (hitRayCast)
			{
				int finalDamage = skillData->baseDamage;
				targetChar->OnDamaged(finalDamage, attacker);

				std::cout << "Attacker[" << attacker->GetId() << "] Hit Target[" << targetChar->GetId() << "] with Skill[" << skillId << "] / Dmg: " << finalDamage << std::endl;
			}
			else
			{
				// 수학적으로 거리는 맞았으나, 벽 뒤에 있어서 맞지 않음
				std::cout << "Target[" << targetChar->GetId() << "] blocked attack from Attacker[" << attacker->GetId() << "] due to obstacle." << std::endl;
			}
		}
	}
}
