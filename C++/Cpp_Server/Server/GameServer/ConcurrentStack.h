#pragma once
#include <mutex>

template<typename T>
class LockStack
{
public:
	LockStack() {} //기본 생성자

	LockStack(const LockStack&) = delete; //복사 생성자 삭제
	LockStack& operator=(const LockStack&) = delete; //복사 대입 연산자 삭제

	void Push(T value) 
	{
		std::lock_guard<std::mutex> lock(_mutex); // mutex 잠금
		_stack.push(std::move(value));
		_condition.notify_one(); // 스택에 요소가 추가되었음을 알림
	}

	bool TryPop(T& value) {
		std::lock_guard<std::mutex> lock(_mutex); // mutex 잠금
		if (_stack.empty()) {
			return false; // 스택이 비어있으면 false 반환
		}
		value = std::move(_stack.top()); // 스택의 top 요소를 value로 이동
		_stack.pop(); // top 요소 제거
		return true; // 성공적으로 pop했음을 나타냄
	}

	void WaitPop(T& value) {
		std::unique_lock<std::mutex> lock(_mutex); // mutex 잠금
		_condition.wait(lock, [this] { return _stack.empty()==false; }); // 스택이 비어있지 않을 때까지 대기
		value = std::move(_stack.top()); // 스택의 top 요소를 value로 이동
		_stack.pop(); // top 요소 제거
	}
private:
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condition; // 조건 변수는 필요에 따라 추가할 수 있습니다.
};