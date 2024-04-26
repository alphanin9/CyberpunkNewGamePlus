#pragma once
#include <cassert>
#include <vector>
#include <RED4ext/RED4ext.hpp>

#include "../interfaceNodeData.hpp"
#include "helpers/persistencyRttiClassCreator.hpp"

namespace cyberpunk {
	struct UnknownRedBuffer {
		std::uint64_t m_id;
		RED4ext::CName m_className;
        redRTTI::RTTIValue m_redClass;
	};

	class PersistencySystemNode : public NodeDataInterface {
		std::vector<std::uint32_t> m_ids;
		std::uint32_t m_unk1;
		std::vector<UnknownRedBuffer> m_redClasses;
	

	public:
		static constexpr std::wstring_view nodeName = L"PersistencySystem2";
		virtual void readData(FileCursor& aCursor, NodeEntry& node) {
			RED4EXT_UNUSED_PARAMETER(node);
			const auto rtti = RED4ext::CRTTISystem::Get();
			const auto idCount = aCursor.readInt();
			m_ids.reserve(idCount);
			for (auto i = 0; i < idCount; i++) {
				m_ids.push_back(aCursor.readUInt());
			}
			m_unk1 = aCursor.readUInt();
			const auto itemCount = aCursor.readInt();

			for (auto i = 0; i < itemCount; i++) {
				auto classEntry = UnknownRedBuffer{};
				classEntry.m_id = aCursor.readUInt64();

				if (classEntry.m_id) {
					const auto classHash = aCursor.readUInt64();
					const auto classSize = aCursor.readUInt();

					auto className = RED4ext::CName{ classHash };
					classEntry.m_className = className;

					auto startOffset = aCursor.offset;
					// For our purposes we only really want to parse 
					// vehicleGarageComponentPS, 
					// as we don't *really* need any other nodes
					if (!className.IsNone() && className == RED4ext::CName{ "vehicleGarageComponentPS" }) {
						auto classBytes = aCursor.readBytes(classSize);
						auto subCursor = FileCursor{ classBytes.data(), classBytes.size() };

                        persistent::PersistentReader reader{};

						redRTTI::RTTIValue wrapper{};

						wrapper.m_typeIndex = Red::ERTTIType::Class;
                        wrapper.m_typeName = className;
                        wrapper.m_value = reader.ReadClass(subCursor, rtti->GetClass(className));

						classEntry.m_redClass = wrapper;
					}

					aCursor.seekTo(FileCursor::SeekTo::Start, startOffset + classSize);
				}

				m_redClasses.push_back(classEntry);
			}

			//std::println("{} nodes in PersistencySystem", m_redClasses.size());
		}

		const UnknownRedBuffer& LookupChunk(std::string_view aChunkName) {
			auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(), [aChunkName](const UnknownRedBuffer& aBuffer) {
				return RED4ext::CName{ aChunkName.data() } == aBuffer.m_className;
			});

			if (chunkIt == m_redClasses.end())
            {
                throw std::runtime_error{std::format("Failed to find class {} in PersistencySystem2", aChunkName)};
			}

			return *chunkIt;
		}
	};
}