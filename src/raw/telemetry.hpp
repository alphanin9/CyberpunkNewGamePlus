#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <detail/hashes.hpp>
#include <util/core.hpp>

namespace raw::Telemetry
{
constexpr auto LoadFactMap =
    util::RawFunc<detail::Hashes::Telemetry_LoadUsedFactsForImportantFactsList, void* (*)(void*)>();
} // namespace raw::Telemetry