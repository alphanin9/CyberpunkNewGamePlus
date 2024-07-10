#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../context/context.hpp"
#include "../filesystem/fs_util.hpp"
#include "../parsing/fileReader.hpp"

#include "../parsing/definitions/nodeParsers/inventory/inventoryNode.hpp"
#include "../parsing/definitions/nodeParsers/parserHelper.hpp"
#include "../parsing/definitions/nodeParsers/persistency/persistencySystemNode.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/scriptableContainerNode.hpp"
#include "../parsing/definitions/nodeParsers/stats/statsSystemNode.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>

#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/craftBook.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/equipmentSystem.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/playerDevelopmentData.hpp"

#include "../util/threads.hpp"

#include <chrono>

#include <simdjson.h>

// NOT A GOOD IDEA TO HAVE IN A HEADER
// But who cares?

using scriptable::native::CraftBook::CraftBook;
using scriptable::native::EquipmentSystem::EquipmentSystemPlayerData;
using scriptable::native::PlayerDevelopment::PlayerDevelopmentData;

// Enums
using scriptable::native::PlayerDevelopment::AttributeDataType;
using scriptable::native::PlayerDevelopment::EspionageMilestonePerks;

// Wrappers
using scriptable::native::CraftBook::ItemRecipe;
using scriptable::native::PlayerDevelopment::SAttribute;
using scriptable::native::PlayerDevelopment::SAttributeData;
using scriptable::native::PlayerDevelopment::SDevelopmentPoints;
using scriptable::native::PlayerDevelopment::SNewPerk;
using scriptable::native::PlayerDevelopment::SProficiency;

namespace redscript
{
struct RedItemData
{
    Red::ItemID m_itemId{};
    int32_t m_itemQuantity{};

    // HACK: make CW upgrades carry over...
    // Makes things a bit heavier, but w/e
    Red::DynArray<Red::ItemID> m_attachments;
    Red::DynArray<Red::Handle<Red::game::StatModifierData_Deprecated>> m_statModifiers;

    RTTI_IMPL_TYPEINFO(RedItemData);
    RTTI_IMPL_ALLOCATOR();
};

struct RedCraftInfo
{
    Red::TweakDBID m_targetItem;
    int m_amount;
    Red::DynArray<Red::ItemID> m_hideOnItemsAdded;

    RTTI_IMPL_TYPEINFO(RedCraftInfo);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace redscript

RTTI_DEFINE_CLASS(redscript::RedItemData, {
    RTTI_PROPERTY(m_itemId);
    RTTI_PROPERTY(m_itemQuantity);
    RTTI_PROPERTY(m_attachments);
    RTTI_PROPERTY(m_statModifiers);
});

RTTI_DEFINE_CLASS(redscript::RedCraftInfo, {
    RTTI_PROPERTY(m_targetItem);
    RTTI_PROPERTY(m_amount);
    RTTI_PROPERTY(m_hideOnItemsAdded);
});

namespace redscript
{
// NOTE: needs ISerializable inheritance for handle stuff - later...
struct PlayerSaveData
{
    bool m_isValid{};
    // We do not save actual perks - what if the player wants a respec?
    // Excessive cyberware can be fixed by not giving it all in one go
    // and debug cyberware capacity shards
    int32_t m_playerPerkPoints{};
    // Not sure how progression builds handle this, but you get Relic perks even without doing anything there
    // Fact n"ep1_tree_unlocked"
    int32_t m_playerRelicPoints{};
    int32_t m_playerAttributePoints{};

    // Maybe put attributes in the character creation screen?
    // Would be cool
    int32_t m_playerBodyAttribute{};
    int32_t m_playerReflexAttribute{};
    int32_t m_playerTechAttribute{};
    int32_t m_playerIntelligenceAttribute{};
    int32_t m_playerCoolAttribute{};

    int32_t m_playerBodySkillLevel{};
    int32_t m_playerReflexSkillLevel{};
    int32_t m_playerTechSkillLevel{};
    int32_t m_playerIntelligenceSkillLevel{};
    int32_t m_playerCoolSkillLevel{};

    // Cap this to 50 on the scripting side, so the player still has something to level towards
    int32_t m_playerLevel{};

    // Dumb and can just be set to 50 LMAO
    int32_t m_playerStreetCred{};

    // Special treatment :D
    int32_t m_playerMoney{};

    // No other inventories matter too much
    // Johnny and Kurtz get their own shit anyway
    // Filter stash items from useless shit like vehicle weapons
    // in native or on the Redscript side?
    Red::DynArray<RedItemData> m_playerItems{};
    Red::DynArray<RedItemData> m_playerStashItems{};

    Red::DynArray<RedCraftInfo> m_knownRecipeTargetItems{};

    // Most distinctive cyberware needs to be equipped
    // by the scripting side to make
    // the player feel at home from the get-go
    // Clothing does not, as the endpoint in Q101
    // will still end up with the player naked

    // Cyberware-EX will mess this up, but do we care?
    Red::ItemID m_playerEquippedOperatingSystem{};
    Red::ItemID m_playerEquippedKiroshis{};
    Red::ItemID m_playerEquippedLegCyberware{};
    Red::ItemID m_playerEquippedArmCyberware{};

    Red::DynArray<Red::ItemID> m_playerEquippedCardiacSystemCW;

