#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

namespace util
{
namespace job
{
template<typename FnType>
requires Red::Detail::IsClosure<FnType, void, Red::JobGroup&> || Red::Detail::IsClosure<FnType, void>
Red::JobHandle MakeJob(FnType&& aFn)
{
    Red::JobQueue queue{};

    queue.Dispatch(std::move(aFn));

    return std::move(queue.Capture());
}
}
}