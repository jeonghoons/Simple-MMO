#pragma once

#pragma once
#include <unordered_map>

// 타격 판정 도형 모양
enum class HitShape
{
	Sector, // 부채꼴
	Line    // 직선(선분)
};

// 스킬/공격 데이터 구조체
struct SkillData
{
	int skillId;
	HitShape hitShape;
	int hitDelayMs;     // 타격 판정 지연 시간 (애니메이션 선딜레이)
	float radius;       // 부채꼴 반경 (또는 사거리)
	float angle;        // 부채꼴 각도
	float range;        // 직선 판정 사거리
	float width;        // 직선 판정 폭
	int baseDamage;     // 기본 데미지
};

// 임시 인메모리 데이터 테이블
class DataManager
{
public:
	static void Init()
	{
		// 101: 기디언 (Gideon) 기본 공격 - 에너지 투사체
		_skillTable[101] = { 101, HitShape::Line, 400, 0.f, 0.f, 1500.f, 120.f, 40 };

		// 201: 스패로우 (Sparrow) 기본 공격 - 화살
		_skillTable[201] = { 201, HitShape::Line, 300, 0.f, 0.f, 1800.f, 80.f, 35 };

		// 301: 그레이스톤 (Greystone) 기본 공격 - 검 휘두르기
		_skillTable[301] = { 301, HitShape::Sector, 200, 250.f, 120.f, 0.f, 0.f, 50 };

		// 401: 램페이지 (Monster/Rampage) 기본 공격 - 거대 팔 휘두르기
		_skillTable[401] = { 401, HitShape::Sector, 500, 300.f, 150.f, 0.f, 0.f, 60 };
	}

	static const SkillData* GetSkillData(int skillId)
	{
		auto it = _skillTable.find(skillId);
		if (it != _skillTable.end())
			return &it->second;
		return nullptr;
	}

private:
	static inline std::unordered_map<int, SkillData> _skillTable;
};