    // Filter this from quest vehicles?
    // (Quadra, Demiurge, ETC)
    // Nah, shit idea, maybe for the Demiurge
    // Seeing as acquiring it is pretty cool
    Red::DynArray<Red::TweakDBID> m_playerVehicleGarage{};

    bool m_addedPerkPointsFromOverflow;

    // NOTE: these do not account for perks like Edgerunner/whatever else... only shards
    // Whatever, we won't be overallocating CW cap anyway :P
    Red::DynArray<float> m_playerCyberwareCapacity{};
    Red::DynArray<float> m_playerCarryCapacity{};

    // Later maybe do Equipment-EX transfer too?

    // In theory we don't need this
    RTTI_IMPL_TYPEINFO(PlayerSaveData);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace redscript

RTTI_DEFINE_CLASS(redscript::PlayerSaveData, {
    RTTI_PROPERTY(m_isValid);
    RTTI_PROPERTY(m_playerPerkPoints);
    RTTI_PROPERTY(m_playerRelicPoints);
    RTTI_PROPERTY(m_playerAttributePoints);
    RTTI_PROPERTY(m_playerBodyAttribute);
    RTTI_PROPERTY(m_playerReflexAttribute);
    RTTI_PROPERTY(m_playerTechAttribute);
    RTTI_PROPERTY(m_playerIntelligenceAttribute);
    RTTI_PROPERTY(m_playerCoolAttribute);
    RTTI_PROPERTY(m_playerBodySkillLevel);
    RTTI_PROPERTY(m_playerReflexSkillLevel);
    RTTI_PROPERTY(m_playerTechSkillLevel);
    RTTI_PROPERTY(m_playerIntelligenceSkillLevel);
    RTTI_PROPERTY(m_playerCoolSkillLevel);
    RTTI_PROPERTY(m_playerLevel);
    RTTI_PROPERTY(m_playerStreetCred);
    RTTI_PROPERTY(m_playerMoney);
    RTTI_PROPERTY(m_playerItems);
    RTTI_PROPERTY(m_playerStashItems);
    RTTI_PROPERTY(m_knownRecipeTargetItems);
    RTTI_PROPERTY(m_playerEquippedOperatingSystem);
    RTTI_PROPERTY(m_playerEquippedKiroshis);
    RTTI_PROPERTY(m_playerEquippedLegCyberware);
    RTTI_PROPERTY(m_playerEquippedArmCyberware);
    RTTI_PROPERTY(m_playerEquippedCardiacSystemCW);
    RTTI_PROPERTY(m_playerVehicleGarage);
    RTTI_PROPERTY(m_playerCyberwareCapacity);
    RTTI_PROPERTY(m_playerCarryCapacity);
});

namespace BlacklistedTDBIDs
{
/*
{
            Red::TweakDBID{"Items.MaskCW"}, Red::TweakDBID{"Items.MaskCWPlus"},
            Red::TweakDBID{"Items.MaskCWPlusPlus"}, Red::TweakDBID{"Items.w_melee_004__fists_a"},
            Red::TweakDBID{"Items.PersonalLink"}, Red::TweakDBID{"Items.personal_link"},
            Red::TweakDBID{"Items.PlayerMaTppHead"}, Red::TweakDBID{"Items.PlayerWaTppHead"},
            Red::TweakDBID{"Items.PlayerFppHead"}, Red::TweakDBID{"Items.HolsteredFists"},
            Red::TweakDBID{"Items.mq024_sandra_data_carrier"},
            // And Skippy leads to him talking in the starting cutscene
            Red::TweakDBID{"Items.mq007_skippy"}, Red::TweakDBID{"Items.mq007_skippy_post_quest"},
            Red::TweakDBID{"Items.Preset_Yukimura_Skippy"},
            Red::TweakDBID{"Items.Preset_Yukimura_Skippy_PostQuest"},
            Red::TweakDBID{"Items.q005_saburo_data_carrier"},
            Red::TweakDBID{"Items.q005_saburo_data_carrier_cracked"}, Red::TweakDBID{"Items.q003_chip"},
            Red::TweakDBID{"Items.q003_chip_cracked"}, Red::TweakDBID{"Items.q003_chip_cracked_funds"},
            Red::TweakDBID{"Items.Preset_Q001_Lexington"}, Red::TweakDBID{"Items.CyberdeckSplinter"},
            Red::TweakDBID{"Items.Preset_Lexington_Wilson"}, Red::TweakDBID{"Items.mq011_wilson_gun"}
}
*/
inline constexpr auto MaskCW = Red::TweakDBID("Items.MaskCW");
inline constexpr auto MaskCWPlus = Red::TweakDBID("Items.MaskCWPlus");
inline constexpr auto MaskCWPlusPlus = Red::TweakDBID("Items.MaskCWPlusPlus");
inline constexpr auto Fists = Red::TweakDBID("Items.w_melee_004__fists_a");
inline constexpr auto PersonalLink = Red::TweakDBID("Items.PersonalLink");

inline constexpr auto PersonalLink2 = Red::TweakDBID("Items.personal_link");
inline constexpr auto MaTppHead = Red::TweakDBID("Items.PlayerMaTppHead");
inline constexpr auto WaTppHead = Red::TweakDBID("Items.PlayerWaTppHead");
inline constexpr auto FppHead = Red::TweakDBID("Items.PlayerFppHead");
inline constexpr auto HolsteredFists = Red::TweakDBID("Items.HolsteredFists");

inline constexpr auto MQ024DataCarrier = Red::TweakDBID("Items.mq024_sandra_data_carrier");
inline constexpr auto Skippy = Red::TweakDBID("Items.mq007_skippy");
inline constexpr auto SkippyPostQuest = Red::TweakDBID("Items.mq007_skippy_post_quest");
inline constexpr auto PresetSkippy = Red::TweakDBID("Items.Preset_Yukimura_Skippy");
inline constexpr auto PresetSkippyPostQuest = Red::TweakDBID("Items.Preset_Yukimura_Skippy_PostQuest");

inline constexpr auto SaburoDataCarrier = Red::TweakDBID("Items.q005_saburo_data_carrier");
inline constexpr auto SaburoDataCarrierCracked = Red::TweakDBID("Items.q005_saburo_data_carrier_cracked");
inline constexpr auto Q003Chip = Red::TweakDBID("Items.q003_chip");
inline constexpr auto Q003ChipCracked = Red::TweakDBID("Items.q003_chip_cracked");
inline constexpr auto Q003ChipCrackedFunds = Red::TweakDBID("Items.q003_chip_cracked_funds");

inline constexpr auto CyberdeckSplinter = Red::TweakDBID("Items.CyberdeckSplinter");

inline bool IsForbidden(Red::TweakDBID aId)
{
    // Disgusting
    return aId == MaskCW || aId == MaskCWPlus || aId == MaskCWPlusPlus || aId == Fists || aId == PersonalLink ||
           aId == PersonalLink2 || aId == MaTppHead || aId == WaTppHead || aId == FppHead || aId == HolsteredFists ||
           aId == MQ024DataCarrier || aId == Skippy || aId == SkippyPostQuest || aId == PresetSkippy ||
           aId == PresetSkippyPostQuest || aId == SaburoDataCarrier || aId == SaburoDataCarrierCracked ||
           aId == Q003Chip || aId == Q003ChipCracked || aId == Q003ChipCrackedFunds || aId == CyberdeckSplinter;
}
}; // namespace BlacklistedTDBIDs

namespace redscript
{

enum class ENewGamePlusStartType
{
    StartFromQ001,
    StartFromQ101,
    StartFromQ001_NoEP1,
    StartFromQ101_NoEP1,
    Count,
    Invalid
};

class NewGamePlusSystem : public Red::IGameSystem
{
public:
    PlayerSaveData GetSaveData()
    {
        return m_saveData;
    }

