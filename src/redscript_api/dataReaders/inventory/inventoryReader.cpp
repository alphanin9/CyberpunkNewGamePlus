#include "inventoryReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/game/data/ItemCategory_Record.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/data/ItemType_Record.hpp>

#include "../resultContext.hpp"

#include <../package/packageReader.hpp>
#include <inventory/inventoryNode.hpp>

#include <unordered_set>

using namespace Red;
using game::data::ItemType;

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

namespace InventoryReader
{
// ItemID with a few additions to simplify things...
struct ExtendedItemData
{
    ItemID m_itemId;
    DynArray<CName>* m_tags;

    ItemType m_itemType;

    static constexpr CName HideInUI = "HideInUI";
    static constexpr CName HideAtVendor = "HideAtVendor";
    static constexpr CName WeaponMod = "WeaponMod";
    static constexpr CName LexingtonWilson = "Lexington_Wilson";
    static constexpr CName DLCStashItem = "DLCStashItem";
    static constexpr CName IconicWeapon = "IconicWeapon";

    bool IsHiddenInUI() const
    {
        return HasTag(HideInUI) || HasTag(HideAtVendor);
    }

    bool IsValidAttachment() const
    {
        return HasTag(WeaponMod) || m_itemType == ItemType::Prt_Program;
    }

    bool IsDyingNight() const
    {
        return HasTag(LexingtonWilson);
    }

    bool IsDLCStashItem() const
    {
        return HasTag(DLCStashItem);
    }

    bool IsIconicWeapon() const
    {
        return HasTag(IconicWeapon);
    }

