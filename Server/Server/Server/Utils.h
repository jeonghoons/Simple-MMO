#pragma once
#include <random>

class Utils
{
public:
	template<typename T>
	static T GetRandom(T min, T max)
	{
		thread_local std::random_device rd;
		thread_local std::mt19937 generator(rd());
		if constexpr (std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(generator);
		}
		else
		{
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(generator);
		}
	}
};

struct Cooldown
{
	std::chrono::steady_clock::time_point lastTick;
	long long durationMs = 0;

	bool IsReady() const
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
		return elapsed >= durationMs;
	}

	void Reset(long long newDurationMs)
	{
		lastTick = std::chrono::steady_clock::now();
		durationMs = newDurationMs;
	}
};

class Physics
{
public:
	static bool CheckSector(const PositionInfo& origin, const XMFLOAT3& forward, const PositionInfo& target, float radius, float angleDegree)
	{
		// 1. 거리 체크 (성능을 위해 제곱으로 비교)
		float diffX = target.x - origin.x;
		float diffY = target.y - origin.y;
		float distSq = diffX * diffX + diffY * diffY;

		if (distSq > radius * radius) return false;

		// 2. 각도 체크 (내적 이용)
		XMVECTOR vForward = XMVector3Normalize(XMLoadFloat3(&forward));
		XMVECTOR vToTarget = XMVector3Normalize(XMVectorSet(diffX, diffY, 0.0f, 0.0f));

		// 두 벡터의 내적 (Cosθ)
		float dot = XMVectorGetX(XMVector3Dot(vForward, vToTarget));

		// 부채꼴의 절반 각도에 대한 Cos값보다 크면 범위 안
		float halfAngleRad = XMConvertToRadians(angleDegree * 0.5f);
		float cosHalf = cosf(halfAngleRad);

		return dot >= cosHalf;
	}
};