    bool GetNewGamePlusState() const
    {
        return PluginContext::m_isNewGamePlusActive;
    }

    void SetNewGamePlusState(bool aNewState)
    {
        PluginContext::m_isNewGamePlusActive = aNewState;
    }

    void LoadExpansionIntoSave()
    {
        // We only have the one expansion... Thank God

        // TODO: move all hashes into a header?
        // Vtable index 62
        // Sig: 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 ED B8
        constexpr auto addQuestHash = 1617892594u;

        // Maybe check if EP1 is already running? Eh, the Redscript side already does...

        static const auto fnAddQuest = Red::UniversalRelocFunc<void(__fastcall*)(
            Red::quest::QuestsSystem * aQuestsSystem, Red::ResourcePath aPath)>(addQuestHash);

        fnAddQuest(Red::GetGameSystem<Red::quest::QuestsSystem>(), R"(ep1\quest\ep1.quest)");
    }

    void SetNewGamePlusGameDefinition(ENewGamePlusStartType aStartType)
    {
        constexpr auto q001Path = Red::ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001.gamedef");
        constexpr auto q101Path = Red::ResourcePath::HashSanitized("mod/quest/NewGamePlus.gamedef");

        // These ones do not have EP1 enabled by default, but will have it enabled via custom quest starting
        constexpr auto q001PathNoEP1 = Red::ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001_NoEP1.gamedef");
        constexpr auto q101PathNoEP1 = Red::ResourcePath::HashSanitized("mod/quest/NewGamePlus_NoEP1.gamedef");

        switch (aStartType)
        {
        case ENewGamePlusStartType::StartFromQ001:
            PluginContext::m_ngPlusGameDefinitionHash = q001Path;
            break;
        case ENewGamePlusStartType::StartFromQ101:
            PluginContext::m_ngPlusGameDefinitionHash = q101Path; // Should fix it being in base/ sometime
            break;
        case ENewGamePlusStartType::StartFromQ001_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = q001PathNoEP1;
            break;
        case ENewGamePlusStartType::StartFromQ101_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = q101PathNoEP1;
            break;
        default:
            break;
        }
    }

    bool HasPointOfNoReturnSave()
    {
        return files::HasValidPointOfNoReturnSave();
    }

    bool IsSaveValidForNewGamePlus(Red::ScriptRef<Red::CString>& aSaveName)
    {
        return files::IsValidForNewGamePlus(aSaveName->c_str());
    }

