#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../parsing/fileReader.hpp"
#include "../filesystem/fs_util.hpp"
#include "../context/context.hpp"

#include "../parsing/definitions/nodeParsers/parserHelper.hpp"
#include "../parsing/definitions/nodeParsers/inventory/inventoryNode.hpp"
#include "../parsing/definitions/nodeParsers/persistency/persistencySystemNode.hpp"
#include "../parsing/definitions/nodeParsers/scriptable/scriptableContainerNode.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>

namespace redscript {
	struct RedItemData {
		Red::ItemID m_itemId{};
		int32_t m_itemQuantity{};

		RTTI_IMPL_TYPEINFO(RedItemData);
		RTTI_IMPL_ALLOCATOR();
	};
}

RTTI_DEFINE_CLASS(redscript::RedItemData, {
	RTTI_PROPERTY(m_itemId);
	RTTI_PROPERTY(m_itemQuantity);
});

namespace redscript {
	struct PlayerSaveData {
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

		RTTI_IMPL_TYPEINFO(PlayerSaveData);
		RTTI_IMPL_ALLOCATOR();
	};
}

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

namespace redscript {
	class TopSecretSystem : public Red::IGameSystem {
	public:
		bool IsAttached() const {
			return m_isAttached;
		}

		PlayerSaveData GetSaveData() const {
			return m_saveData;
		}

		bool GetSecretVariableState() const {
			return pluginContext::m_isSecretOverrideActivated;
		}

		void SetSecretVariableState(bool newState) {
			pluginContext::m_isSecretOverrideActivated = newState;
		}

		bool HasPointOfNoReturnSave() {
			return !files::GetLatestPointOfNoReturnSave().empty();
		}

		bool ParsePointOfNoReturnSaveData() {
			// Invalidate and reset
			m_saveData = PlayerSaveData{};
			const auto basePath = files::GetLatestPointOfNoReturnSave();

			if (basePath.empty()) {
				return false;
			}

			parser::Parser parser{};

			const auto wasParseSuccessful = parser.parseSavegame(basePath / L"sav.dat");

			if (!wasParseSuccessful) {
				return false;
			}

			const auto inventoryNode = static_cast<cyberpunk::InventoryNode*>(parser.LookupNode(cyberpunk::InventoryNode::nodeName)->nodeData.get());
			const auto persistencySystemNode = static_cast<cyberpunk::PersistencySystemNode*>(parser.LookupNode(cyberpunk::PersistencySystemNode::nodeName)->nodeData.get());
			const auto scriptableSystemsContainerNode = static_cast<cyberpunk::ScriptableSystemsContainerNode*>(parser.LookupNode(cyberpunk::ScriptableSystemsContainerNode::nodeName)->nodeData.get());

			const auto& equipmentSystemPlayerData = scriptableSystemsContainerNode->LookupChunk("EquipmentSystemPlayerData").m_redClass;
			const auto& playerDevelopmentData = scriptableSystemsContainerNode->LookupChunk("PlayerDevelopmentData").m_redClass;
			const auto& vehicleGarageData = persistencySystemNode->LookupChunk("vehicleGarageComponentPS").m_redClass;

			LoadPlayerDevelopmentData(playerDevelopmentData);
			LoadEquipmentSystemPlayerData(equipmentSystemPlayerData);
			LoadInventory(inventoryNode);
			LoadGarage(vehicleGarageData);

			m_saveData.m_isValid = true;
			return true;
		}
	private:
		void OnWorldAttached(Red::world::RuntimeScene* aScene) {
			m_isAttached = true;
		}

		void OnWorldDetached(Red::world::RuntimeScene* aScene) {
			m_isAttached = false;
		}

