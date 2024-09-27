#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <stats/statsSystemNode.hpp>

// Bits that need to be passed around
struct ResultContext
{
    Red::TweakDB* m_tweakDB;
    save::StatsSystemNode* m_statsSystem;
};