    Red::DynArray<int> ResolveNewGamePlusSaves(Red::ScriptRef<Red::DynArray<Red::CString>>& aSaves)
    {
        if (!aSaves)
        {
            return {};
        }

        static const auto basePath = files::GetCpSaveFolder();

        // We don't need sorting, the game already sorts the saves before passing them to Redscript
        std::unordered_set<std::uint64_t> playthroughIds{};

        Red::DynArray<int> returnedData{};
        returnedData.Reserve(aSaves->size);
        // Technically a signed/unsigned mismatch, but I have doubts about people having 2 billion+ saves
        for (auto i = 0; i < aSaves->size; i++)
        {
            // We need the save index for this...
            auto& saveName = aSaves->entries[i];

            std::uint64_t hash{};

            if (!files::IsValidForNewGamePlus(saveName.c_str(), hash))
            {
                continue;
            }

            if (playthroughIds.contains(hash))
            {
                continue;
            }

            playthroughIds.insert(hash);

            returnedData.PushBack(i);
        }

        return returnedData;
    }

    bool ParsePointOfNoReturnSaveData(Red::ScriptRef<Red::CString>& aSaveName)
    {
        if (!aSaveName)
        {
            return false;
        }

        // Invalidate and reset
        m_saveData = PlayerSaveData{};

        static const auto basePath = files::GetCpSaveFolder();
        const auto fullPath = basePath / aSaveName->c_str();

        auto start = std::chrono::high_resolution_clock{}.now();

        try
        {
            parser::Parser parser{};

            if (!parser.ParseSavegame(fullPath / "sav.dat"))
            {
                return false;
            }

            m_tweakDb = Red::TweakDB::Get();

            // Needs to be shared across several transferrers :P
            // NOTE: maybe error check this? Eh, shouldn't fail...
            m_statsSystemPtr = parser.LookupNodeData<cyberpunk::StatsSystemNode>();

            if (const auto inventory = parser.LookupNodeData<cyberpunk::InventoryNode>())
            {
                LoadInventoryNew(inventory);
            }

            if (const auto persistencySystem = parser.LookupNodeData<cyberpunk::PersistencySystemNode>())
            {
                // Vehicle garage can technically not be present in a savegame

                if (auto garageComponent =
                        persistencySystem->LookupInstanceAs<Red::GarageComponentPS>("vehicleGarageComponentPS"))
                {
                    LoadGarageNative(garageComponent);
                }
            }

            if (auto scriptableSystemsContainer = parser.LookupNodeData<cyberpunk::ScriptableSystemsContainerNode>())
            {
                // All of them kind of have to be present lol

                // Yes, I should be resolving them through handles
                // But that adds more complexity to the scriptable parser

                if (auto playerDevelopmentData = scriptableSystemsContainer->LookupInstance("PlayerDevelopmentData"))
                {
                    LoadPlayerDevelopmentData(playerDevelopmentData);
                }

                if (auto equipmentSystemPlayerData =
                        scriptableSystemsContainer->LookupInstance("EquipmentSystemPlayerData"))
                {
                    LoadEquipmentSystemPlayerData(equipmentSystemPlayerData);
                }

                if (auto craftBook = scriptableSystemsContainer->LookupInstance("CraftBook"))
                {
                    LoadCraftBook(craftBook);
                }
            }

            LoadStatModifiers();
        }
        catch (std::exception e)
        {
            PluginContext::Error(std::format("EXCEPTION {}", e.what()));
            return false;
        }

        auto end = std::chrono::high_resolution_clock{}.now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        PluginContext::Spew(std::format("Time taken: {}", duration));

        m_saveData.m_isValid = true;
        return true;
    }

    // Some users don't have LogChannel defined :P
    void Spew(Red::ScriptRef<Red::CString>& aStr)
    {
        PluginContext::Spew(aStr->c_str());
    }

    void Error(Red::ScriptRef<Red::CString>& aStr)
    {
        PluginContext::Error(aStr->c_str());
    }

private:
    void HandleUselessStatPoints()
    {
        // This logic is fucked and results in too many added perk points...
        // Figure it out later
        // NVM, wasn't the fault of this - was the fault of proficiency effectors...
        constexpr auto minAttributeValue = 3;
        constexpr auto attributeCount = 5;

        constexpr auto maxAttributePointCount = (20 - minAttributeValue) * attributeCount;

        const auto allocatedAttributePoints =
            (m_saveData.m_playerBodyAttribute + m_saveData.m_playerReflexAttribute + m_saveData.m_playerTechAttribute +
             m_saveData.m_playerIntelligenceAttribute + m_saveData.m_playerCoolAttribute) -
            minAttributeValue * attributeCount;

        const auto totalAttributePoints = allocatedAttributePoints + m_saveData.m_playerAttributePoints;

        if (totalAttributePoints > maxAttributePointCount && m_saveData.m_playerAttributePoints > 0)
        {
            m_saveData.m_addedPerkPointsFromOverflow = true;
            m_saveData.m_playerPerkPoints += m_saveData.m_playerAttributePoints;
            m_saveData.m_playerAttributePoints = 0;
        }

        constexpr auto maxRelicPoints = 15;

        if (m_saveData.m_playerRelicPoints > maxRelicPoints)
        {
            m_saveData.m_addedPerkPointsFromOverflow = true;
            m_saveData.m_playerPerkPoints += (m_saveData.m_playerRelicPoints - maxRelicPoints);
            m_saveData.m_playerRelicPoints = maxRelicPoints;
        }

        // https://old.reddit.com/r/LowSodiumCyberpunk/comments/1690oq4/mathed_out_the_new_perk_trees_coming_in_20_over/
        constexpr auto maxPerkPointCount = 41 + 46 + 44 + 42 + 46;

        if (m_saveData.m_playerPerkPoints > maxPerkPointCount)
        {
            m_saveData.m_playerMoney += (1000 * (m_saveData.m_playerPerkPoints - maxPerkPointCount));
            m_saveData.m_playerPerkPoints = maxPerkPointCount;
        }
    }

