#pragma once
#include <functional>
#include <memory>

using CallbackType = std::function<void()>;

class Job
{
public:
	Job(CallbackType&& callback) : _callback(std::move(callback)) {}

	// [Critical Fix] 템플릿 인자 분리 (Args vs RealArgs)
	// - Args: 함수 포인터에 정의된 인자 타입 (예: C_MOVE)
	// - RealArgs: 실제로 넘겨받은 인자 타입 (예: C_MOVE&)
	// 이렇게 분리하면 인자가 참조(&)로 들어와도, 함수가 값(Value)을 원하면 알아서 매칭된다.
	template<typename T, typename Ret, typename... Args, typename... RealArgs>
	Job(std::shared_ptr<T> owner, Ret(T::* memFunc)(Args...), RealArgs&&... args)
	{
		// [Capture By Value]
		// args... 를 캡처할 때 복사가 일어난다.
		// 즉, 패킷 데이터(pkt)가 Job 내부에 복사되어 안전하게 저장된다.
		_callback = [owner, memFunc, args...]()
			{
				(owner.get()->*memFunc)(args...);
			};
	}

	void Execute()
	{
		if (_callback)
			_callback();
	}

private:
	CallbackType _callback;
};