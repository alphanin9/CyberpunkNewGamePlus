#include "statsSystemNode.hpp"

#include "../../../../context/context.hpp"

#include <RED4ext/Package.hpp>

namespace cyberpunk
{
void StatsSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    // Stats system isn't a special case, I think...
    // It either is, or my reader is broken
    // DISABLED FOR NOW
    const auto packageSize = static_cast<std::uint32_t>(aCursor.readInt());

    auto subCursor = aCursor.CreateSubCursor(packageSize);

    Red::ObjectPackageReader packageHeaderReader{subCursor.GetCurrentPtr(), packageSize};
    Red::ObjectPackageHeader packageHeader{};

    packageHeaderReader.ReadHeader(packageHeader);

    Red::ObjectPackageExtractor packageExtractor{packageHeader};

    packageExtractor.ExtractSync();

    for (const auto& statsObject : packageExtractor.results)
    {
        if (!statsObject)
        {
            continue;
        }

        const auto objType = statsObject->GetType();

        if (!objType)
        {
            continue;
        }

        PluginContext::Spew(std::format("StatsSystemNode::ReadData, result obj {}...", objType->GetName().ToString()));
    }
}
}