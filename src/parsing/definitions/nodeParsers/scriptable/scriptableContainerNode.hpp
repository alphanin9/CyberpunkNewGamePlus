#pragma once

// Parsing scriptable systems seems too difficult to be worth it to do it in native code
// So into the trash it goes (for now)

#include <stdexcept>
#include <vector>
#include <RED4ext/RED4ext.hpp>

#include "../defaultNodeData.hpp"

namespace cyberpunk {
	struct RedPackageHeader {
		std::byte version;
		std::byte unk1;
		std::uint16_t numSections;
		std::uint32_t numComponents;
		std::uint32_t refPoolDescOffset;
		std::uint32_t refPoolDataOffset;
		std::uint32_t namePoolDescOffset;
		std::uint32_t namePoolDataOffset;
		std::uint32_t chunkDescOffset;
		std::uint32_t chunkDataOffset;
	};

	class ScriptableSystemsContainerNode : public NodeDataInterface {
	public:
		static constexpr std::wstring_view nodeName = L"ScriptableSystemsContainer";

		RedPackageHeader header;
		std::vector<RED4ext::CRUID> rootCruids;
		
		virtual void readData(FileCursor& cursor, NodeEntry& node) {
			throw std::logic_error{ "Not yet implemented!" };

			const auto dataSize = cursor.readInt();
			auto dataBuffer = cursor.readBytes(dataSize);

			auto innerCursor = FileCursor{ dataBuffer.data(), dataBuffer.size() };

			if (dataSize == 0) {
				return;
			}

			header.version = static_cast<std::byte>(innerCursor.readByte());
			header.unk1 = static_cast<std::byte>(innerCursor.readByte());

			header.numSections = innerCursor.readUShort();
			header.numComponents = innerCursor.readUInt();

			if (header.numSections == 7) {
				header.refPoolDescOffset = innerCursor.readUInt();
				header.refPoolDataOffset = innerCursor.readUInt();
			}

			header.namePoolDescOffset = innerCursor.readUInt();
			header.namePoolDataOffset = innerCursor.readUInt();

			header.chunkDescOffset = innerCursor.readUInt();
			header.chunkDataOffset = innerCursor.readUInt();

			const auto numCruids = innerCursor.readUInt();

			for (auto i = 0; i < numCruids; i++) {
				rootCruids.push_back(innerCursor.readCruid());
			}


		}
	};
}