    bool IsAllowedType() const
    {
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

    bool HasTag(CName aTag) const
    {
        if (!m_tags)
        {
            return false;
        }

        return m_tags->Contains(aTag);
    }

    TweakDBID GetTDBID() const
    {
        return m_itemId.tdbid; 
    }

    ExtendedItemData(const ItemID& aId, ResultContext& aContext)
    {
        m_itemId = aId;
        m_itemType = ItemType::Invalid;

        if (auto tagValuePtr = aContext.m_tweakDB->GetFlatValue(TweakDBID(GetTDBID(), ".tags")))
        {
            m_tags = tagValuePtr->GetValue<DynArray<CName>>();
        }

        if (auto typeValuePtr = aContext.m_tweakDB->GetFlatValue(TweakDBID(GetTDBID(), ".itemType")))
        {
            auto typeFk = *typeValuePtr->GetValue<TweakDBID>();

            if (auto nameValuePtr = aContext.m_tweakDB->GetFlatValue(TweakDBID(typeFk, ".name")))
            {
                static const auto typeEnum = GetEnum<ItemType>();

                std::int64_t itemType{};
                package::Package::ResolveEnumValue(typeEnum, *nameValuePtr->GetValue<CName>(), itemType);

                m_itemType = static_cast<ItemType>(itemType);
            }
        }
    }
};

void ProcessStatModifiers(Handle<NGPlusItemData>& aItemData, ResultContext& aContext) noexcept
{
    const auto statsObjectId = aContext.m_statsSystem->GetEntityHashFromItemId(aItemData->m_itemId);
    auto modifiers = aContext.m_statsSystem->GetStatModifiers(statsObjectId);

    aItemData->m_statModifiers.Reserve(modifiers.size);

    for (auto& modifier : modifiers)
    {
        if (!modifier)
        {
            continue;
        }

        if (modifier->statType == game::data::StatType::Invalid)
        {
            continue;
        }

        aItemData->m_statModifiers.PushBack(std::move(modifier));
    }
}

void ProcessAttachments(const save::ItemSlotPart& aSlotPart, Handle<NGPlusItemData>* aRootItemData,
                        DynArray<Handle<NGPlusItemData>>& aTargetList, ResultContext& aContext)
{
    constexpr TweakDBID iconicWeaponModLegendary = "AttachmentSlots.IconicWeaponModLegendary";
    constexpr TweakDBID iconicMeleeWeaponMod1 = "AttachmentSlots.IconicMeleeWeaponMod1";
    constexpr TweakDBID statsShardSlot = "AttachmentSlots.StatsShardSlot";

    // Don't bother doing iconic weapon mods
    if (aSlotPart.m_attachmentSlotTdbId == iconicWeaponModLegendary ||
        aSlotPart.m_attachmentSlotTdbId == iconicMeleeWeaponMod1)
    {
        return;
    }

    // CW stat shards are stored here
    if (aSlotPart.m_attachmentSlotTdbId == statsShardSlot && aRootItemData)
    {
        (*aRootItemData)->m_attachments.PushBack(aSlotPart.m_itemId);
    }

    ExtendedItemData attachmentData(aSlotPart.m_itemId, aContext);

    if (attachmentData.IsAllowedType() && attachmentData.IsValidAttachment() && !attachmentData.IsHiddenInUI())
    {
        auto attachmentItemData = MakeHandle<NGPlusItemData>();

        attachmentItemData->m_itemId = aSlotPart.m_itemId;
        attachmentItemData->m_itemQuantity = 1;

        ProcessStatModifiers(attachmentItemData, aContext);

        aTargetList.PushBack(std::move(attachmentItemData));
    }

    for (const auto& child : aSlotPart.m_children)
    {
        ProcessAttachments(child, aRootItemData, aTargetList, aContext);
    }
}

void ProcessItem(const save::ItemData& aItem, DynArray<Handle<NGPlusItemData>>& aTargetList,
                 std::unordered_set<TweakDBID>& aAddedIconics, ResultContext& aContext)
{
    if (BlacklistedTDBIDs::IsForbidden(aItem.GetRecordID()))
    {
        return;
    }

    ExtendedItemData data(aItem.GetItemID(), aContext);

    if (!data.IsAllowedType() || data.IsDyingNight() || data.IsDLCStashItem() || data.IsHiddenInUI())
    {
        return;
    }

    if (data.IsIconicWeapon())
    {
        if (aAddedIconics.contains(data.GetTDBID()))
        {
            if (aItem.HasExtendedData())
            {
                // Save attachments on X-MOD2 weapons, despite them being iconic and possibly dupes
                ProcessAttachments(aItem.m_itemSlotPart, nullptr, aTargetList, aContext);
            }

            return;
        }

        aAddedIconics.insert(data.GetTDBID());
    }

    auto itemData = MakeHandle<NGPlusItemData>();

    itemData->m_itemId = aItem.GetItemID();
    itemData->m_itemQuantity = aItem.HasQuantity() ? std::max(1u, aItem.m_itemQuantity) : 1;

    ProcessStatModifiers(itemData, aContext);

    if (aItem.HasExtendedData())
    {
        ProcessAttachments(aItem.m_itemSlotPart, &itemData, aTargetList, aContext);
    }

    aTargetList.PushBack(std::move(itemData));
}

} // namespace InventoryReader

InventoryReader::InventoryReaderResults::InventoryReaderResults(save::InventoryNode& aInventory,
                                                                ResultContext& aContext) noexcept
{
    auto& localInventory = aInventory.LookupInventory(save::SubInventory::inventoryIdLocal);
    auto& stashInventory = aInventory.LookupInventory(save::SubInventory::inventoryIdCarStash);

    m_inventory.Reserve(static_cast<std::uint32_t>(localInventory.m_inventoryItems.size()));
    m_stash.Reserve(static_cast<std::uint32_t>(stashInventory.m_inventoryItems.size()));

    std::unordered_set<TweakDBID> addedIconics{};

    for (const auto& item : localInventory.m_inventoryItems)
    {
        // Money can't be in stash I think
        constexpr TweakDBID c_money = "Items.money";

        if (item.GetRecordID() == c_money)
        {
            m_money += item.GetQuantity();
            continue;
        }

        ProcessItem(item, m_inventory, addedIconics, aContext);
    }

    for (const auto& item : stashInventory.m_inventoryItems)
    {
        ProcessItem(item, m_stash, addedIconics, aContext);
    }
}