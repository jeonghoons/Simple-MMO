#pragma once
#include "pch.h"
#include <shared_mutex>

class RWLock {
public:
    // 읽기용 RAII 락
    class ReadGuard {
    public:
        ReadGuard(RWLock& lock) : _lock(lock) {
            _lock._mutex.lock_shared();  // 읽기 락 획득
        }
        ~ReadGuard() {
            _lock._mutex.unlock_shared(); // 읽기 락 해제
        }
    private:
        RWLock& _lock;
    };

    // 쓰기용 RAII 락
    class WriteGuard {
    public:
        WriteGuard(RWLock& lock) : _lock(lock) {
            // _lock._mutex.lock();  // 쓰기 락 획득
        }
        ~WriteGuard() {
            // _lock._mutex.unlock(); // 쓰기 락 해제
        }
    private:
        RWLock& _lock;
    };

private:
    std::shared_mutex _mutex; // 내부 mutex
};


/*----------------
    LockGuards
-----------------*/



