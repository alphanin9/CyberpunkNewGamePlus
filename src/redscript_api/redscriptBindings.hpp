#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../context/context.hpp"
#include "../filesystem/fs_util.hpp"
#include "../parsing/fileReader.hpp"

#include "../parsing/definitions/nodeParsers/parserHelper.hpp"
#include "../parsing/definitions/nodeParsers/inventory/inventoryNode.hpp"
#include "../parsing/definitions/nodeParsers/persistency/persistencySystemNode.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/scriptableContainerNode.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/equipmentSystem.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/playerDevelopmentData.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/helpers/classDefinitions/craftBook.hpp"

#include <chrono>

#include <simdjson.h>

// NOT A GOOD IDEA TO HAVE IN A HEADER
// But who cares?

using scriptable::native::EquipmentSystem::EquipmentSystemPlayerData;
using scriptable::native::PlayerDevelopment::PlayerDevelopmentData;
using scriptable::native::CraftBook::CraftBook;

// Enums
using scriptable::native::PlayerDevelopment::AttributeDataType;
using scriptable::native::PlayerDevelopment::EspionageMilestonePerks;

// Wrappers
using scriptable::native::PlayerDevelopment::SAttribute;
using scriptable::native::PlayerDevelopment::SAttributeData;
using scriptable::native::PlayerDevelopment::SDevelopmentPoints;
using scriptable::native::PlayerDevelopment::SNewPerk;
using scriptable::native::PlayerDevelopment::SProficiency;
using scriptable::native::CraftBook::ItemRecipe;

namespace redscript
{
struct RedItemData
{
    Red::ItemID m_itemId{};
    int32_t m_itemQuantity{};

    RTTI_IMPL_TYPEINFO(RedItemData);
    RTTI_IMPL_ALLOCATOR();
};
} // namespace redscript

RTTI_DEFINE_CLASS(redscript::RedItemData, {
    RTTI_PROPERTY(m_itemId);
    RTTI_PROPERTY(m_itemQuantity);
});

namespace redscript
{
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
    Red::DynArray<Red::TweakDBID> m_knownRecipeTargetItems{};

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
});

namespace redscript
{

enum class ENewGamePlusStartType
{
    StartFromQ001,
    StartFromQ101,
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

