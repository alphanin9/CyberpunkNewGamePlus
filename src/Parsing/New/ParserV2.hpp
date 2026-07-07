#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/StringView.hpp>

#include <Shared/Raw/FileSystem/FileSystem.hpp>
#include <Shared/Raw/Save/Save.hpp>

#include <RED4ext/Scripting/Natives/Generated/save/MetadataContainer.hpp>

#include <parsing/New/Nodes/Inventory.hpp>
#include <parsing/New/Nodes/PersistencySystem.hpp>
#include <parsing/New/Nodes/ScriptableSystemsContainer.hpp>
#include <parsing/New/Nodes/StatsSystem.hpp>

namespace parser
{
struct LoadStreamContainer
{
    Red::save::Metadata m_metadata{};

    shared::raw::Filesystem::BufferedRedFileStream m_fileStream{};
    shared::raw::Save::Stream::LoadStream m_loadStream{};

    bool Setup(Red::StringView& aSaveName) noexcept;
};

// Parser using the native game save stream.
//
// Replacement for parser::Parser (src/Parsing/FileReader.cpp), which is a WolvenKit-derived
// re-implementation of the file format. Instead of decompressing the whole save and rebuilding
// the node tree by hand, this drives the game's own LoadStream + SaveNodeAccessor: each node
// seeks itself by name and deserializes through native reads (ReadPackage / ReadBuffer).
//
// Migration tracked in issue #4. Consumer rewiring (NGPlusProgressionData / LoadSaveData) and the
// remaining nodes (persistency/vehicle garage, wardrobe) are follow-up phases.
class ParserV2
{
    LoadStreamContainer m_container{};

    node::ScriptableSystemsContainerNode m_scriptableSystems{};
    node::StatsSystemNode m_statsSystem{};
    node::InventoryNode m_inventory{};
    node::PersistencySystemNode m_persistency{};

    bool m_isValid{};

public:
    bool ParseSavegame(Red::StringView aSaveName) noexcept;

    node::ScriptableSystemsContainerNode& GetScriptableSystems() noexcept
    {
        return m_scriptableSystems;
    }

    node::StatsSystemNode& GetStatsSystem() noexcept
    {
        return m_statsSystem;
    }

    node::InventoryNode& GetInventory() noexcept
    {
        return m_inventory;
    }

    node::PersistencySystemNode& GetPersistency() noexcept
    {
        return m_persistency;
    }

    explicit operator bool() const noexcept
    {
        return m_isValid;
    }
};
} // namespace parser
