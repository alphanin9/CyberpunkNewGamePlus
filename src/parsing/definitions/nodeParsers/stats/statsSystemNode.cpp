// Dumb project structure strikes again...
#include "../../../../context/context.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/StatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CombinedStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/CurveStatModifierData_Deprecated.hpp>

#include "statsSystemNode.hpp"
#include "../../package/packageReader.hpp"

namespace cyberpunk
{
Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> GetStatModifiersInternal(Red::DataBuffer* aBuffer)
{
    using namespace Red;

    static const auto statTypes = GetEnum<game::data::StatType>();

    // TODO: ugly, needs to accept void* in ctor
    FileCursor bufCursor(reinterpret_cast<std::byte*>(aBuffer->buffer.data), aBuffer->buffer.size);

    DynArray<Handle<game::StatModifierData_Deprecated>> bufferData{};

    while (bufCursor.getRemaining())
    {
        CName className = bufCursor.ReadLengthPrefixedANSI().c_str();

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
            CName curveName = bufCursor.ReadLengthPrefixedANSI().c_str();
            CName colName = bufCursor.ReadLengthPrefixedANSI().c_str();

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

    m_package.Init(aCursor.CreateSubCursor(packageSize));
    m_package.ReadPackage();
    
    constexpr auto shouldDumpPlayerStatModifiers = false;

    if (shouldDumpPlayerStatModifiers)
    {
        auto handlePtr = m_package.GetChunkByTypeName("gameStatsStateMapStructure");

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

        auto statBuffer = GetStatModifiersInternal(&statsObj.modifiersBuffer);
        auto forcedBuffer = GetStatModifiersInternal(&statsObj.forcedModifiersBuffer);
        auto modifierGroupStatTypesBuffer = GetStatModifiersInternal(&statsObj.savedModifierGroupStatTypesBuffer);

        using namespace Red;
        static const auto statTypes = GetEnum<game::data::StatType>();
        static const auto statModifierTypes = GetEnum<game::StatModifierType>();

        const auto dumpStatBuffer = [](DynArray<Handle<game::StatModifierData_Deprecated>>& aList)
        {
            for (auto& stat : aList)
            {
                const auto strPrefix = std::format(
                    "Stat type: {}\nModifier type: {}",
                    package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(stat->statType)),
                    package::Package::GetEnumString(statModifierTypes, static_cast<std::int64_t>(stat->modifierType)));

                std::string strSuffix{};

                if (const auto& asConstant = Cast<game::ConstantStatModifierData_Deprecated>(stat))
                {
                    strSuffix = std::move(std::format("Constant\nValue: {}", asConstant->value));
                }
                else if (const auto& asCombined = Cast<game::CombinedStatModifierData_Deprecated>(stat))
                {
                    static const auto objRelations = GetEnum<game::StatObjectsRelation>();
                    static const auto combinedOperations = GetEnum<game::CombinedStatOperation>();

                    strSuffix = std::move(std::format(
                        "Combined\nRef stat type: {}\nRef object: {}\nOperation: {}\nValue: {}",
                        package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(asCombined->refStatType)),
                        package::Package::GetEnumString(objRelations, static_cast<std::int64_t>(asCombined->refObject)),
                        package::Package::GetEnumString(combinedOperations,
                                                        static_cast<std::int64_t>(asCombined->operation)),
                        asCombined->value));
                }
                else if (const auto& asCurve = Cast<game::CurveStatModifierData_Deprecated>(stat))
                {
                    strSuffix = std::move(std::format(
                        "Curve\nCurve name: {}\nColumn name: {}\nCurve stat: {}", asCurve->curveName.ToString(),
                        asCurve->columnName.ToString(),
                        package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(asCurve->curveStat))));
                }

                PluginContext::Spew(std::format("{}\n{}", strPrefix, strSuffix));
                PluginContext::Spew("");
            }
        };

        PluginContext::Spew("Stats:");
        dumpStatBuffer(statBuffer);
    }
}

std::uint64_t StatsSystemNode::GetEntityHashFromItemId(const Red::ItemID& aId) noexcept
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
    const Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>>& aStatModifierList) noexcept
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

Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> StatsSystemNode::GetStatModifiers(std::uint64_t aEntityHash) noexcept
{
    auto handlePtr = m_package.GetChunkByTypeName("gameStatsStateMapStructure");

    if (!handlePtr)
    {
        return {};
    }

    auto statsMap = reinterpret_cast<Red::game::StatsStateMapStructure*>(handlePtr->GetPtr());

    std::size_t entityIndex{};

    for (; entityIndex < statsMap->keys.size; entityIndex++)
    {
        if (statsMap->keys[entityIndex].entityHash == aEntityHash)
        {
            break;
        }
    }

    if (entityIndex == statsMap->keys.size)
    {
        return {};
    }

    auto valueBuffer = &statsMap->values[entityIndex].modifiersBuffer;

    constexpr auto shouldDumpAllStatsForID = true;

    auto modifiers = GetStatModifiersInternal(valueBuffer);
    if constexpr (shouldDumpAllStatsForID)
    {
        PluginContext::Spew("Modifiers: ");
        DumpStatModifiersToConsole(modifiers);
    }

    return modifiers;
}
}