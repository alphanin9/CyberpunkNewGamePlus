#pragma once

#include <cassert>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../interfaceNodeData.hpp"
#include "../parserHelper.hpp"

#include <context/context.hpp>

// TODO: Optimize this
// Make this use Red structures instead of STL for noexcept stuff?
// Seems to have a bunch of pointless copies - get rid of them later
namespace save {
namespace item
{
inline Red::ItemID ReadItemId(FileCursor& aCursor)
{
    Red::ItemID id{};

	id.tdbid = aCursor.readTdbId();
    id.rngSeed = aCursor.readUInt();
    id.structure = aCursor.readValue<std::uint8_t>();
    id.uniqueCounter = aCursor.readUShort();
    id.flags = aCursor.readByte();

	return id;
}

enum class ItemStructure : std::uint8_t
{
    None = 0,
    Extended = 1,
    Quantity = 2
};
}
	struct AdditionalItemInfo {
		bool isValid = false;

		RED4ext::TweakDBID lootItemPoolId;
		std::uint32_t unk2;
		float requiredLevel;

		static AdditionalItemInfo FromCursor(FileCursor& cursor) {
			auto ret = AdditionalItemInfo{};

			ret.isValid = true;
			ret.lootItemPoolId = cursor.readTdbId();
			ret.unk2 = cursor.readUInt();
			ret.requiredLevel = cursor.readFloat();

			return ret;
		}
	};

	struct ItemSlotPart {
		bool isValid = false;

		Red::ItemID m_itemId;
		std::string m_appearanceName;
		Red::TweakDBID m_attachmentSlotTdbId;

		std::vector<ItemSlotPart> m_children;

		std::uint32_t unk2;
		AdditionalItemInfo additionalItemInfo;

		static ItemSlotPart FromCursor(FileCursor& cursor) {
			auto ret = ItemSlotPart{};

			ret.isValid = true;

			ret.m_itemId = item::ReadItemId(cursor);
			ret.m_appearanceName = cursor.ReadLengthPrefixedANSI(); // Fix this to use ANSI later, but we don't use appearance name anyway...
			ret.m_attachmentSlotTdbId = cursor.readTdbId();

			const auto count = cursor.readVlqInt32();

			ret.m_children = cursor.ReadMultipleClasses<ItemSlotPart>(count);

			ret.unk2 = cursor.readUInt();
			ret.additionalItemInfo = AdditionalItemInfo::FromCursor(cursor);

			return ret;
		}
	};

	struct ItemData {
		enum class ItemFlags : std::uint8_t {
			IsQuestItem = 1,
			IsNotUnequippable = 2
		};

		Red::ItemID m_itemId;
		ItemFlags m_itemFlags;
		std::uint32_t m_creationTime;
		std::uint32_t m_itemQuantity = 1;
		
		AdditionalItemInfo m_additionalItemInfo;
		ItemSlotPart m_itemSlotPart;

		// Actually kinda silly, don't we have TDB?
		// I hate this
		bool HasQuantity() const {
            return (m_itemId.structure & static_cast<std::uint8_t>(item::ItemStructure::Quantity)) ==
                       static_cast<std::uint8_t>(item::ItemStructure::Quantity) ||
                   m_itemId.structure == static_cast<std::uint8_t>(item::ItemStructure::None) &&
                       m_itemId.rngSeed == 2;
		}

		bool HasExtendedData() const {
            return (m_itemId.structure & static_cast<std::uint8_t>(item::ItemStructure::Extended)) ==
                       static_cast<std::uint8_t>(item::ItemStructure::Extended) ||
                   m_itemId.structure == static_cast<std::uint8_t>(item::ItemStructure::None) &&
                       m_itemId.rngSeed != 2;
		}

		std::uint32_t GetQuantity() const
        {
            if (HasQuantity())
            {
                return m_itemQuantity;
			}

			return 1;
		}

		Red::TweakDBID GetRecordID() const
        {
            return m_itemId.tdbid;
		}

		const Red::ItemID& GetItemID() const
        {
            return m_itemId;
		}
	};

	class ItemDataNode : public NodeDataInterface {
	public:
		static constexpr Red::CName m_nodeName = "itemData";

		ItemData itemData;
		virtual void ReadData(FileCursor& cursor, NodeEntry& node) {
            itemData.m_itemId = item::ReadItemId(cursor);

			itemData.m_itemFlags = cursor.readValue<ItemData::ItemFlags>();
			itemData.m_creationTime = cursor.readUInt();

			if (itemData.HasQuantity()) {
				itemData.m_itemQuantity = cursor.readUInt();
			}

			if (itemData.HasExtendedData()) {
				itemData.m_additionalItemInfo = AdditionalItemInfo::FromCursor(cursor);
				itemData.m_itemSlotPart = ItemSlotPart::FromCursor(cursor);
			}
		}
	};

	struct SubInventory {
		static constexpr auto inventoryIdLocal = 0x1;
		static constexpr auto inventoryIdCarStash = 0xF4240;

		// This is actually EntityId or NodeRef
		Red::NodeRef m_inventoryId;
		std::vector<ItemData> m_inventoryItems;
	};

	class InventoryNode : public NodeDataInterface {
	public:
		static constexpr Red::CName m_nodeName = "inventory";
		std::vector<SubInventory> subInventories;
	private:
		SubInventory ReadSubInventory(FileCursor& cursor, NodeEntry& node, int offset) {
			auto ret = SubInventory{};

			ret.m_inventoryId = cursor.readUInt64();

			const auto count = cursor.readUInt();

			ret.m_inventoryItems.reserve(count);

			for (auto i = 0u; i < count; i++) {
				auto nextItemInfo = item::ReadItemId(cursor);

				// Not a big fan of this... But it has to be done
				save::ParseNode(cursor, *node.nodeChildren.at(offset + i));
				node.nodeChildren.at(offset + i)->isReadByParent = true;

				auto itemInfoActual = reinterpret_cast<ItemDataNode*>(node.nodeChildren.at(offset + i)->nodeData.get());
				
				// Relatively big copy
				ret.m_inventoryItems.push_back(std::move(itemInfoActual->itemData));
			}

			return ret;
		}
	public:
		virtual void ReadData(FileCursor& cursor, NodeEntry& node) {
			const auto count = cursor.readInt();
			auto offset = 0;

			for (auto i = 0; i < count; i++) {
				auto subInventory = ReadSubInventory(cursor, node, offset);

				auto inventorySize = static_cast<int>(subInventory.m_inventoryItems.size());

				subInventories.push_back(std::move(subInventory));

				offset += inventorySize;
			}
		}

		const SubInventory& LookupInventory(std::uint64_t aInventoryId) {
			auto inventoryIt = std::find_if(subInventories.begin(), subInventories.end(), [aInventoryId](const SubInventory& aSubInventory) {
				return aSubInventory.m_inventoryId == aInventoryId;
			});

			// Remove assert later
			assert(inventoryIt != subInventories.end());

			return *inventoryIt;
		}
	};
}