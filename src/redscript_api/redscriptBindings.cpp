#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ConstantStatModifierData_Deprecated.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/quest/QuestsSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include <RedLib.hpp>

#include <context.hpp>

#include "../filesystem/fs_util.hpp"
#include "../parsing/fileReader.hpp"

#include "../parsing/definitions/nodeParsers/inventory/inventoryNode.hpp"
#include "../parsing/definitions/nodeParsers/parserHelper.hpp"
#include "../parsing/definitions/nodeParsers/persistency/persistencySystemNode.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/scriptableContainerNode.hpp"
#include "../parsing/definitions/nodeParsers/stats/statsSystemNode.hpp"
#include "../parsing/definitions/nodeParsers/wardrobe/wardrobeSystemNode.hpp"

#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/craftBook.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/equipmentSystem.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/playerDevelopmentData.hpp"

#include "../util/offsetPtr.hpp"
#include "../util/threads.hpp"

#include <chrono>

#include "definitions/playerSaveData.hpp"

// TODO: redo all of this
// All of this is SHIT
// Redesign things: move loaders for different data into their own namespaces, pass handle to progression data around?
// Maybe figure out how to add things to save metadata and add ngplusActive field to it...

// Small refactor: move complex reader stuff (player development system, equipment system, inventory...) into their own files

using namespace Red;

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
    ItemID m_itemId{};
    int32_t m_itemQuantity{};

    // HACK: make CW upgrades carry over...
    // Makes things a bit heavier, but w/e
    DynArray<ItemID> m_attachments;
    DynArray<Handle<game::StatModifierData_Deprecated>> m_statModifiers;

    RTTI_IMPL_TYPEINFO(RedItemData);
    RTTI_IMPL_ALLOCATOR();
};

struct RedCraftInfo
{
    TweakDBID m_targetItem;
    int m_amount{};
    DynArray<ItemID> m_hideOnItemsAdded;

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
    DynArray<RedItemData> m_playerItems{};
    DynArray<RedItemData> m_playerStashItems{};

    DynArray<RedCraftInfo> m_knownRecipeTargetItems{};

    // Most distinctive cyberware needs to be equipped
    // by the scripting side to make
    // the player feel at home from the get-go
    // Clothing does not, as the endpoint in Q101
    // will still end up with the player naked

    // Cyberware-EX will mess this up, but do we care?
    ItemID m_playerEquippedOperatingSystem{};
    ItemID m_playerEquippedKiroshis{};
    ItemID m_playerEquippedLegCyberware{};
    ItemID m_playerEquippedArmCyberware{};

    DynArray<ItemID> m_playerEquippedCardiacSystemCW;

    // Filter this from quest vehicles?
    // (Quadra, Demiurge, ETC)
    // Nah, shit idea, maybe for the Demiurge
    // Seeing as acquiring it is pretty cool
    DynArray<TweakDBID> m_playerVehicleGarage{};

    bool m_addedPerkPointsFromOverflow{};

    // NOTE: these do not account for perks like Edgerunner/whatever else... only shards
    // Whatever, we won't be overallocating CW cap anyway :P
    DynArray<float> m_playerCyberwareCapacity{};
    DynArray<float> m_playerCarryCapacity{};

    DynArray<save::WardrobeEntry> m_wardrobeEntries{};

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
    RTTI_PROPERTY(m_wardrobeEntries);
});

