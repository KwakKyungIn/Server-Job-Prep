#pragma once
#include "Types.h"

#include <functional>
#include <memory>

using CallbackType = std::function<void()>;

class Job
{
public:
    Job(CallbackType&& callback)
        : _callback(std::move(callback))
    {
    }

    template<typename T, typename Ret, typename... Args, typename... RealArgs>
    Job(std::shared_ptr<T> owner, Ret(T::* memFunc)(Args...), RealArgs&&... args)
    {
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

    void SetEnqueueTimestampUs(uint64 enqueueTimestampUs)
    {
        _enqueueTimestampUs = enqueueTimestampUs;
    }

    uint64 GetEnqueueTimestampUs() const
    {
        return _enqueueTimestampUs;
    }

private:
    CallbackType _callback;
    uint64 _enqueueTimestampUs = 0;
};