		// CRINGE (AND SLOW) CODE AHEAD (It's actually not too slow in the release configuration)
		// This is the result of not using proper RTTI classes...
		void LoadPlayerDevelopmentData(const redRTTI::RedValueWrapper& aPlayerDevelopmentData) {
			auto root = std::any_cast<redRTTI::RedClassMap>(aPlayerDevelopmentData.m_value);

			// Load unspent points? Kinda useless, TBH (Also completely broken with progression builds)
			// Maybe useful for Relic? Or wraparound into perk points for attribute points that can't be spent anymore
			for (auto& devPointData : std::any_cast<std::vector<redRTTI::RedValueWrapper>>(root.at("devPoints").m_value)) {
				auto developmentPointData = std::any_cast<redRTTI::RedClassMap>(devPointData.m_value);
				const auto pointType = std::any_cast<RED4ext::CName>(developmentPointData.at("type")());
				const auto unspentPointCount = std::any_cast<std::int32_t>(developmentPointData.at("unspent")());

				if (pointType == RED4ext::CName{ "Espionage" }) {
					m_saveData.m_playerRelicPoints = unspentPointCount + std::any_cast<std::int32_t>(developmentPointData.at("spent").m_value);

					// 3 milestone perks - 9 points
					// 4 cyberarm perks - 4 points
					// 1 additional perk for stealth
					// 1 additional perk for weakspots
					constexpr auto MAX_RELIC_POINTS = 15;

					const auto relicPointSpillover = m_saveData.m_playerRelicPoints - MAX_RELIC_POINTS;

					if (relicPointSpillover > 0) {
						m_saveData.m_playerPerkPoints += relicPointSpillover;
						m_saveData.m_playerRelicPoints = MAX_RELIC_POINTS;
					}
				}
				else if (pointType == RED4ext::CName{ "Primary" }) { // Perks
					m_saveData.m_playerPerkPoints += unspentPointCount; // Fill the unspent points
				}
			}

			auto perkPointsSpent = 0;

			for (auto& attributePerkData : std::any_cast<std::vector<redRTTI::RedValueWrapper>>(root.at("attributesData").m_value)) {
				auto attributePerkDataClass = std::any_cast<redRTTI::RedClassMap>(attributePerkData.m_value);
				auto unlockedPerks = std::any_cast<std::vector<redRTTI::RedValueWrapper>>(
					attributePerkDataClass.at("unlockedPerks").m_value
				);

				if (std::any_cast<RED4ext::CName>(attributePerkDataClass.at("type").m_value) == RED4ext::CName{ "EspionageAttributeData" }) {
					// We already process Espionage
					continue;
				}

				for (auto& perkData : unlockedPerks) {
					auto perk = std::any_cast<redRTTI::RedClassMap>(perkData.m_value);
					perkPointsSpent += std::any_cast<std::int32_t>(perk.at("currLevel").m_value);
				}
			}

			m_saveData.m_playerPerkPoints += perkPointsSpent;

			for (auto& proficiency : std::any_cast<std::vector<redRTTI::RedValueWrapper>>(root.at("proficiencies").m_value)) {
				auto proficiencyData = std::any_cast<redRTTI::RedClassMap>(proficiency.m_value);

				auto proficiencyLevel = std::any_cast<std::int32_t>(proficiencyData.at("currentLevel").m_value);
				auto proficiencyType = std::any_cast<RED4ext::CName>(proficiencyData.at("type").m_value);

				if (proficiencyType == RED4ext::CName{ "Level" }) {
					m_saveData.m_playerLevel = std::clamp(proficiencyLevel, 1, 50);
				}
				else if (proficiencyType == RED4ext::CName{ "StreetCred" }) {
					m_saveData.m_playerStreetCred = proficiencyLevel;
				}
				else if (proficiencyType == RED4ext::CName{ "StrengthSkill" }) {
					m_saveData.m_playerBodySkillLevel = proficiencyLevel;
				}
				else if (proficiencyType == RED4ext::CName{ "ReflexesSkill" }) {
					m_saveData.m_playerReflexSkillLevel = proficiencyLevel;
				}
				else if (proficiencyType == RED4ext::CName{ "TechnicalAbilitySkill" }) {
					m_saveData.m_playerTechSkillLevel = proficiencyLevel;
				}
				else if (proficiencyType == RED4ext::CName{ "IntelligenceSkill" }) {
					m_saveData.m_playerIntelligenceSkillLevel = proficiencyLevel;
				}
				else if (proficiencyType == RED4ext::CName{ "CoolSkill" }) {
					m_saveData.m_playerCoolSkillLevel = proficiencyLevel;
				}
			}

			for (auto& attribute : std::any_cast<std::vector<redRTTI::RedValueWrapper>>(root.at("attributes").m_value)) {
				auto attributeData = std::any_cast<redRTTI::RedClassMap>(attribute.m_value);

				auto attributeLevel = std::any_cast<std::int32_t>(attributeData.at("value").m_value);
				auto attributeName = std::any_cast<RED4ext::CName>(attributeData.at("attributeName").m_value);

				if (attributeName == RED4ext::CName{ "Strength" }) {
					m_saveData.m_playerBodyAttribute = attributeLevel;
				}
				else if (attributeName == RED4ext::CName{ "Reflexes" }) {
					m_saveData.m_playerReflexAttribute = attributeLevel;
				}
				else if (attributeName == RED4ext::CName{ "TechnicalAbility" }) {
					m_saveData.m_playerTechAttribute = attributeLevel;
				}
				else if (attributeName == RED4ext::CName{ "Intelligence" }) {
					m_saveData.m_playerIntelligenceAttribute = attributeLevel;
				}
				else if (attributeName == RED4ext::CName{ "Cool" }) {
					m_saveData.m_playerCoolAttribute = attributeLevel;
				}
			}
		}

