// Dumb project structure strikes again...
#include <Context/Context.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/StatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CombinedStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CurveStatModifierData_Deprecated.hpp>

#include "StatsSystemNode.hpp"
#include <Parsing/Definitions/Package/PackageReader.hpp>
#include <Parsing/Definitions/RTTIHelpers/RTTIValueReaderNative.hpp>

#include <iterator>

using namespace Red;

namespace modsave
{
Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetStatModifiersInternal(Red::DataBuffer* aBuffer)
{
    static const auto statTypes = GetEnum<game::data::StatType>();

    // TODO: ugly, needs to accept void* in ctor
    FileCursor bufCursor(reinterpret_cast<std::byte*>(aBuffer->buffer.data), aBuffer->buffer.size);

    DynArray<Handle<game::StatModifierData_Deprecated>> bufferData{};

    while (bufCursor.getRemaining())
    {
        const auto className = bufCursor.OptimizedReadLengthPrefixedCName();

        const auto statName = bufCursor.ReadCNameHash();
        const auto modifierType = bufCursor.readValue<game::StatModifierType>();

        std::int64_t statEnumValue{};

        package::Package::ResolveEnumValue(statTypes, statName, statEnumValue);

        const auto statType = static_cast<game::data::StatType>(statEnumValue);

        switch (className.hash)
        {
        case CName("gameConstantStatModifier").hash:
        {
            auto data = MakeHandle<game::ConstantStatModifierData_Deprecated>();

            data->statType = statType;
            data->modifierType = modifierType;
            data->value = bufCursor.readFloat();

            bufferData.PushBack(std::move(data));
            break;
        }
        case CName("gameCombinedStatModifier").hash:
        {
            const auto refStatName = bufCursor.ReadCNameHash();

            std::int64_t refStatEnumValue{};

            package::Package::ResolveEnumValue(statTypes, refStatName, refStatEnumValue);

            const auto refStatType = static_cast<game::data::StatType>(refStatEnumValue);

            const auto operation = bufCursor.readValue<game::CombinedStatOperation>();
            const auto relation = bufCursor.readValue<game::StatObjectsRelation>();
            const auto value = bufCursor.readFloat();

            auto data = MakeHandle<game::CombinedStatModifierData_Deprecated>();

            data->statType = statType;
            data->modifierType = modifierType;
            data->refStatType = refStatType;
            data->operation = operation;
            data->refObject = relation;
            data->value = value;

            bufferData.PushBack(std::move(data));
            break;
        }
        case CName("gameCurveStatModifier").hash:
        {
            CName curveName = bufCursor.OptimizedReadLengthPrefixedANSI().c_str();
            CName colName = bufCursor.OptimizedReadLengthPrefixedANSI().c_str();

            const auto curveStatName = bufCursor.ReadCNameHash();

            std::int64_t curveStatEnumValue{};

            package::Package::ResolveEnumValue(statTypes, curveStatName, curveStatEnumValue);

            const auto curveStatType = static_cast<game::data::StatType>(curveStatEnumValue);

            auto data = MakeHandle<game::CurveStatModifierData_Deprecated>();

            data->statType = statType;
            data->modifierType = modifierType;
            data->curveName = curveName;
            data->columnName = colName;
            data->curveStat = curveStatType;

            bufferData.PushBack(std::move(data));
            
            bufCursor.readUInt();

            break;
        }
        default:
            break;
        }
    }

    return bufferData;
}

void StatsSystemNode::ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept
{
    const auto packageSize = static_cast<std::uint32_t>(aCursor.readInt());

    // Assuming we use game stream instead of having whole save unpacked, we need to allocate every time we create subcursor

    m_package.Init(aCursor.CreateSubCursor(packageSize));
    m_package.ReadPackage();

    m_package.m_useRootClassOptimization = true; // Use MT for root class...

    auto handlePtr = m_package.GetChunkByTypeName("gameStatsStateMapStructure");

    if (!handlePtr)
    {
        return;
    }

    // Ugly, but inheritance is fucked on this struct...
    auto asStatsStateMap = reinterpret_cast<game::StatsStateMapStructure*>(handlePtr->GetPtr());

    // Patch 2.5 (save version >= 0x114) empties keys/values and stores per-object stats in
    // shared pools instead; the game's own StatsSystem::OnGameLoad rebuilds SavedStatsData from
    // those, and so do we. Older saves keep the parallel arrays path below.
    if (aNode.saveVersion >= m_optimizedStatsFormatSaveVersion)
    {
        ReadOptimizedStatsData(*asStatsStateMap);
        return;
    }

    // Will this help?
    m_statsMap.reserve(asStatsStateMap->keys.size());
    for (auto i = 0u; i < asStatsStateMap->keys.size(); i++)
    {
        m_statsMap.try_emplace(asStatsStateMap->keys[i].entityHash, &asStatsStateMap->values[i]); 
    }
}

void StatsSystemNode::ReadOptimizedStatsData(Red::game::StatsStateMapStructure& aStatsStateMap) noexcept
{
    const auto& sharedData = aStatsStateMap.sharedDataBuffer.buffer;

    m_statsMap.reserve(aStatsStateMap.valuesOptimized.size());
    m_rebuiltStatsData.reserve(aStatsStateMap.valuesOptimized.size());

    for (auto& optimizedEntry : aStatsStateMap.valuesOptimized)
    {
        auto handle = Red::MakeScriptedHandle(Red::CName{Red::game::SavedStatsData::NAME});

        if (!handle)
        {
            PluginContext::Error("StatsSystemNode: failed to allocate a gameSavedStatsData instance!");
            continue;
        }

        auto* savedData = reinterpret_cast<Red::game::SavedStatsData*>(handle.GetPtr());

        Red::DataBuffer* perKindBuffers[] = {&savedData->modifiersBuffer, &savedData->forcedModifiersBuffer,
                                             &savedData->savedModifierGroupStatTypesBuffer};

        // A slice picks a window of sharedDataBuffer. `combined` packs the window size in its
        // low 30 bits and which of the three DataBuffers it belongs to in the top two bits
        // (the game's converter: size = combined & 0x3FFFFFFF, kind = combined >> 30).
        for (auto sliceIndex : optimizedEntry.bufferSliceIndices)
        {
            if (sliceIndex >= aStatsStateMap.sharedBufferSlices.size())
            {
                continue;
            }

            const auto& slice = aStatsStateMap.sharedBufferSlices[sliceIndex];

            const auto bufferKind = slice.combined >> 30;
            const auto bufferSize = slice.combined & 0x3FFFFFFF;

            if (bufferKind >= std::size(perKindBuffers) || bufferSize == 0 || !sharedData.data ||
                slice.offset > sharedData.size || bufferSize > sharedData.size - slice.offset)
            {
                continue;
            }

            // The buffers stay views into the parser-owned node data, same as every other
            // DataBuffer the package reader hands out.
            redRTTI::native::SetDataBufferView(
                *perKindBuffers[bufferKind], static_cast<std::byte*>(sharedData.data) + slice.offset, bufferSize);
        }

        for (auto statIndex : optimizedEntry.inactiveStatsIndices)
        {
            if (statIndex >= aStatsStateMap.sharedInactiveStats.size())
            {
                continue;
            }

            savedData->inactiveStats.PushBack(aStatsStateMap.sharedInactiveStats[statIndex]);
        }

        if (optimizedEntry.recordIDIndex < aStatsStateMap.sharedRecordId.size())
        {
            savedData->recordID = aStatsStateMap.sharedRecordId[optimizedEntry.recordIDIndex];
        }

        savedData->seed = optimizedEntry.seed;

        m_statsMap.try_emplace(optimizedEntry.entityHash, savedData);
        m_rebuiltStatsData.push_back(std::move(handle));
    }
}

std::uint64_t StatsSystemNode::GetEntityHashFromItemId(const ItemID& aId) noexcept
{
    // Shamelessly stolen from WKit
    const auto c = 0xC6A4A7935BD1E995;

    std::uint64_t tmp{};

    if (aId.uniqueCounter == 0)
    {
        tmp = aId.rngSeed * c;
        tmp = (tmp >> 0x2F) ^ tmp;
    }
    else
    {
        tmp = aId.uniqueCounter * c;
        tmp = ((((tmp >> 0x2F) ^ tmp) * c) ^ aId.rngSeed) * 0x35A98F4D286A90B9;
        tmp = (tmp >> 0x2F) ^ tmp;
    }

    return ((tmp * c) ^ aId.tdbid) * c;
}

void StatsSystemNode::DumpStatModifiersToConsole(
    const DynArray<Handle<game::StatModifierData_Deprecated>>& aStatModifierList) noexcept
{
    using namespace Red;
    static const auto statTypes = GetEnum<game::data::StatType>();
    static const auto statModifierTypes = GetEnum<game::StatModifierType>();

    for (auto& stat : aStatModifierList)
    {
        const auto strPrefix = std::format(
            "\tStat type: {}\n\tModifier type: {}",
            package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(stat->statType)),
            package::Package::GetEnumString(statModifierTypes, static_cast<std::int64_t>(stat->modifierType)));

        std::string strSuffix{};

        if (const auto& asConstant = Cast<game::ConstantStatModifierData_Deprecated>(stat))
        {
            strSuffix = std::move(std::format("\tConstant\n\tValue: {}", asConstant->value));
        }
        else if (const auto& asCombined = Cast<game::CombinedStatModifierData_Deprecated>(stat))
        {
            static const auto objRelations = GetEnum<game::StatObjectsRelation>();
            static const auto combinedOperations = GetEnum<game::CombinedStatOperation>();

            strSuffix = std::move(std::format(
                "\tCombined\n\tRef stat type: {}\n\tRef object: {}\n\tOperation: {}\n\tValue: {}",
                package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(asCombined->refStatType)),
                package::Package::GetEnumString(objRelations, static_cast<std::int64_t>(asCombined->refObject)),
                package::Package::GetEnumString(combinedOperations, static_cast<std::int64_t>(asCombined->operation)),
                asCombined->value));
        }
        else if (const auto& asCurve = Cast<game::CurveStatModifierData_Deprecated>(stat))
        {
            strSuffix = std::move(
                std::format("\tCurve\n\tCurve name: {}\n\tColumn name: {}\n\tCurve stat: {}", asCurve->curveName.ToString(),
                            asCurve->columnName.ToString(),
                            package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(asCurve->curveStat))));
        }

        PluginContext::Spew(std::format("\n{}\n{}", strPrefix, strSuffix));
        PluginContext::Spew("");
    }
}

DynArray<Handle<game::StatModifierData_Deprecated>> StatsSystemNode::GetStatModifiers(std::uint64_t aEntityHash) noexcept
{
    if (!m_statsMap.contains(aEntityHash))
    {
        return {};
    }
    
    auto savedStatsDataPtr = m_statsMap[aEntityHash];

    return GetStatModifiersInternal(&savedStatsDataPtr->modifiersBuffer);
}

DynArray<Handle<game::StatModifierData_Deprecated>> StatsSystemNode::GetForcedModifiers(
    std::uint64_t aEntityHash) noexcept
{
    if (!m_statsMap.contains(aEntityHash))
    {
        return {};
    }

    auto savedStatsDataPtr = m_statsMap[aEntityHash];
    
    return GetStatModifiersInternal(&savedStatsDataPtr->forcedModifiersBuffer);
}

DynArray<game::data::StatType> StatsSystemNode::GetDisabledModifiers(std::uint64_t aEntityHash) noexcept 
{
    if (!m_statsMap.contains(aEntityHash))
    {
        return {};
    }

    return m_statsMap[aEntityHash]->inactiveStats;
}
}