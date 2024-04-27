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
    // Maybe add Blood Pump and biomon here as well? Not sure :D

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
    RTTI_PROPERTY(m_playerEquippedOperatingSystem);
    RTTI_PROPERTY(m_playerEquippedKiroshis);
    RTTI_PROPERTY(m_playerEquippedLegCyberware);
    RTTI_PROPERTY(m_playerEquippedArmCyberware);
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

        try
        {
            parser::Parser parser{};

            const auto wasParseSuccessful = parser.ParseSavegame(fullPath / "sav.dat");

            if (!wasParseSuccessful)
            {
                return false;
            }

            const auto inventoryNode = static_cast<cyberpunk::InventoryNode*>(
                parser.LookupNode(cyberpunk::InventoryNode::nodeName)->nodeData.get());
            const auto persistencySystemNode = static_cast<cyberpunk::PersistencySystemNode*>(
                parser.LookupNode(cyberpunk::PersistencySystemNode::nodeName)->nodeData.get());
            const auto scriptableSystemsContainerNode = static_cast<cyberpunk::ScriptableSystemsContainerNode*>(
                parser.LookupNode(cyberpunk::ScriptableSystemsContainerNode::nodeName)->nodeData.get());

            const auto& equipmentSystemPlayerData =
                scriptableSystemsContainerNode->LookupChunk("EquipmentSystemPlayerData").m_redClass;
            const auto& playerDevelopmentData =
                scriptableSystemsContainerNode->LookupChunk("PlayerDevelopmentData").m_redClass;
            const auto& vehicleGarageData = persistencySystemNode->LookupChunk("vehicleGarageComponentPS").m_redClass;

            LoadPlayerDevelopmentData(playerDevelopmentData);
            LoadEquipmentSystemPlayerData(equipmentSystemPlayerData);
            //LoadInventory(inventoryNode);
            LoadInventoryNew(inventoryNode);
            LoadGarage(vehicleGarageData);
        }
        catch (std::exception e)
        {
            PluginContext::Error(std::format("EXCEPTION {}", e.what()));
            m_lastError = e.what();
            return false;
        }

        m_saveData.m_isValid = true;
        return true;
    }