    void LoadStatModifiers()
    {
        constexpr auto playerStatObjectId = 1ull;

        auto statBuffer = m_statsSystemPtr->GetStatModifiers(playerStatObjectId);

        using namespace Red::game;
        using Red::game::data::StatType;

        for (auto& stat : statBuffer)
        {
            if (!stat)
            {
                continue;
            }

            // NOTE: all CW shards/carry capacity shards seem to come in constant stat mod...
            // Maybe it would be better to have a DynArray of added stat mods?
            if (const auto& asConstant = Red::Cast<ConstantStatModifierData_Deprecated>(stat))
            {
                constexpr auto arbitraryBigStatValue = 300.f; // Should be big enough
                if (asConstant->value > arbitraryBigStatValue)
                {
                    // Old NG+ added really big carry cap/humanity modifiers
                    // This should filter them out...
                    continue;
                }

                if (asConstant->statType == StatType::Humanity)
                {
                    m_saveData.m_playerCyberwareCapacity.PushBack(asConstant->value);
                }
                else if (asConstant->statType == StatType::CarryCapacity)
                {
                    m_saveData.m_playerCarryCapacity.PushBack(asConstant->value);
                }
            }
        }
    }

    // CRINGE (AND SLOW) CODE AHEAD (It's actually not too slow in the release configuration)
    // This is the result of not using proper RTTI classes...
    void LoadPlayerDevelopmentData(PlayerDevelopmentData aPlayerDevelopmentData)
    {
        auto pSaveData = &m_saveData;

        const auto developmentPointIterator = [pSaveData](SDevelopmentPoints aDevPoints)
        {
            auto spentCount = aDevPoints.GetSpent();
            auto unspentCount = aDevPoints.GetUnspent();

            using Red::game::data::DevelopmentPointType;

            switch (aDevPoints.GetType())
            {
            case DevelopmentPointType::Espionage: // Relic
                pSaveData->m_playerRelicPoints += unspentCount;
                break;
            case DevelopmentPointType::Attribute:
                pSaveData->m_playerAttributePoints += unspentCount;
                break;
            case DevelopmentPointType::Primary: // Perks
                pSaveData->m_playerPerkPoints += unspentCount;
                break;
            default:
                break;
            }
        };

        aPlayerDevelopmentData.IterateOverDevPoints(developmentPointIterator);

        const auto attributeDataIterator = [pSaveData](SAttributeData aAttributeData)
        {
            auto isEspionage = aAttributeData.GetType() == AttributeDataType::EspionageAttributeData;

            const auto newPerkIterator = [pSaveData, isEspionage](SNewPerk aNewPerk)
            {
                auto currLevel = aNewPerk.GetCurrLevel();

                if (currLevel == 0)
                {
                    return;
                }

                if (isEspionage)
                {
                    // Espionage doesn't have levels
                    pSaveData->m_playerRelicPoints += aNewPerk.IsEspionageMilestonePerk() ? 3 : 1;
                }
                else
                {
                    pSaveData->m_playerPerkPoints += currLevel;
                }
            };

            aAttributeData.IterateOverUnlockedPerks(newPerkIterator);
        };

        aPlayerDevelopmentData.IterateOverAttributesData(attributeDataIterator);

        const auto proficiencyIterator = [pSaveData](SProficiency aProficiency)
        {
            const auto currLevel = aProficiency.GetCurrentLevel();
            using Red::game::data::ProficiencyType;
            switch (aProficiency.GetType())
            {
            case ProficiencyType::Level:
                pSaveData->m_playerLevel = std::clamp(currLevel, 1, 50);
                break;
            case ProficiencyType::StreetCred:
                pSaveData->m_playerStreetCred = currLevel;
                break;
            case ProficiencyType::StrengthSkill:
                pSaveData->m_playerBodySkillLevel = currLevel;
                break;
            case ProficiencyType::ReflexesSkill:
                pSaveData->m_playerReflexSkillLevel = currLevel;
                break;
            case ProficiencyType::TechnicalAbilitySkill:
                pSaveData->m_playerTechSkillLevel = currLevel;
                break;
            case ProficiencyType::IntelligenceSkill:
                pSaveData->m_playerIntelligenceSkillLevel = currLevel;
                break;
            case ProficiencyType::CoolSkill:
                pSaveData->m_playerCoolSkillLevel = currLevel;
                break;
            }
        };

        aPlayerDevelopmentData.IterateOverProficiencies(proficiencyIterator);

        const auto attributeIterator = [pSaveData](SAttribute aAttribute)
        {
            const auto currLevel = aAttribute.GetValue();

            using Red::game::data::StatType;

            switch (aAttribute.GetAttributeName())
            {
            case StatType::Strength:
                pSaveData->m_playerBodyAttribute = currLevel;
                break;
            case StatType::Reflexes:
                pSaveData->m_playerReflexAttribute = currLevel;
                break;
            case StatType::TechnicalAbility:
                pSaveData->m_playerTechAttribute = currLevel;
                break;
            case StatType::Intelligence:
                pSaveData->m_playerIntelligenceAttribute = currLevel;
                break;
            case StatType::Cool:
                pSaveData->m_playerCoolAttribute = currLevel;
                break;
            }
        };

        aPlayerDevelopmentData.IterateOverAttributes(attributeIterator);

        HandleUselessStatPoints();
    }

