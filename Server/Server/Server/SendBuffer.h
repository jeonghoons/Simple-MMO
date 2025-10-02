#pragma once
#include "ConcurrentQueue.h"

class SendBuffer
{
public:
	SendBuffer(int bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.data(); }
	int WritePos() { return _writePos; }
	int Capacity() { return static_cast<int>(_buffer.size()); }

	void CopyData(void* data, int len);

private:
	vector<BYTE> _buffer;
	int		_writePos = 0;
};

class SendBufferQueue
{
public:
	void Push(shared_ptr<SendBuffer> sendBuffer)
	{
		_queue.Push(sendBuffer);
	}

	shared_ptr<SendBuffer> TryPop()
	{
		return _queue.TryPop();
	}



	bool Empty() { return _queue.Empty(); }

	
public:
	ConcurrentQueue<shared_ptr<SendBuffer>> _queue;
};

