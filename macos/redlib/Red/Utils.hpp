#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <RED4ext/JobQueue.hpp>

namespace Red
{
template<typename T, typename U = T>
inline WeakHandle<U>& AsWeakHandle(T* aInstance)
{
    return *reinterpret_cast<WeakHandle<U>*>(&aInstance->ref);
}

template<typename T, typename U = T>
inline Handle<U> AsHandle(T* aInstance)
{
    return AsWeakHandle(aInstance);
}

template<typename T, typename U = T>
inline WeakHandle<U> ToWeakHandle(T* aInstance)
{
    if (!aInstance)
        return {};

    return AsWeakHandle(aInstance);
}

template<typename T>
inline Handle<T> ToHandle(T* aInstance)
{
    if (!aInstance)
        return {};

    if (aInstance->ref.instance)
    {
        return reinterpret_cast<WeakHandle<T>*>(&aInstance->ref)->Lock();
    }
    else
    {
        return Handle<T>(aInstance);
    }
}

template<typename T, typename U>
inline Handle<T> ToHandle(U* aInstance)
{
    if (!aInstance)
        return {};

    auto instance = reinterpret_cast<ISerializable*>(aInstance);
    if (instance->ref.instance)
    {
        return reinterpret_cast<WeakHandle<T>*>(&instance->ref)->Lock();
    }
    else
    {
        return Handle<T>(reinterpret_cast<T*>(instance));
    }
}

template<typename T = ISerializable>
inline Handle<T> MakeScriptedHandle(const CClass* aType)
{
    if (!aType)
    {
        return {};
    }

    return Handle<T>(reinterpret_cast<T*>(aType->CreateInstance(true)));
}

template<typename T = ISerializable>
inline Handle<T> MakeScriptedHandle(CName aTypeName)
{
    auto classType = GetClass(aTypeName);
    if (!classType)
    {
        return {};
    }

    return Handle<T>(reinterpret_cast<T*>(classType->CreateInstance(true)));
}

namespace Detail
{
struct WaitingContext
{
    std::mutex mutex;
    std::condition_variable cv;
    bool finished{false};
};
}

template<typename W>
inline void WaitForQueue(JobQueue& aQueue, const W& aTimeout)
{
    auto context = std::make_shared<Detail::WaitingContext>();

    aQueue.Dispatch([context]() {
        context->finished = true;
        context->cv.notify_all();
    });

    std::unique_lock lock(context->mutex);
    context->cv.wait_for(lock, aTimeout, [context]() { return context->finished; });
}
}
