#include "pch.h"
#include "Timer.h"

void Timer::Run()
{
    while (true)
    {
        ProcessTimerQueue();

        // 큐가 비어있지 않다면, 가장 빠른 작업까지 대기
        std::chrono::milliseconds waitTime(10); // 기본 대기 시간 10ms


        if (false == _timerQueue.empty()) {
            TimerItem item{};
            if (_timerQueue.try_pop(item)) {
                TimePoint now = std::chrono::steady_clock::now();

                _timerQueue.push(item);

                if (now < item._executeTime) // 실행할 아이템이 있음
                {
                    // 다음 작업까지 남은 시간 계산
                    auto duration = item._executeTime - now;
                    waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

                    // 최대 대기 시간을 제한하여 너무 길게 블로킹되는 것을 방지
                    if (waitTime.count() > 10) { // 예: 최대 1초 대기
                        waitTime = std::chrono::milliseconds(10);
                    }
                }
                else
                {
                    // 이미 실행 시간이 지났다면 즉시 다시 큐를 처리하도록 대기 시간 0
                    waitTime = std::chrono::milliseconds(0);
                }

            }
            
        }


        std::this_thread::sleep_for(waitTime);
    }
}

void Timer::ProcessTimerQueue()
{
    TimePoint now = std::chrono::steady_clock::now();

    
    while (true)
    {
        TimerItem item{};

        if (false == _timerQueue.try_pop(item))
            break;


        if (item._executeTime <= now)
        {
            if (std::shared_ptr<JobQueue> jobQueue = item._jobQueue.lock())
            {
                // Job을 목표 Room의 JobQueue에 Push (이 JobQueue의 Push는 스레드 안전해야 함)
                jobQueue->Push(item._job);
            }
        }
        else
        {
            _timerQueue.push(item);
            break;
        }
    }

}
