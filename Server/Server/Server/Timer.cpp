#include "pch.h"
#include "Timer.h"

//void Timer::Run()
//{
//    while (!_stopFlag.load())
//    {
//        ProcessTimerQueue();
//
//        // 큐가 비어있지 않다면, 가장 빠른 작업까지 대기
//        std::chrono::milliseconds waitTime(10); // 기본 대기 시간 10ms
//
//        if (false == _timerQueue.empty())
//        {
//            const TimerItem& item = _timerQueue.top();
//
//            TimePoint now = std::chrono::steady_clock::now();
//
//            if (now < item._executeTime) // 실행할 아이템이 있음
//            {
//                // 다음 작업까지 남은 시간 계산
//                auto duration = item._executeTime - now;
//                waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
//
//                // 최대 대기 시간을 제한하여 너무 길게 블로킹되는 것을 방지
//                if (waitTime.count() > 10) { // 예: 최대 1초 대기
//                    waitTime = std::chrono::milliseconds(10);
//                }
//            }
//            else
//            {
//                // 이미 실행 시간이 지났다면 즉시 다시 큐를 처리하도록 대기 시간 0
//                waitTime = std::chrono::milliseconds(0);
//            }
//        }
//        
//
//        std::this_thread::sleep_for(waitTime);
//    }
//}
//
//void Timer::ProcessTimerQueue()
//{
//    TimePoint now = std::chrono::steady_clock::now();
//
//    while (false == _timerQueue.empty())
//    {
//        const TimerItem& item = _timerQueue.top();
//        if (item._executeTime <= now)
//        {
//            TimerItem executeItem = item;
//            lock_guard<mutex> lock(_lock);
//            _timerQueue.pop();
//
//            if (std::shared_ptr<JobQueue> jobQueue = executeItem._jobQueue.lock())
//            {
//                // Job을 목표 Room의 JobQueue에 Push (이 JobQueue의 Push는 스레드 안전해야 함)
//                jobQueue->Push(executeItem._job);
//            }
//        }
//        else
//        {
//            // 아직 실행 시간이 되지 않은 항목이 맨 위에 있으므로 종료
//            break;
//        }
//    }
//
//}

void Timer::Run()
{
    while (true)
    {
        ProcessTimerQueue();

        // 큐가 비어있지 않다면, 가장 빠른 작업까지 대기
        std::chrono::milliseconds waitTime(10); // 기본 대기 시간 10ms

        TimerItem item{};

        if (false == _timerQueue.try_pop(item))
        {
            TimePoint now = std::chrono::steady_clock::now();

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


        std::this_thread::sleep_for(waitTime);
    }
}

void Timer::ProcessTimerQueue()
{
    TimePoint now = std::chrono::steady_clock::now();

    TimerItem item{};
    while (false == _timerQueue.try_pop(item))
    {
        
        if (item._executeTime <= now)
        {
            TimerItem executeItem = item;
           
            if (std::shared_ptr<JobQueue> jobQueue = executeItem._jobQueue.lock())
            {
                // Job을 목표 Room의 JobQueue에 Push (이 JobQueue의 Push는 스레드 안전해야 함)
                jobQueue->Push(executeItem._job);
            }
        }
        else
        {
            // 아직 실행 시간이 되지 않은 항목이 맨 위에 있으므로 종료
            break;
        }
    }

}
