// Dumb project structure strikes again...
#include "../../../../context/context.hpp"

#include "statsSystemNode.hpp"

#include <RED4ext/Package.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>

#include "../../package/packageReader.hpp"

namespace cyberpunk
{
void StatsSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    // DISABLED FOR NOW, IT TURNS OUT IT NEEDS A COMPLETE REFACTOR OF PACKAGE PARSER (INTEGRATING GENERAL PACKAGE READING CODE WITH PARSING, DUE TO HANDLE FUCKERY)
    // It's also useless
    const auto packageSize = static_cast<std::uint32_t>(aCursor.readInt());

    auto subCursor = aCursor.CreateSubCursor(packageSize);

    package::Package package(subCursor);

    package.ReadPackage();

    auto handlePtr = package.GetChunkByTypeName("gameStatsStateMapStructure");

    if (!handlePtr)
    {
        PluginContext::Spew("Handle ptr missing!");
        return;
    }

    auto asStatsStateMap = reinterpret_cast<Red::game::StatsStateMapStructure*>(handlePtr->GetPtr());
    std::size_t playerKey = std::numeric_limits<std::size_t>::max();

    for (std::size_t i = 0; i < asStatsStateMap->keys.size; i++)
    {
        const auto& elem = asStatsStateMap->keys[i];

        if (elem.idType == Red::gameStatIDType::EntityID && elem.entityHash == 1)
        {
            playerKey = i;
            break;
        }
    }

    if (playerKey == std::numeric_limits<std::size_t>::max())
    {
        return;
    }

    auto& statsObj = asStatsStateMap->values[playerKey];

    auto statCount = statsObj.statModifiers.size;

    PluginContext::Spew(std::format("Stat modifier count: {}", statCount));
}
}