		RED4ext::ItemID GetItemIDFromClassMap(const redRTTI::RedClassMap& aClassMap) {
			RED4ext::ItemID id{};
			// HACK
			if (aClassMap.empty()) {
				return id;
			}

			id.tdbid = std::any_cast<RED4ext::TweakDBID>(aClassMap.at("id")());
			id.rngSeed = std::any_cast<std::uint32_t>(aClassMap.at("rngSeed")());
			id.uniqueCounter = std::any_cast<std::uint16_t>(aClassMap.at("uniqueCounter")());
			id.flags = std::any_cast<std::uint8_t>(aClassMap.at("flags")());

			return id;
		}

		void LoadEquipmentSystemPlayerData(const redRTTI::RedValueWrapper& aEquipmentSystemPlayerData) {
			// Get rid of the behavioral imprint
			auto tweakDb = RED4ext::TweakDB::Get();

			auto root = std::any_cast<redRTTI::RedClassMap>(aEquipmentSystemPlayerData());
			auto loadoutData = std::any_cast<redRTTI::RedClassMap>(root.at("equipment")());

			for (auto& equipArea : std::any_cast<std::vector<redRTTI::RedValueWrapper>>(loadoutData.at("equipAreas")())) {
				auto equipAreaData = std::any_cast<redRTTI::RedClassMap>(equipArea());
				auto equipAreaType = std::any_cast<RED4ext::CName>(equipAreaData.at("areaType")());
				auto equipSlots = std::any_cast<std::vector<redRTTI::RedValueWrapper>>(equipAreaData.at("equipSlots")());

				if (equipAreaType == RED4ext::CName{ "EyesCW" }) {
					// Fuck the mask
					for (auto& equippedItemData : equipSlots) {
						auto equippedItem = std::any_cast<redRTTI::RedClassMap>(
							std::any_cast<redRTTI::RedClassMap>(equippedItemData()).at("itemID")()
						);
						auto gameItem = GetItemIDFromClassMap(std::any_cast<redRTTI::RedClassMap>(equippedItem));

						auto tweakRecord = tweakDb->GetRecord(gameItem.tdbid);

						if (!tweakRecord) {
							continue;
						}

						bool isMask{};
						Red::CName tagName = Red::CName{ "MaskCW" };

						Red::CallVirtual(tweakRecord, "TagsContains", isMask, tagName);

						if (!isMask) {
							m_saveData.m_playerEquippedKiroshis = gameItem;
							break;
						}
					}
				}
				else if (equipAreaType == RED4ext::CName{ "SystemReplacementCW" }) {
					if (equipSlots.size() > 0) {
						auto equippedItem = std::any_cast<redRTTI::RedClassMap>(
							std::any_cast<redRTTI::RedClassMap>(equipSlots.at(0)()).at("itemID")()
						);

						m_saveData.m_playerEquippedOperatingSystem = GetItemIDFromClassMap(equippedItem);
					}
				}
				else if (equipAreaType == RED4ext::CName{ "ArmsCW" }) {
					if (equipSlots.size() > 0) {
						auto equippedItem = std::any_cast<redRTTI::RedClassMap>(
							std::any_cast<redRTTI::RedClassMap>(equipSlots.at(0)()).at("itemID")()
						);

						m_saveData.m_playerEquippedArmCyberware = GetItemIDFromClassMap(equippedItem);
					}
				}
				else if (equipAreaType == RED4ext::CName{ "LegsCW" }) {
					if (equipSlots.size() > 0) {
						auto equippedItem = std::any_cast<redRTTI::RedClassMap>(
							std::any_cast<redRTTI::RedClassMap>(equipSlots.at(0)()).at("itemID")()
						);

						m_saveData.m_playerEquippedLegCyberware = GetItemIDFromClassMap(equippedItem);
					}
				}
			}
		}