namespace BlacklistedTDBIDs
{
inline constexpr auto MaskCW = TweakDBID("Items.MaskCW");
inline constexpr auto MaskCWPlus = TweakDBID("Items.MaskCWPlus");
inline constexpr auto MaskCWPlusPlus = TweakDBID("Items.MaskCWPlusPlus");
inline constexpr auto Fists = TweakDBID("Items.w_melee_004__fists_a");
inline constexpr auto PersonalLink = TweakDBID("Items.PersonalLink");

inline constexpr auto PersonalLink2 = TweakDBID("Items.personal_link");
inline constexpr auto MaTppHead = TweakDBID("Items.PlayerMaTppHead");
inline constexpr auto WaTppHead = TweakDBID("Items.PlayerWaTppHead");
inline constexpr auto FppHead = TweakDBID("Items.PlayerFppHead");
inline constexpr auto HolsteredFists = TweakDBID("Items.HolsteredFists");

inline constexpr auto MQ024DataCarrier = TweakDBID("Items.mq024_sandra_data_carrier");
inline constexpr auto Skippy = TweakDBID("Items.mq007_skippy");
inline constexpr auto SkippyPostQuest = TweakDBID("Items.mq007_skippy_post_quest");
inline constexpr auto PresetSkippy = TweakDBID("Items.Preset_Yukimura_Skippy");
inline constexpr auto PresetSkippyPostQuest = TweakDBID("Items.Preset_Yukimura_Skippy_PostQuest");

inline constexpr auto SaburoDataCarrier = TweakDBID("Items.q005_saburo_data_carrier");
inline constexpr auto SaburoDataCarrierCracked = TweakDBID("Items.q005_saburo_data_carrier_cracked");
inline constexpr auto Q003Chip = TweakDBID("Items.q003_chip");
inline constexpr auto Q003ChipCracked = TweakDBID("Items.q003_chip_cracked");
inline constexpr auto Q003ChipCrackedFunds = TweakDBID("Items.q003_chip_cracked_funds");

inline constexpr auto CyberdeckSplinter = TweakDBID("Items.CyberdeckSplinter");
inline constexpr auto TiconGwent = TweakDBID("Items.Preset_Ticon_Gwent");
inline constexpr auto WitcherSword = TweakDBID("Items.Preset_Sword_Witcher");

inline bool IsForbidden(TweakDBID aId)
{
    // Disgusting
    return aId == MaskCW || aId == MaskCWPlus || aId == MaskCWPlusPlus || aId == Fists || aId == PersonalLink ||
           aId == PersonalLink2 || aId == MaTppHead || aId == WaTppHead || aId == FppHead || aId == HolsteredFists ||
           aId == MQ024DataCarrier || aId == Skippy || aId == SkippyPostQuest || aId == PresetSkippy ||
           aId == PresetSkippyPostQuest || aId == SaburoDataCarrier || aId == SaburoDataCarrierCracked ||
           aId == Q003Chip || aId == Q003ChipCracked || aId == Q003ChipCrackedFunds || aId == CyberdeckSplinter ||
           aId == TiconGwent || aId == WitcherSword;
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
    StartFromQ101_ProgressionBuild,
    StartFromQ101_ProgressionBuild_NoEP1,
    Count,
    Invalid
};

// Sidenote: this does not need IGameSystem inheritance...
// Whatever
class NewGamePlusSystem : public IGameSystem
{
public:
    // Should really be ref/wref...
    PlayerSaveData GetSaveData()
    {
        return m_saveData;
    }

    Handle<NGPlusProgressionData> GetProgressionData()
    {
        return m_progressionData;
    }

    bool GetNewGamePlusState() const
    {
        return PluginContext::m_isNewGamePlusActive;
    }

    void SetNewGamePlusState(bool aNewState)
    {
        PluginContext::m_isNewGamePlusActive = aNewState;
    }

    bool GetStandaloneState() const
    {
        return m_isInStandalone;
    }

    void SetStandaloneState(bool aNewState)
    {
        m_isInStandalone = aNewState;
    }

    bool IsInNewGamePlusSave()
    {
        auto questsSystem = GetGameSystem<quest::QuestsSystem>();

        if (!questsSystem)
        {
            return false;
        }

        // https://github.com/psiberx/cp2077-archive-xl/blob/9653e9d2eb07831941533fdff3839fc9bef80c8d/src/Red/QuestsSystem.hpp#L169
        // Should be accessed somewhere in QuestsSystem::OnGameLoad, I think?
        auto& questsList = util::OffsetPtr<0xA8, DynArray<ResourcePath>>::Ref(questsSystem);

        constexpr std::array c_ngPlusQuests = {ResourcePath(R"(mod\quest\NewGamePlus.quest)"),
                                               ResourcePath(R"(mod\quest\NewGamePlus_Standalone.quest)"),
                                               ResourcePath(R"(mod\quest\NewGamePlus_Q001.quest)")};

        for (auto questResource : questsList)
        {
            if (std::find(c_ngPlusQuests.begin(), c_ngPlusQuests.end(), questResource) != c_ngPlusQuests.end())
            {
                return true;
            }
        }

        return false;
    }

