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

