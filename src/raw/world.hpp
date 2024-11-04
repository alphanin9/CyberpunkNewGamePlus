#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/WorldPosition.hpp>

#include <detail/hashes.hpp>
#include <util/core.hpp>

namespace raw::World
{
constexpr auto IsInInterior =
    util::RawFunc<detail::Hashes::World_IsInInterior, int* (*)(void*, int*, Red::WorldPosition&, bool)>();
};