private:
    // CRINGE (AND SLOW) CODE AHEAD (It's actually not too slow in the release configuration)
    // This is the result of not using proper RTTI classes...
    void LoadPlayerDevelopmentData(const redRTTI::RTTIValue& aPlayerDevelopmentData)
    {
        // Load unspent points? Kinda useless, TBH (Also completely broken with progression builds)
        // Maybe useful for Relic? Or wraparound into perk points for attribute points that can't be spent anymore
        for (auto& devPointData : aPlayerDevelopmentData["devPoints"].AsArray())
        {
            const auto pointType = devPointData["type"].AsCName();
            const auto unspentPointCount = devPointData["unspent"].AsInt();

            if (pointType == "Espionage")
            {
                m_saveData.m_playerRelicPoints = unspentPointCount + devPointData["spent"].AsInt();

                // 3 milestone perks - 9 points
                // 4 cyberarm perks - 4 points
                // 1 additional perk for stealth
                // 1 additional perk for weakspots
                constexpr auto MAX_RELIC_POINTS = 15;

                const auto relicPointSpillover = m_saveData.m_playerRelicPoints - MAX_RELIC_POINTS;

                if (relicPointSpillover > 0)
                {
                    m_saveData.m_playerPerkPoints += relicPointSpillover;
                    m_saveData.m_playerRelicPoints = MAX_RELIC_POINTS;
                }
            }
            else if (pointType == "Primary")
            {                                                       // Perks
                m_saveData.m_playerPerkPoints += unspentPointCount; // Fill the unspent points
            }
        }

        auto perkPointsSpent = 0;

        for (auto& attributePerkData : aPlayerDevelopmentData["attributesData"].AsArray())
        {
            auto unlockedPerks = attributePerkData["unlockedPerks"].AsArray();

            if (attributePerkData["type"].Equals(Red::CName{"EspionageAttributeData"}))
            {
                // We already process Espionage
                continue;
            }

            for (auto& perk : unlockedPerks)
            {
                perkPointsSpent += perk["currLevel"].AsInt();
            }
        }

        m_saveData.m_playerPerkPoints += perkPointsSpent;

        for (auto& proficiency : aPlayerDevelopmentData["proficiencies"].AsArray())
        {
            auto proficiencyLevel = proficiency["currentLevel"].AsInt();
            auto proficiencyType = proficiency["type"].AsCName();

            if (proficiencyType == "Level")
            {
                m_saveData.m_playerLevel = std::clamp(proficiencyLevel, 1, 50);
            }
            else if (proficiencyType == "StreetCred")
            {
                m_saveData.m_playerStreetCred = proficiencyLevel;
            }
            else if (proficiencyType == "StrengthSkill")
            {
                m_saveData.m_playerBodySkillLevel = proficiencyLevel;
            }
            else if (proficiencyType == "ReflexesSkill")
            {
                m_saveData.m_playerReflexSkillLevel = proficiencyLevel;
            }
            else if (proficiencyType == "TechnicalAbilitySkill")
            {
                m_saveData.m_playerTechSkillLevel = proficiencyLevel;
            }
            else if (proficiencyType == "IntelligenceSkill")
            {
                m_saveData.m_playerIntelligenceSkillLevel = proficiencyLevel;
            }
            else if (proficiencyType == "CoolSkill")
            {
                m_saveData.m_playerCoolSkillLevel = proficiencyLevel;
            }
        }
        for (auto& attribute : aPlayerDevelopmentData["attributes"].AsArray())
        {
            auto attributeLevel = attribute["value"].AsInt();
            auto attributeName = attribute["attributeName"].AsCName();

            if (attributeName == "Strength")
            {
                m_saveData.m_playerBodyAttribute = attributeLevel;
            }
            else if (attributeName == "Reflexes")
            {
                m_saveData.m_playerReflexAttribute = attributeLevel;
            }
            else if (attributeName == "TechnicalAbility")
            {
                m_saveData.m_playerTechAttribute = attributeLevel;
            }
            else if (attributeName == "Intelligence")
            {
                m_saveData.m_playerIntelligenceAttribute = attributeLevel;
            }
            else if (attributeName == "Cool")
            {
                m_saveData.m_playerCoolAttribute = attributeLevel;
            }
        }
    }

    RED4ext::ItemID GetItemIDFromClassMap(const redRTTI::RTTIValue& aItemId)
    {
        RED4ext::ItemID id{};
        // HACK
        if (aItemId.AsClass().IsEmpty())
        {
            return id;
        }

        id.tdbid = aItemId["id"].AsTweakDBID();
        id.rngSeed = aItemId["rngSeed"].AsUint();
        id.uniqueCounter = aItemId["uniqueCounter"].AsUshort();
        id.flags = aItemId["flags"].AsUbyte();

        return id;
    }

    void LoadEquipmentSystemPlayerData(const redRTTI::RTTIValue& aEquipmentSystemPlayerData)
    {
        // Get rid of the behavioral imprint
        auto tweakDb = RED4ext::TweakDB::Get();

        auto loadoutData = aEquipmentSystemPlayerData["equipment"];

        for (auto& equipArea : loadoutData["equipAreas"].AsArray())
        {
            auto equipAreaType = equipArea["areaType"].AsCName();
            auto equipSlots = equipArea["equipSlots"].AsArray();

            if (equipAreaType == RED4ext::CName{"EyesCW"})
            {
                // Fuck the mask
                for (auto& equippedItemData : equipSlots)
                {
                    auto equippedItem = equippedItemData["itemID"];
                    auto gameItem = GetItemIDFromClassMap(equippedItem);

                    auto tweakRecord = tweakDb->GetRecord(gameItem.tdbid);

                    if (!tweakRecord)
                    {
                        continue;
                    }

                    bool isMask{};
                    Red::CName tagName = Red::CName{"MaskCW"};

                    Red::CallVirtual(tweakRecord, "TagsContains", isMask, tagName);

                    if (!isMask)
                    {
                        m_saveData.m_playerEquippedKiroshis = gameItem;
                        break;
                    }
                }
            }
            else if (equipAreaType == "SystemReplacementCW")
            {
                for (auto& slot : equipSlots)
                {
                    auto equippedItem = slot["itemID"];
                    m_saveData.m_playerEquippedOperatingSystem = GetItemIDFromClassMap(equippedItem);

                    if (m_saveData.m_playerEquippedOperatingSystem.IsValid())
                    {
                        break;
                    }
                }
            }
            else if (equipAreaType == "ArmsCW")
            {
                for (auto& slot : equipSlots)
                {
                    auto equippedItem = slot["itemID"];
                    m_saveData.m_playerEquippedArmCyberware = GetItemIDFromClassMap(equippedItem);

                    if (m_saveData.m_playerEquippedArmCyberware.IsValid())
                    {
                        break;
                    }
                }
            }
            else if (equipAreaType == "LegsCW")
            {
                for (auto& slot : equipSlots)
                {
                    auto equippedItem = slot["itemID"];
                    m_saveData.m_playerEquippedLegCyberware = GetItemIDFromClassMap(equippedItem);

                    if (m_saveData.m_playerEquippedLegCyberware.IsValid())
                    {
                        break;
                    }
                }
            }
        }
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
        static constexpr auto bannedTdbIds = std::array<RED4ext::TweakDBID, 32>{
            RED4ext::TweakDBID{"Items.MaskCW"}, RED4ext::TweakDBID{"Items.MaskCWPlus"},
            RED4ext::TweakDBID{"Items.MaskCWPlusPlus"}, RED4ext::TweakDBID{"Items.w_melee_004__fists_a"},
            RED4ext::TweakDBID{"Items.PersonalLink"}, RED4ext::TweakDBID{"Items.personal_link"},
            RED4ext::TweakDBID{"Items.PlayerMaTppHead"}, RED4ext::TweakDBID{"Items.PlayerWaTppHead"},
            RED4ext::TweakDBID{"Items.PlayerFppHead"}, RED4ext::TweakDBID{"Items.HolsteredFists"},
            RED4ext::TweakDBID{"Items.mq024_sandra_data_carrier"},
            // And Skippy leads to him talking in the starting cutscene
            RED4ext::TweakDBID{"Items.mq007_skippy"}, RED4ext::TweakDBID{"Items.mq007_skippy_post_quest"},
            RED4ext::TweakDBID{"Items.Preset_Yukimura_Skippy"},
            RED4ext::TweakDBID{"Items.Preset_Yukimura_Skippy_PostQuest"},
            RED4ext::TweakDBID{"Items.q005_saburo_data_carrier"},
            RED4ext::TweakDBID{"Items.q005_saburo_data_carrier_cracked"}, RED4ext::TweakDBID{"Items.q003_chip"},
            RED4ext::TweakDBID{"Items.q003_chip_cracked"}, RED4ext::TweakDBID{"Items.q003_chip_cracked_funds"},
            RED4ext::TweakDBID{"Items.Preset_Q001_Lexington"}};

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

        m_tweakDb = Red::TweakDB::Get();

        for (const auto& item : inventoryLocal.inventoryItems)
        {
            ProcessItem(item, m_saveData.m_playerItems);
        }

        for (const auto& item : inventoryCarStash.inventoryItems)
        {
            ProcessItem(item, m_saveData.m_playerStashItems);
        }
    }

    void LoadGarage(const redRTTI::RTTIValue& aVehicleGarage)
    {
        // Good enough
        // I'd include the V-Tech, but I actually like that one.
        static constexpr auto blacklistedVehicles =
            std::array<RED4ext::TweakDBID, 32>{RED4ext::TweakDBID{"Vehicle.v_utility4_thorton_mackinaw_bmf_player"},
            RED4ext::TweakDBID{"Vehicle.v_sport2_quadra_type66_nomad_tribute"},
            RED4ext::TweakDBID{"Vehicle.v_sportbike2_arch_jackie_player"}, RED4ext::TweakDBID{"Vehicle.v_sportbike2_arch_jackie_tuned_player"}};

        
        for (auto unlockedVehicle : aVehicleGarage["unlockedVehicleArray"].AsArray())
        {
            auto vehicleId = unlockedVehicle["vehicleID"]["recordID"].AsTweakDBID();

            if (std::find(blacklistedVehicles.begin(), blacklistedVehicles.end(), vehicleId) ==
                blacklistedVehicles.end())
            {
                m_saveData.m_playerVehicleGarage.PushBack(vehicleId);
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
    RTTI_METHOD(GetNewGamePlusState);
    RTTI_METHOD(SetNewGamePlusState);
    RTTI_METHOD(GetSaveData);
    RTTI_METHOD(SetNewGamePlusGameDefinition);
    RTTI_METHOD(IsSaveValidForNewGamePlus);
});