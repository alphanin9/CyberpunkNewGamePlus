#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <Parsing/Definitions/NodeParsers/Stats/StatsSystemNode.hpp>

// Bits that need to be passed around
struct ResultContext
{
    Red::TweakDB* m_tweakDB;
    modsave::StatsSystemNode* m_statsSystem;
};