    void LoadEquipmentSystemPlayerData(EquipmentSystemPlayerData aEquipmentSystemPlayerData)
    {
        auto loadoutData = aEquipmentSystemPlayerData.GetLoadout();

        if (!loadoutData)
        {
            PluginContext::Error("NewGamePlusSystem::LoadEquipmentSystemPlayerData, loadoutData is null");
            return;
        }

        using Red::game::data::EquipmentArea;

        for (auto& area : loadoutData->equipAreas)
        {
            switch (area.areaType)
            {
            case EquipmentArea::EyesCW:
                if (m_saveData.m_playerEquippedKiroshis.IsValid())
                {
                    break;
                }

                for (auto& equippedItemData : area.equipSlots)
                {
                    // We need to get rid of the cybermask
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    // NOTE: no need to check for CW record existence, we know it exists if flat ptr exists
                    Red::TweakDBID tagsFlat(itemId.tdbid, ".tags");

                    auto flat = m_tweakDb->GetFlatValue(tagsFlat);

                    if (!flat)
                    {
                        continue;
                    }

                    auto tagList = flat->GetValue<Red::DynArray<Red::CName>>();

                    if (!tagList->Contains("MaskCW"))
                    {
                        m_saveData.m_playerEquippedKiroshis = itemId;
                        break;
                    }
                }
                break;
            case EquipmentArea::SystemReplacementCW:
                if (m_saveData.m_playerEquippedOperatingSystem.IsValid())
                {
                    break;
                }

                for (auto& equippedItemData : area.equipSlots)
                {
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    m_saveData.m_playerEquippedOperatingSystem = itemId;
                    break;
                }
                break;
            case EquipmentArea::CardiovascularSystemCW:
                if (m_saveData.m_playerEquippedCardiacSystemCW.size > 0)
                {
                    break;
                }
                for (auto& equippedItemData : area.equipSlots)
                {
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    m_saveData.m_playerEquippedCardiacSystemCW.PushBack(itemId);
                }
                break;
            case EquipmentArea::ArmsCW:
                if (m_saveData.m_playerEquippedArmCyberware.IsValid())
                {
                    break;
                }
                for (auto& equippedItemData : area.equipSlots)
                {
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    m_saveData.m_playerEquippedArmCyberware = itemId;
                    break;
                }
                break;
            case EquipmentArea::LegsCW:
                if (m_saveData.m_playerEquippedLegCyberware.IsValid())
                {
                    break;
                }
                for (auto& equippedItemData : area.equipSlots)
                {
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    m_saveData.m_playerEquippedLegCyberware = itemId;
                    break;
                }
                break;
            }
        }
    }

    void LoadCraftBook(CraftBook aCraftBook)
    {
        auto pSaveData = &m_saveData;

        aCraftBook.IterateOverRecipes(
            [pSaveData](ItemRecipe aRecipe)
            {
                RedCraftInfo craftInfo{};

                craftInfo.m_hideOnItemsAdded = aRecipe.GetHideOnAdded();
                craftInfo.m_amount = aRecipe.GetAmount();
                craftInfo.m_targetItem = aRecipe.GetTargetItem();

                pSaveData->m_knownRecipeTargetItems.PushBack(std::move(craftInfo));
            });
    }

    struct ExtendedItemData
    {
        Red::ItemID m_itemId;
        Red::TweakDBID m_tdbId;
        Red::Handle<Red::TweakDBRecord> m_itemRecord;
        Red::DynArray<Red::CName>* m_tags;

        Red::gamedataItemType m_itemType;

        bool IsHiddenInUI() const
        {
            return HasTag("HideInUI") || HasTag("HideAtVendor");
        }

        bool IsValidAttachment() const
        {
            return HasTag("WeaponMod") || m_itemType == Red::gamedataItemType::Prt_Program;
        }

        bool IsDyingNight() const
        {
            return HasTag("Lexington_Wilson");
        }

        bool IsAllowedType() const
        {
            using Red::gamedataItemType;
            using Red::game::data::ItemType;
            switch (m_itemType)
            {
            case ItemType::Con_Edible:
            case ItemType::Gen_DataBank:
            case ItemType::CyberwareStatsShard:
            case ItemType::Gen_Jewellery:
            case ItemType::Gen_Junk:
            case ItemType::Gen_Keycard:
            case ItemType::Gen_MoneyShard:
            case ItemType::Gen_Misc:
            case ItemType::Gen_Readable:
            case ItemType::Prt_Receiver:
            case ItemType::Prt_Magazine:
            case ItemType::Prt_ScopeRail:
            case ItemType::Prt_Stock:
            case ItemType::Gen_Tarot:
            case ItemType::VendorToken:
            case ItemType::Wea_Fists:
            case ItemType::Wea_VehicleMissileLauncher:
            case ItemType::Wea_VehiclePowerWeapon:
            case ItemType::Invalid:
            case ItemType::Grenade_Core:
                return false;
            }

            return true;
        }

