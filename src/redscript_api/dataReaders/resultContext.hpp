#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <parsing/definitions/nodeParsers/stats/statsSystemNode.hpp>

// Bits that need to be passed around
struct ResultContext
{
    Red::TweakDB* m_tweakDB;
    modsave::StatsSystemNode* m_statsSystem;
};