    void SetNewGamePlusGameDefinition(ENewGamePlusStartType aStartType)
    {
        if (aStartType == ENewGamePlusStartType::StartFromQ001)
        {
            constexpr auto q001Path = RED4ext::ResourcePath::HashSanitized("mod/quest/NewGamePlus_Q001.gamedef");
            PluginContext::m_ngPlusGameDefinitionHash = q001Path;   
        }
        else
        {
            constexpr auto q101Path = RED4ext::ResourcePath::HashSanitized("base/quest/NewGamePlus.gamedef");
            PluginContext::m_ngPlusGameDefinitionHash = q101Path; // Should fix it being in base/ sometime
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
        auto& saveList = *aSaves;
        
        std::unordered_set<std::uint64_t> playthroughIds{};

        Red::DynArray<int> returnedData{};
        returnedData.Reserve(saveList.size);
        // Technically a signed/unsigned mismatch, but I have doubts about people having 2 billion+ saves
        for (auto i = 0; i < saveList.size; i++)
        {
            // We need the save index for this...
            auto& saveName = saveList.entries[i];

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

            if (const auto inventory = parser.LookupNodeData<cyberpunk::InventoryNode>())
            {
                LoadInventoryNew(inventory);
            }

            if (const auto persistencySystem = parser.LookupNodeData<cyberpunk::PersistencySystemNode>())
            {
                // Vehicle garage can technically not be present in a savegame

                if (auto garageComponent = persistencySystem->LookupInstanceAs<Red::GarageComponentPS>("vehicleGarageComponentPS"))
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
        }
        catch (std::exception e)
        {
            PluginContext::Error(std::format("EXCEPTION {}", e.what()));
            m_lastError = e.what();
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
        constexpr auto minAttributeValue = 3;
        constexpr auto attributeCount = 5;

        constexpr auto maxAttributePointCount = (20 - minAttributeValue) * attributeCount;

        const auto allocatedAttributePoints =
            (m_saveData.m_playerBodyAttribute + m_saveData.m_playerReflexAttribute + m_saveData.m_playerTechAttribute +
             m_saveData.m_playerIntelligenceAttribute + m_saveData.m_playerCoolAttribute) -
            minAttributeValue * attributeCount;

        const auto totalAttributePoints = allocatedAttributePoints + m_saveData.m_playerAttributePoints;

        if (totalAttributePoints >= maxAttributePointCount)
        {
            m_saveData.m_playerPerkPoints += m_saveData.m_playerAttributePoints;
            m_saveData.m_playerAttributePoints = 0;
        }

        constexpr auto maxRelicPoints = 15;

        if (m_saveData.m_playerRelicPoints > maxRelicPoints)
        {
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

    // CRINGE (AND SLOW) CODE AHEAD (It's actually not too slow in the release configuration)
    // This is the result of not using proper RTTI classes...
    void LoadPlayerDevelopmentData(PlayerDevelopmentData aPlayerDevelopmentData)
    {
        auto pSaveData = &m_saveData;

        const auto developmentPointIterator = [pSaveData](SDevelopmentPoints aDevPoints)
        { 
            auto spentCount = aDevPoints.GetSpent();
            auto unspentCount = aDevPoints.GetUnspent();

            switch (aDevPoints.GetType())
            {
            case Red::gamedataDevelopmentPointType::Espionage: // Relic
                pSaveData->m_playerRelicPoints += unspentCount;
                break;
            case Red::gamedataDevelopmentPointType::Attribute:
                pSaveData->m_playerAttributePoints += unspentCount;
                break;
            case Red::gamedataDevelopmentPointType::Primary: // Perks
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
            using Red::gamedataProficiencyType;
            switch (aProficiency.GetType())
            {
            case gamedataProficiencyType::Level:
                pSaveData->m_playerLevel = std::clamp(currLevel, 1, 50);
                break;
            case gamedataProficiencyType::StreetCred:
                pSaveData->m_playerStreetCred = currLevel;
                break;
            case gamedataProficiencyType::StrengthSkill:
                pSaveData->m_playerBodySkillLevel = currLevel;
                break;
            case gamedataProficiencyType::ReflexesSkill:
                pSaveData->m_playerReflexSkillLevel = currLevel;
                break;
            case gamedataProficiencyType::TechnicalAbilitySkill:
                pSaveData->m_playerTechSkillLevel = currLevel;
                break;
            case gamedataProficiencyType::IntelligenceSkill:
                pSaveData->m_playerIntelligenceSkillLevel = currLevel;
                break;
            case gamedataProficiencyType::CoolSkill:
                pSaveData->m_playerCoolSkillLevel = currLevel;
                break;
            }
        };

        aPlayerDevelopmentData.IterateOverProficiencies(proficiencyIterator);

        const auto attributeIterator = [pSaveData](SAttribute aAttribute) 
        {
            const auto currLevel = aAttribute.GetValue();

            using Red::gamedataStatType;

            switch (aAttribute.GetAttributeName())
            {
            case gamedataStatType::Strength:
                pSaveData->m_playerBodyAttribute = currLevel;
                break;
            case gamedataStatType::Reflexes:
                pSaveData->m_playerReflexAttribute = currLevel;
                break;
            case gamedataStatType::TechnicalAbility:
                pSaveData->m_playerTechAttribute = currLevel;
                break;
            case gamedataStatType::Intelligence:
                pSaveData->m_playerIntelligenceAttribute = currLevel;
                break;
            case gamedataStatType::Cool:
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

        using Red::gamedataEquipmentArea;

        for (auto& area : loadoutData->equipAreas)
        {
            switch (area.areaType)
            {
            case gamedataEquipmentArea::EyesCW:
                for (auto& equippedItemData : area.equipSlots)
                {
                    // We need to get rid of the cybermask
                    auto& itemId = equippedItemData.itemID;

                    if (!itemId.IsValid())
                    {
                        continue;
                    }

                    auto tweakRecord = m_tweakDb->GetRecord(itemId.tdbid);

                    if (!tweakRecord)
                    {
                        continue;
                    }

                    bool isMask{};
                    Red::CName tagName = "MaskCW";

                    Red::CallVirtual(tweakRecord, "TagsContains", isMask, tagName);

                    if (!isMask)
                    {
                        m_saveData.m_playerEquippedKiroshis = itemId;
                        break;
                    }
                }
                break;
            case gamedataEquipmentArea::SystemReplacementCW:
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
            case gamedataEquipmentArea::CardiovascularSystemCW:
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
            case gamedataEquipmentArea::ArmsCW:
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
            case gamedataEquipmentArea::LegsCW:
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
            [pSaveData](ItemRecipe aRecipe) { 
                pSaveData->m_knownRecipeTargetItems.PushBack(aRecipe.GetTargetItem());
            }
        );
    }
 
    struct ExtendedItemData
    {
        Red::ItemID m_itemId;
        Red::TweakDBID m_tdbId;
        Red::Handle<Red::IScriptable> m_itemRecord;
        Red::DynArray<Red::CName> m_tags;

        Red::gamedataItemType m_itemType;
        Red::CName m_itemCategory;

        bool IsHiddenInUI() const
        {
            return HasTag("HideInUI") || HasTag("HideAtVendor");
        }

        bool IsValidAttachment() const
        {
            return HasTag("WeaponMod") || m_itemType == Red::gamedataItemType::Prt_Program;
        }

        bool IsAllowedType() const
        {
            using Red::gamedataItemType;
            switch (m_itemType)
            {
            case gamedataItemType::Con_Edible:
            case gamedataItemType::Gen_DataBank:
            case gamedataItemType::CyberwareStatsShard:
            case gamedataItemType::Gen_Jewellery:
            case gamedataItemType::Gen_Junk:
            case gamedataItemType::Gen_Keycard:
            case gamedataItemType::Gen_MoneyShard:
            case gamedataItemType::Gen_Misc:
            case gamedataItemType::Gen_Readable:
            case gamedataItemType::Prt_Receiver:
            case gamedataItemType::Prt_Magazine:
            case gamedataItemType::Prt_ScopeRail:
            case gamedataItemType::Prt_Stock:
            case gamedataItemType::Gen_Tarot:
            case gamedataItemType::VendorToken:
            case gamedataItemType::Wea_Fists:
            case gamedataItemType::Wea_VehicleMissileLauncher:
            case gamedataItemType::Wea_VehiclePowerWeapon:
            case gamedataItemType::Invalid:
            case gamedataItemType::Grenade_Core:    
                return false;
            }

            return true;
        }

        bool HasTag(Red::CName aTag) const
        {
            return m_tags.Contains(aTag);
        }
    };

    ExtendedItemData CreateExtendedData(const cyberpunk::ItemInfo& aItem, Red::Handle<Red::IScriptable>& aRecord)
    {
        ExtendedItemData ret{};

        ret.m_itemId = aItem.itemId;
        ret.m_tdbId = ret.m_itemId.tdbid;
        ret.m_itemRecord = aRecord;
        ret.m_itemType = Red::gamedataItemType::Invalid;
        ret.m_itemCategory = "";
        
        Red::CallVirtual(aRecord, "Tags", ret.m_tags);

        Red::Handle<Red::gamedataItemType_Record> itemTypeHandle;
        Red::CallVirtual(aRecord, "ItemTypeHandle", itemTypeHandle);

        if (itemTypeHandle)
        {
            Red::CallVirtual(itemTypeHandle, "Type", ret.m_itemType);
        }

        Red::Handle<Red::gamedataItemCategory_Record> itemCategoryHandle;
        Red::CallVirtual(aRecord, "ItemCategoryHandle", itemCategoryHandle);

        if (itemCategoryHandle)
        {
            Red::CallVirtual(itemCategoryHandle, "Name", ret.m_itemCategory);
        }

        return ret;
    }

    void ProcessAttachments(const cyberpunk::ItemSlotPart& aSlotPart, Red::DynArray<RedItemData>& aTargetList)
    {
        // Don't bother doing iconic weapon mods
        if (aSlotPart.attachmentSlotTdbId == "AttachmentSlots.IconicWeaponModLegendary" ||
            aSlotPart.attachmentSlotTdbId == "AttachmentSlots.IconicMeleeWeaponMod1")
        {
            return;
        }

        auto itemId = aSlotPart.itemInfo.itemId;
        auto record = m_tweakDb->GetRecord(itemId.tdbid);

        if (!record)
        {
            return;
        }

        auto extendedData = CreateExtendedData(aSlotPart.itemInfo, record);

        if (extendedData.IsAllowedType() && !extendedData.IsHiddenInUI() && extendedData.IsValidAttachment())
        {
            RedItemData itemData{};

            itemData.m_itemId = itemId;
            itemData.m_itemQuantity = 1;

            constexpr auto showAddedMods = false;

            if constexpr (showAddedMods)
            {
                Red::CString str{};
		        Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, extendedData.m_tdbId);
                
                PluginContext::Spew(std::format("Added mod {} to inventory!", str.c_str()));
            }

            aTargetList.PushBack(itemData);
        }

        for (const auto& child : aSlotPart.children)
        {
            ProcessAttachments(child, aTargetList);   
        }
    }

    void AddItemToInventory(const ExtendedItemData& aExtendedData, const cyberpunk::ItemData& aItem, Red::DynArray<RedItemData>& aTargetList)
    {
        // Sorry, but these REALLY annoy me
        static constexpr auto bannedTdbIds = std::array<Red::TweakDBID, 32>{
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
            Red::TweakDBID{"Items.Preset_Q001_Lexington"}, Red::TweakDBID{"Items.CyberdeckSplinter"}};

        // TODO: just make this a switch statement using TDBID hashes

        if (std::find(bannedTdbIds.begin(), bannedTdbIds.end(), aExtendedData.m_tdbId) != bannedTdbIds.end())
        {
            return;
        }

        if (aExtendedData.m_tdbId == "Items.money")
        {
            m_saveData.m_playerMoney += aItem.itemQuantity;
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

        RedItemData itemData{};

        itemData.m_itemId = aExtendedData.m_itemId;
        itemData.m_itemQuantity = aItem.hasQuantity() ? aItem.itemQuantity : 1;

        aTargetList.PushBack(itemData);

        if (aItem.hasExtendedData())
        {
            ProcessAttachments(aItem.itemSlotPart, aTargetList);
        }
    }

    void ProcessItem(const cyberpunk::ItemData& aItem, Red::DynArray<RedItemData>& aTargetList)
    {
        auto record = m_tweakDb->GetRecord(aItem.itemInfo.itemId.tdbid);

        if (!record)
        {
            return;
        }

        auto extendedItemData = CreateExtendedData(aItem.itemInfo, record);

        AddItemToInventory(extendedItemData, aItem, aTargetList);
    }

    void LoadInventoryNew(cyberpunk::InventoryNode* const aInventory)
    {
        auto& inventoryLocal = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdLocal);
        auto& inventoryCarStash = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdCarStash);

        for (const auto& item : inventoryLocal.inventoryItems)
        {
            ProcessItem(item, m_saveData.m_playerItems);
        }

        for (const auto& item : inventoryCarStash.inventoryItems)
        {
            ProcessItem(item, m_saveData.m_playerStashItems);
        }
    }

    void LoadGarageNative(Red::GarageComponentPS* aGarage)
    {
        static constexpr std::array<Red::TweakDBID, 32> blacklistedVehicles =
                                            {Red::TweakDBID{"Vehicle.v_utility4_thorton_mackinaw_bmf_player"},
                                           Red::TweakDBID{"Vehicle.v_sport2_quadra_type66_nomad_tribute"},
                                           Red::TweakDBID{"Vehicle.v_sportbike2_arch_jackie_player"},
                                           Red::TweakDBID{"Vehicle.v_sportbike2_arch_jackie_tuned_player"}};

        auto& unlockedVehicles = aGarage->unlockedVehicleArray;

        for (auto& unlockedVehicle : unlockedVehicles)
        {
            if (std::find(blacklistedVehicles.begin(), blacklistedVehicles.end(), unlockedVehicle.vehicleID.recordID) == blacklistedVehicles.end())
            {
                m_saveData.m_playerVehicleGarage.PushBack(unlockedVehicle.vehicleID.recordID);
            }
        }
    }

    PlayerSaveData m_saveData{};
    std::string m_lastError{};

    Red::TweakDB* m_tweakDb;

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
    RTTI_METHOD(Spew);
    RTTI_METHOD(Error);
});