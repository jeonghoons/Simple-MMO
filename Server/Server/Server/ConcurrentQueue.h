#pragma once
#include <condition_variable>

template <typename T>
class ConcurrentQueue {
public:
    void Push(T item)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(item));
    }

    T TryPop()
    {
        lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty())
            return nullptr;

        T value = _queue.front();
        _queue.pop();
        return value;
    }

    bool TryPop(T& item)
    {
        lock_guard<mutex> lock(_mutex);
        if (_queue.empty())
            return false;

        item = std::move(_queue.front());
        _queue.pop();
        return true;
    }


    size_t Getsize() const {
        lock_guard<std::mutex> lock(_mutex);
        return _queue.size(); }
    bool Empty() { 
        lock_guard<std::mutex> lock(_mutex);
        return _queue.empty(); }


private:
    queue<T> _queue;
    mutex _mutex;
};

