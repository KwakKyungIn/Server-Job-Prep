#pragma once
#include <mutex>

template <typename T>
class LockQueue
{
public:
	LockQueue() {}

	LockQueue(const LockQueue&) = delete;
	LockQueue& operator=(const LockQueue&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_queue.push(std::move(value));
		_condition.notify_one();
	}

	bool TryPop(T& value) {
		std::lock_guard<std::mutex> lock(_mutex); // mutex 잠금
		if (_queue.empty()) {
			return false; // 스택이 비어있으면 false 반환
		}
		value = std::move(_queue.front()); // 스택의 top 요소를 value로 이동
		_queue.pop(); // top 요소 제거
		return true; // 성공적으로 pop했음을 나타냄
	}

	void WaitPop(T& value) {
		std::unique_lock<std::mutex> lock(_mutex); // mutex 잠금
		_condition.wait(lock, [this] { return _queue.empty() == false; }); // 스택이 비어있지 않을 때까지 대기
		value = std::move(_queue.front()); // 스택의 top 요소를 value로 이동
		_queue.pop(); // top 요소 제거
	}
private:
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condition;
};