    void LoadExpansionIntoSave()
    {
        // We only have the one expansion... Thank God

        // TODO: move all hashes into a header?
        // Vtable index 62
        // Sig: 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 ED B8
        constexpr auto addQuestHash = 1617892594u;

        // Maybe check if EP1 is already running? Eh, the Redscript side already does...

        static const auto fnAddQuest =
            UniversalRelocFunc<void(__fastcall*)(quest::QuestsSystem * aQuestsSystem, ResourcePath aPath)>(
                addQuestHash);

        constexpr ResourcePath EP1 = R"(ep1\quest\ep1.quest)";

        fnAddQuest(GetGameSystem<quest::QuestsSystem>(), EP1);
    }

    void SetNewGamePlusGameDefinition(ENewGamePlusStartType aStartType)
    {
        constexpr auto q001Path = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001.gamedef");
        constexpr auto q101Path = ResourcePath::HashSanitized("mod/quest/NewGamePlus.gamedef");

        // These ones do not have EP1 enabled by default, but will have it enabled via custom quest starting
        constexpr auto q001PathNoEP1 = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001_NoEP1.gamedef");
        constexpr auto q101PathNoEP1 = ResourcePath::HashSanitized("mod/quest/NewGamePlus_NoEP1.gamedef");

        // Bare Post-Heist starts, applying EP1 Standalone progression builds...
        constexpr auto standalonePath = ResourcePath::HashSanitized("mod/quest/NewGamePlus_Standalone.gamedef");
        constexpr auto standalonePathNoEP1 =
            ResourcePath::HashSanitized("mod/quest/NewGamePlus_Standalone_NoEP1.gamedef");

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
        case ENewGamePlusStartType::StartFromQ101_ProgressionBuild:
            PluginContext::m_ngPlusGameDefinitionHash = standalonePath;
            break;
        case ENewGamePlusStartType::StartFromQ101_ProgressionBuild_NoEP1:
            PluginContext::m_ngPlusGameDefinitionHash = standalonePathNoEP1;
            break;
        default:
            break;
        }
    }

    bool HasPointOfNoReturnSave()
    {
        return files::HasValidPointOfNoReturnSave();
    }

    bool IsSaveValidForNewGamePlus(Red::ScriptRef<CString>& aSaveName)
    {
        return files::IsValidForNewGamePlus(aSaveName->c_str());
    }

    DynArray<int> ResolveNewGamePlusSaves(Red::ScriptRef<DynArray<CString>>& aSaves)
    {
        if (!aSaves)
        {
            return {};
        }

        // We don't need sorting, the game already sorts the saves before passing them to Redscript
        std::unordered_set<std::uint64_t> playthroughIds{};

        DynArray<int> returnedData{};
        returnedData.Reserve(aSaves->size);
        // Technically a signed/unsigned mismatch, but I have doubts about people having 2 billion+ saves
        for (auto i = 0; i < aSaves->size; i++)
        {
            // We need the save index for this...
            auto& saveName = aSaves->entries[i];

            auto savePath = files::GetRedPathToSaveFile(saveName.c_str(), files::c_metadataFileName);

            std::uint64_t hash{};

            if (!files::IsValidForNewGamePlus(savePath, hash))
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

    bool ParsePointOfNoReturnSaveData(Red::ScriptRef<CString>& aSaveName)
    {
        if (!aSaveName)
        {
            return false;
        }

        // Invalidate and reset
        m_saveData = PlayerSaveData{};

        auto start = std::chrono::high_resolution_clock{}.now();

        try
        {
            parser::Parser parser{};

            if (!parser.ParseSavegame(*aSaveName))
            {
                return false;
            }

            if (m_progressionData)
            {
                m_progressionData.Reset();
            }

            m_progressionData = MakeHandle<NGPlusProgressionData>(parser);
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
    void Spew(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Spew(aStr->c_str());
    }

    void Error(Red::ScriptRef<CString>& aStr)
    {
        PluginContext::Error(aStr->c_str());
    }

private:
    // Old now
    PlayerSaveData m_saveData{};

    Handle<NGPlusProgressionData> m_progressionData{};

    bool m_isInStandalone{};

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
    RTTI_METHOD(GetProgressionData);
    RTTI_METHOD(SetNewGamePlusGameDefinition);
    RTTI_METHOD(IsSaveValidForNewGamePlus);
    RTTI_METHOD(LoadExpansionIntoSave);
    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);
    RTTI_METHOD(GetStandaloneState);
    RTTI_METHOD(SetStandaloneState);
    RTTI_METHOD(IsInNewGamePlusSave);
});