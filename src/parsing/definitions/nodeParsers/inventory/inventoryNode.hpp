#pragma once
#include "../interfaceNodeData.hpp"

#include <RED4ext/RED4ext.hpp>

namespace cyberpunk {
	struct ItemInfo {
		enum class ItemStructure : std::uint8_t {
			None = 0,
			Extended = 1,
			Quantity = 2
		};

		RED4ext::ItemID itemId;
		ItemStructure itemStructure;

		static ItemInfo fromCursor(FileCursor& cursor) {
			auto id = cursor.readTdbId();
			auto seed = cursor.readUInt();
			auto itemStructore = cursor.readValue<ItemStructure>();
			auto uniqueCounter = cursor.readUShort();
			auto flags = cursor.readByte();

			auto ret = ItemInfo{};

			ret.itemId.tdbid = id;
			ret.itemId.rngSeed = seed;
			ret.itemId.uniqueCounter = uniqueCounter;
			ret.itemId.flags = flags;

			ret.itemStructure = itemStructore;

			return ret;
		}

		bool operator ==(const ItemInfo& rhs) {
			if (itemStructure != rhs.itemStructure) {
				return false;
			}

			if (itemId.tdbid != rhs.itemId.tdbid) {
				return false;
			}

			if (itemId.rngSeed != rhs.itemId.rngSeed) {
				return false;
			}

			if (itemId.uniqueCounter != rhs.itemId.uniqueCounter) {
				return false;
			}

			if (itemId.flags != rhs.itemId.flags) {
				return false;
			}

			return true;
		}

		bool operator !=(const ItemInfo& rhs) {
			return !(*this == rhs);
		}
	};

	struct AdditionalItemInfo {
		bool isValid = false;

		RED4ext::TweakDBID lootItemPoolId;
		std::uint32_t unk2;
		float requiredLevel;

		static AdditionalItemInfo fromCursor(FileCursor& cursor) {
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

		ItemInfo itemInfo;
		std::wstring appearanceName;
		RED4ext::TweakDBID attachmentSlotTdbId;

		std::vector<ItemSlotPart> children;

		std::uint32_t unk2;
		AdditionalItemInfo additionalItemInfo;

		static ItemSlotPart fromCursor(FileCursor& cursor) {
			auto ret = ItemSlotPart{};

			ret.isValid = true;

			ret.itemInfo = ItemInfo::fromCursor(cursor);
			ret.appearanceName = cursor.readLengthPrefixedString();
			ret.attachmentSlotTdbId = cursor.readTdbId();

			const auto count = cursor.readVlqInt32();

			for (auto i = 0; i < count; i++) {
				ret.children.push_back(fromCursor(cursor));
			}

			ret.unk2 = cursor.readUInt();
			ret.additionalItemInfo = AdditionalItemInfo::fromCursor(cursor);

			return ret;
		}
	};

	struct ItemData {
		enum class ItemFlags : std::uint8_t {
			IsQuestItem = 1,
			IsNotUnequippable = 2
		};

		ItemInfo itemInfo;
		ItemFlags itemFlags;
		std::uint32_t creationTime;
		std::uint32_t itemQuantity = 1;
		
		AdditionalItemInfo additionalItemInfo;
		ItemSlotPart itemSlotPart;

		bool hasQuantity() const {
			return (static_cast<std::uint8_t>(itemInfo.itemStructure) & static_cast<std::uint8_t>(ItemInfo::ItemStructure::Quantity))
				== static_cast<std::uint8_t>(ItemInfo::ItemStructure::Quantity) || itemInfo.itemStructure == ItemInfo::ItemStructure::None && itemInfo.itemId.rngSeed == 2;
		}

		bool hasExtendedData() const {
			return (static_cast<std::uint8_t>(itemInfo.itemStructure) & static_cast<std::uint8_t>(ItemInfo::ItemStructure::Extended))
				== static_cast<std::uint8_t>(ItemInfo::ItemStructure::Extended) || itemInfo.itemStructure == ItemInfo::ItemStructure::None && itemInfo.itemId.rngSeed != 2;
		}
	};

	class ItemDataNode : public NodeDataInterface {
	public:
		static constexpr std::wstring_view nodeName = L"itemData";

		ItemData itemData;
		virtual void readData(FileCursor& cursor, NodeEntry& node) {
			itemData.itemInfo = ItemInfo::fromCursor(cursor);

			itemData.itemFlags = cursor.readValue<ItemData::ItemFlags>();
			itemData.creationTime = cursor.readUInt();

			if (itemData.hasQuantity()) {
				itemData.itemQuantity = cursor.readUInt();
			}

			if (itemData.hasExtendedData()) {
				itemData.additionalItemInfo = AdditionalItemInfo::fromCursor(cursor);
				itemData.itemSlotPart = ItemSlotPart::fromCursor(cursor);
			}
		}
	};

	struct SubInventory {
		std::uint64_t inventoryId;
		std::vector<ItemData> inventoryItems;
	};

	class InventoryNode : public NodeDataInterface {
	public:
		static constexpr std::wstring_view nodeName = L"inventory";
		std::vector<SubInventory> subInventories;
	private:
		SubInventory readSubInventory(FileCursor& cursor, NodeEntry& node, int offset) {
			auto ret = SubInventory{};

			ret.inventoryId = cursor.readUInt64();

			const auto count = cursor.readUInt();

			for (auto i = 0; i < count; i++) {
				auto nextItemInfo = ItemInfo::fromCursor(cursor);

				cyberpunk::parseNode(cursor, *node.nodeChildren.at(offset + i));

				auto itemInfoActual = reinterpret_cast<ItemDataNode*>(node.nodeChildren.at(offset + i)->nodeData.get());
				
				if (itemInfoActual->itemData.itemInfo != nextItemInfo) {
					std::println("Item info parsing failed!");
				}
				
				ret.inventoryItems.push_back(itemInfoActual->itemData);
			}

			return ret;
		}
	public:
		virtual void readData(FileCursor& cursor, NodeEntry& node) {
			const auto count = cursor.readInt();
			auto offset = 0;

			for (auto i = 0; i < count; i++) {
				auto subInventory = readSubInventory(cursor, node, offset);

				subInventories.push_back(subInventory);

				offset += subInventory.inventoryItems.size();
			}
		}
	};
}