		bool ShouldAddItemToInventory(RED4ext::TweakDB* aTweakDb, const RED4ext::ItemID& aItemId) {
			// Sorry, but these REALLY annoy me
			static constexpr auto bannedTdbIds = std::array<RED4ext::TweakDBID, 32>{
				RED4ext::TweakDBID{ "Items.MaskCW" },
					RED4ext::TweakDBID{ "Items.MaskCWPlus" },
					RED4ext::TweakDBID{ "Items.MaskCWPlusPlus" },
					RED4ext::TweakDBID{ "Items.w_melee_004__fists_a" },
					RED4ext::TweakDBID{ "Items.PersonalLink" },
					RED4ext::TweakDBID{ "Items.personal_link" },
					RED4ext::TweakDBID{ "Items.PlayerMaTppHead" },
					RED4ext::TweakDBID{ "Items.PlayerWaTppHead" },
					RED4ext::TweakDBID{ "Items.PlayerFppHead" },
					RED4ext::TweakDBID{ "Items.HolsteredFists" },
					RED4ext::TweakDBID{ "Items.mq024_sandra_data_carrier" },
			};

			if (std::find(bannedTdbIds.begin(), bannedTdbIds.end(), aItemId.tdbid) != bannedTdbIds.end()) {
				return false;
			}

			auto itemRecord = aTweakDb->GetRecord(aItemId.tdbid);

			if (!itemRecord) {
				return false;
			}

			Red::Handle<Red::gamedataItemType_Record> itemType;
			Red::CallVirtual(itemRecord, "ItemTypeHandle", itemType);

			if (!itemType) {
				return false;
			}

			Red::gamedataItemType type{};
			Red::CallVirtual(itemType, "Type", type);

			using Red::gamedataItemType;
			switch (type) {
			case gamedataItemType::Con_Edible:
			case gamedataItemType::CyberwareStatsShard:
			case gamedataItemType::Gen_Jewellery:
			case gamedataItemType::Gen_Junk:
			case gamedataItemType::Gen_Keycard:
			case gamedataItemType::Gen_MoneyShard:
			case gamedataItemType::Gen_Readable:
			case gamedataItemType::Gen_Tarot:
			case gamedataItemType::VendorToken:
			case gamedataItemType::Wea_Fists:
			case gamedataItemType::Wea_VehicleMissileLauncher:
			case gamedataItemType::Wea_VehiclePowerWeapon:
				return false;
			}

			return true;
		}

		// This is really dumb code IMO
		// But if it works...
		bool IsItemMod(RED4ext::TweakDB* aTweakDb, RED4ext::TweakDBID aTdbId) {
			auto itemRecord = aTweakDb->GetRecord(aTdbId);

			if (!itemRecord) {
				return false;
			}

			Red::Handle<Red::gamedataItemCategory_Record> itemCategoryHandle;
			Red::CallVirtual(itemRecord, "ItemCategoryHandle", itemCategoryHandle);

			if (!itemCategoryHandle) {
				return false;
			}

			Red::CName name;
			Red::CallVirtual(itemCategoryHandle, "Name", name);

			return name == Red::CName{ "WeaponMod" };
		}

		bool IsItemIconic(RED4ext::TweakDB* aTweakDb, RED4ext::TweakDBID aTdbId) {
			auto itemRecord = aTweakDb->GetRecord(aTdbId);

			if (!itemRecord) {
				return false;
			}

			bool isIconic{};
			Red::CName iconicTag = "IconicWeapon";

			Red::CallVirtual(itemRecord, "TagsContains", isIconic, iconicTag);

			return isIconic;
		}

		void ParseAttachments(RED4ext::TweakDB* aTweakDb, const cyberpunk::ItemSlotPart& aItemSlotPart, bool aIsCarStash) {
			if (IsItemMod(aTweakDb, aItemSlotPart.itemInfo.itemId.tdbid)) {
				RedItemData itemData{};
				itemData.m_itemId = aItemSlotPart.itemInfo.itemId;
				itemData.m_itemQuantity = 1;

				if (!aIsCarStash) {
					m_saveData.m_playerItems.PushBack(itemData);
				}
				else {
					m_saveData.m_playerStashItems.PushBack(itemData);
				}
			}

			for (const auto& childItem : aItemSlotPart.children) {
				ParseAttachments(aTweakDb, childItem, aIsCarStash);
			}
		}