        bool HasTag(Red::CName aTag) const
        {
            if (!m_tags)
            {
                return false;
            }

            return m_tags->Contains(aTag);
        }
    };

    ExtendedItemData CreateExtendedData(const Red::ItemID& aItemId)
    {
        ExtendedItemData ret{};

        using namespace Red;
        using game::data::ItemType;

        ret.m_itemId = aItemId;
        ret.m_tdbId = aItemId.tdbid;
        ret.m_itemType = ItemType::Invalid;

        if (auto tagValuePtr = m_tweakDb->GetFlatValue(TweakDBID(ret.m_tdbId, ".tags")))
        {
            ret.m_tags = tagValuePtr->GetValue<DynArray<CName>>();
        }

        if (auto typeValuePtr = m_tweakDb->GetFlatValue(TweakDBID(ret.m_tdbId, ".itemType")))
        {
            auto typeFk = *typeValuePtr->GetValue<TweakDBID>();

            if (auto nameValuePtr = m_tweakDb->GetFlatValue(TweakDBID(typeFk, ".name")))
            {
                static const auto typeEnum = GetEnum<ItemType>();

                std::int64_t itemType{};
                package::Package::ResolveEnumValue(typeEnum, *nameValuePtr->GetValue<CName>(), itemType);

                ret.m_itemType = static_cast<ItemType>(itemType);
            }
        }

        return ret;
    }

    // Now should handle CW upgrades too
    void ProcessAttachments(const cyberpunk::ItemSlotPart& aSlotPart, Red::DynArray<RedItemData>& aTargetList,
                            RedItemData& aData)
    {
        // Don't bother doing iconic weapon mods
        if (aSlotPart.m_attachmentSlotTdbId == "AttachmentSlots.IconicWeaponModLegendary" ||
            aSlotPart.m_attachmentSlotTdbId == "AttachmentSlots.IconicMeleeWeaponMod1")
        {
            return;
        }

        constexpr auto shouldLogCWStatsShards = false;

        if constexpr (shouldLogCWStatsShards)
        {
            if (aSlotPart.m_attachmentSlotTdbId == "AttachmentSlots.StatsShardSlot")
            {
                const auto rngSeed = aSlotPart.m_itemId.rngSeed; // RNG seed seems to determine CW stat mods...
                const auto tdbId = aSlotPart.m_itemId.tdbid;

                Red::CString str;
                Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, tdbId);

                PluginContext::Spew(std::format("Shard ID: {}, RNG seed: {}", str.c_str(), rngSeed));
            }
        }

        // FIX: CW stats shards not being transferred
        if (aSlotPart.m_attachmentSlotTdbId == "AttachmentSlots.StatsShardSlot")
        {
            aData.m_attachments.PushBack(aSlotPart.m_itemId);
        }

        auto itemId = aSlotPart.m_itemId;

        auto extendedData = CreateExtendedData(aSlotPart.m_itemId);

        if (extendedData.IsAllowedType() && !extendedData.IsHiddenInUI() && extendedData.IsValidAttachment())
        {
            RedItemData itemData{};

            itemData.m_itemId = itemId;
            itemData.m_itemQuantity = 1;

            ProcessStatModifiers(itemData);

            aTargetList.PushBack(std::move(itemData));
        }

        for (const auto& child : aSlotPart.m_children)
        {
            ProcessAttachments(child, aTargetList, aData);
        }
    }

    void AddItemToInventory(const ExtendedItemData& aExtendedData, const cyberpunk::ItemData& aItem,
                            Red::DynArray<RedItemData>& aTargetList, std::unordered_set<Red::TweakDBID>& aAddedIconics)
    {
        // Sorry, but these REALLY annoy me
        if (BlacklistedTDBIDs::IsForbidden(aExtendedData.m_tdbId))
        {
            return;
        }

        if (aExtendedData.IsDyingNight())
        {
            // Just fuck off...
            return;
        }

        if (aExtendedData.m_tdbId == "Items.money")
        {
            m_saveData.m_playerMoney += aItem.m_itemQuantity;
            return;
        }

        if (!aExtendedData.IsAllowedType())
        {
            return;
        }

        if (aExtendedData.IsHiddenInUI())
        {
            // We don't need something like that
            return;
        }

        // Don't add multiple same iconics to one save...
        if (aExtendedData.HasTag("IconicWeapon"))
        {
            // NOTE: this will murder item mods on X-MOD2...
            // Dirty hack fix
            std::unique_lock lock{m_iconicsMutex};

            if (aAddedIconics.contains(aExtendedData.m_tdbId))
            {
                if (aItem.HasExtendedData())
                {
                    RedItemData dummy{};
                    ProcessAttachments(aItem.m_itemSlotPart, aTargetList, dummy);
                }

                return;
            }

            aAddedIconics.insert(aExtendedData.m_tdbId);
        }

        constexpr auto testStatsOnWeapons = false;

        if constexpr (testStatsOnWeapons)
        {
            if (aExtendedData.HasTag("Weapon"))
            {
                Red::CString str;
                Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aExtendedData.m_tdbId);

                PluginContext::Spew(std::format("Gun {}", str.c_str()));

                auto statsObjectId = m_statsSystemPtr->GetEntityHashFromItemId(aExtendedData.m_itemId);
                auto statModifiers = m_statsSystemPtr->GetStatModifiers(statsObjectId);

                if (!statModifiers.size)
                {
                    PluginContext::Spew("No stat modifiers on weapon!");
                }

                m_statsSystemPtr->DumpStatModifiersToConsole(statModifiers);

                static const auto statTypes = Red::GetEnum<Red::game::data::StatType>();

                for (auto i : m_statsSystemPtr->GetDisabledModifiers(statsObjectId))
                {
                    PluginContext::Spew(std::format(
                        "\tDisabled: {}", package::Package::GetEnumString(statTypes, static_cast<std::int64_t>(i))));
                }
            }
        }

