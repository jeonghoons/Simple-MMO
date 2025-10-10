#pragma once
#include "JobQueue.h"
#include <concurrent_priority_queue.h>

using TimePoint = std::chrono::steady_clock::time_point;

struct TimerItem
{
    bool operator<(const TimerItem& other) const 
    {
        return _executeTime > other._executeTime;
    }
    TimerItem() = default;
    TimerItem(TimePoint time, std::shared_ptr<Job> job, std::shared_ptr<JobQueue> jobQueue)
        : _executeTime(time), _job(std::move(job)), _jobQueue(jobQueue) {
    }

    TimePoint _executeTime{};                   // 실행 시각
    std::shared_ptr<Job> _job{};               // 실행할 Job
    std::weak_ptr<JobQueue> _jobQueue{};    // Job을 넣을 Room의 JobQueue (weak_ptr로 안전하게 참조)

};

class Timer
{
public:
    /*struct TimerItemCompare {
        bool operator()(const TimerItem& a, const TimerItem& b) const {
            return a._executeTime > b._executeTime;
        }
    };*/
    Timer() : _stopFlag(false)
    {
        // 타이머 스레드를 시작합니다.
        _thread = std::make_unique<std::thread>(&Timer::Run, this);
    }

    ~Timer()
    {
        _stopFlag.store(true);
        if (_thread && _thread->joinable())
        {
            _thread->join();
        }
    }

    // 타이머 작업 예약 메소드
    template<typename T, typename... Arguments>
    void Reserve(DWORD milliseconds,shared_ptr<JobQueue> jobQueue, shared_ptr<T> owner, void(T::* memFunc)(Arguments...), Arguments&&... args)
    {
        TimePoint executeTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        shared_ptr<Job> job = std::make_shared<Job>(owner, memFunc, std::forward<Arguments>(args)...);

        TimerItem item = TimerItem(executeTime, std::move(job), jobQueue);
        // lock_guard<mutex> lock(_lock);
        _timerQueue.push(item);
    }

private:
    void Run();

    void ProcessTimerQueue();

private:
    mutex       _lock;
    // priority_queue<TimerItem> _timerQueue;
    concurrency::concurrent_priority_queue<TimerItem> _timerQueue;
    std::unique_ptr<std::thread> _thread;
    std::atomic<bool> _stopFlag;
};