		void ParseItem(RED4ext::TweakDB* aTweakDb, const cyberpunk::ItemData& aItemData, bool aIsCarStash) {
			auto& itemInfo = aItemData.itemInfo;
			if (!ShouldAddItemToInventory(aTweakDb, itemInfo.itemId)) {
				return;
			}

			static constexpr auto moneyItemTDBID = Red::TweakDBID{ "Items.money" };

			if (aItemData.itemInfo.itemId.tdbid == moneyItemTDBID) {
				m_saveData.m_playerMoney += aItemData.itemQuantity;
				return;
			}

			RedItemData itemData{};

			itemData.m_itemId = itemInfo.itemId;
			itemData.m_itemQuantity = aItemData.hasQuantity() ? aItemData.itemQuantity : 1;

			if (!aIsCarStash) {
				m_saveData.m_playerItems.PushBack(itemData);
			}
			else {
				m_saveData.m_playerStashItems.PushBack(itemData);
			}

			if (!aItemData.hasExtendedData()) {
				return;
			}

			// Iconics don't get mods anymore :(
			if (IsItemIconic(aTweakDb, itemInfo.itemId.tdbid)) {
				return;
			}

			const auto& itemSlotPart = aItemData.itemSlotPart;

			ParseAttachments(aTweakDb, itemSlotPart, aIsCarStash);
		}

		void LoadInventory(cyberpunk::InventoryNode* const aInventory) {
			auto tweakDb = RED4ext::TweakDB::Get();

			auto& inventoryLocal = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdLocal);
			auto& inventoryCarStash = aInventory->LookupInventory(cyberpunk::SubInventory::inventoryIdCarStash);

			for (auto item : inventoryLocal.inventoryItems) {
				ParseItem(tweakDb, item, false);
			}

			for (auto item : inventoryCarStash.inventoryItems) {
				ParseItem(tweakDb, item, true);
			}
		}

		void LoadGarage(const redRTTI::persistent::RedValueWrapper& aClassMap) {
			// Good enough
			// I'd include the V-Tech, but I actually like that one.
			static constexpr auto blacklistedVehicles = std::array<RED4ext::TweakDBID, 32>{
				RED4ext::TweakDBID{ "Vehicle.v_utility4_thorton_mackinaw_bmf_player" },
					RED4ext::TweakDBID{ "Vehicle.v_sport2_quadra_type66_nomad_tribute" }
			};

			auto root = std::any_cast<redRTTI::persistent::RedClassMap>(aClassMap());
			// Really need a typedef for arrays
			// Some sort of .at_as<>(std::string_view aProperty) method for class maps as well
			// But that's a lot of refactoring....
			for (auto unlockedVehicle : std::any_cast<std::vector<redRTTI::persistent::RedValueWrapper>>(root.at("unlockedVehicleArray")())) {
				auto unlockedVehicleData = std::any_cast<redRTTI::persistent::RedClassMap>(unlockedVehicle());
				auto vehicleIdData = std::any_cast<redRTTI::persistent::RedClassMap>(unlockedVehicleData.at("vehicleID")());
				auto vehicleRecordId = std::any_cast<RED4ext::TweakDBID>(vehicleIdData.at("recordID")());

				if (std::find(blacklistedVehicles.begin(), blacklistedVehicles.end(), vehicleRecordId) == blacklistedVehicles.end()) {
					m_saveData.m_playerVehicleGarage.PushBack(vehicleRecordId);
				}
			}
		}

		bool m_isAttached{};
		PlayerSaveData m_saveData{};

		RTTI_IMPL_TYPEINFO(TopSecretSystem);
		RTTI_IMPL_ALLOCATOR();
	};
}

RTTI_DEFINE_CLASS(redscript::TopSecretSystem, {
	RTTI_METHOD(IsAttached);
	RTTI_METHOD(ParsePointOfNoReturnSaveData);
	RTTI_METHOD(HasPointOfNoReturnSave);
	RTTI_METHOD(GetSecretVariableState);
	RTTI_METHOD(SetSecretVariableState);
	RTTI_METHOD(GetSaveData);
});