        RedItemData itemData{};

        itemData.m_itemId = aExtendedData.m_itemId;
        itemData.m_itemQuantity = aItem.HasQuantity() ? aItem.m_itemQuantity : 1;

        ProcessStatModifiers(itemData);

        if (aItem.HasExtendedData())
        {
            ProcessAttachments(aItem.m_itemSlotPart, aTargetList, itemData);
        }

        aTargetList.PushBack(std::move(itemData));
    }

    void ProcessItem(const cyberpunk::ItemData& aItem, Red::DynArray<RedItemData>& aTargetList,
                     std::unordered_set<Red::TweakDBID>& aAddedIconics)
    {
        auto extendedItemData = CreateExtendedData(aItem.GetItemID());

        AddItemToInventory(extendedItemData, aItem, aTargetList, aAddedIconics);
    }

    void ProcessStatModifiers(RedItemData& aData)
    {
        const auto statsObjectId = m_statsSystemPtr->GetEntityHashFromItemId(aData.m_itemId);
        auto modifiers = m_statsSystemPtr->GetStatModifiers(statsObjectId);

        for (auto& modifier : modifiers)
        {
            if (!modifier)
            {
                continue;
            }

            if (modifier->statType == Red::game::data::StatType::Invalid)
            {
                continue;
            }

            aData.m_statModifiers.PushBack(std::move(modifier));
        }
    }

    void LoadInventoryNew(cyberpunk::InventoryNode* const aInventory)
    {
        std::unordered_set<Red::TweakDBID> addedIconics{};

        auto& inventoryLocal = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdLocal);
        auto& inventoryCarStash = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdCarStash);

        m_saveData.m_playerItems.Reserve(inventoryLocal.inventoryItems.size());
        m_saveData.m_playerStashItems.Reserve(inventoryCarStash.inventoryItems.size());

        Red::JobQueue adderWaiter{};

        // I don't know if this is efficient...
        adderWaiter.Wait(util::job::MakeJob(
            [this, &inventoryLocal, &addedIconics]()
            {
                for (const auto& item : inventoryLocal.inventoryItems)
                {
                    ProcessItem(item, m_saveData.m_playerItems, addedIconics);
                }
            }));

       adderWaiter.Wait(util::job::MakeJob(
            [this, &inventoryCarStash, &addedIconics]()
            {
                for (const auto& item : inventoryCarStash.inventoryItems)
                {
                    ProcessItem(item, m_saveData.m_playerStashItems, addedIconics);
                }
            }));

       Red::WaitForQueue(adderWaiter, std::chrono::seconds(1));
    }

    void LoadGarageNative(Red::GarageComponentPS* aGarage)
    {
        static constexpr auto Demiurge = Red::TweakDBID("Vehicle.v_utility4_thorton_mackinaw_bmf_player");
        static constexpr auto Hoon = Red::TweakDBID("Vehicle.v_sport2_quadra_type66_nomad_tribute");
        static constexpr auto JackieArch = Red::TweakDBID("Vehicle.v_sportbike2_arch_jackie_player");
        static constexpr auto JackieTunedArch = Red::TweakDBID("Vehicle.v_sportbike2_arch_jackie_tuned_player");

        // Hardcoded HACK

        auto& unlockedVehicles = aGarage->unlockedVehicleArray;

        for (auto& unlockedVehicle : unlockedVehicles)
        {
            const auto recordId = unlockedVehicle.vehicleID.recordID;

            if (recordId == Demiurge || recordId == Hoon || recordId == JackieArch || recordId == JackieTunedArch)
            {
                continue;
            }

            m_saveData.m_playerVehicleGarage.PushBack(recordId);
        }
    }

    PlayerSaveData m_saveData{};

    Red::TweakDB* m_tweakDb;
    cyberpunk::StatsSystemNode* m_statsSystemPtr;

    // Multithreading inventory - assures two queues don't screw each other up...
    mutable std::shared_mutex m_iconicsMutex;

    RTTI_IMPL_TYPEINFO(NewGamePlusSystem);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace redscript

RTTI_DEFINE_ENUM(redscript::ENewGamePlusStartType);

RTTI_DEFINE_CLASS(redscript::NewGamePlusSystem, {
    RTTI_METHOD(ParsePointOfNoReturnSaveData);
    RTTI_METHOD(HasPointOfNoReturnSave);
    RTTI_METHOD(ResolveNewGamePlusSaves);
    RTTI_METHOD(GetNewGamePlusState);
    RTTI_METHOD(SetNewGamePlusState);
    RTTI_METHOD(GetSaveData);
    RTTI_METHOD(SetNewGamePlusGameDefinition);
    RTTI_METHOD(IsSaveValidForNewGamePlus);
    RTTI_METHOD(LoadExpansionIntoSave);
    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);
});