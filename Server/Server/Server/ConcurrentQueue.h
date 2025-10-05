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

    vector<T> PopAll()
    {
        vector<T> ret;

        lock_guard<mutex> lock(_mutex);
        while (false == _queue.empty()) {
            T item = _queue.front();
            _queue.pop();
            ret.push_back(item);
        }

        return ret;
    }


    size_t Getsize() const {
        // lock_guard<std::mutex> lock(_mutex);
        return _queue.size(); }
    bool Empty() { return _queue.empty(); }


private:
    queue<T> _queue;
    mutex _mutex;
    
};

