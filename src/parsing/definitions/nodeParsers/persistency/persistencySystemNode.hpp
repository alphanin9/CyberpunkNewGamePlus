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
		redRTTI::persistent::RedValueWrapper m_redClass;
	};

	class PersistencySystemNode : public NodeDataInterface {
		std::vector<std::uint32_t> m_ids;
		std::uint32_t m_unk1;
		std::vector<UnknownRedBuffer> m_redClasses;

		void printProperty(std::ofstream& outputDump, RED4ext::CRTTISystem* aRttiSystem, RED4ext::CProperty* aProperty, bool aPersistentOnly, int aIndentation = 0, int aDepth = 0) {
			if (aPersistentOnly && aProperty->flags.isPersistent == 0) {
				return;
			}

			// Avoiding circular refs
			constexpr auto MAX_RECURSIVE_TYPE_DEPTH = 4;

			if (aDepth > MAX_RECURSIVE_TYPE_DEPTH) {
				return;
			}

			auto baseType = redRTTI::persistent::getRttiType(aProperty->type);
			for (auto i = 0; i < aIndentation; i++) {
				// Dumb way, I know
				outputDump << '\t';
			}

			outputDump << std::format("[{}] {} {} {}", aProperty->type->GetTypeName().c_str(), aProperty->type->GetName().ToString(), aProperty->name.ToString(), aProperty->valueOffset);
			outputDump << '\n';

			if (baseType->GetType() == RED4ext::ERTTIType::Class) {
				auto classInstance = aRttiSystem->GetClass(baseType->GetName());

				if (!classInstance) {
					return;
				}

				for (auto prop : classInstance->props) {
					printProperty(outputDump, aRttiSystem, prop, aPersistentOnly, aIndentation + 1, aDepth + 1);
				}
			}
		}

		void dumpRttiClassData(std::ofstream& outputDump, RED4ext::CRTTISystem* aRtti, RED4ext::CClass* aRttiClass) {
			if (!aRttiClass) {
				return;
			}

			outputDump << std::format("Class {}\n", aRttiClass->GetName().ToString());;

			for (auto prop : aRttiClass->props) {
				printProperty(outputDump, aRtti, prop, true, 1);
			}
		}

	public:
		static constexpr std::wstring_view nodeName = L"PersistencySystem2";
		virtual void readData(FileCursor& aCursor, NodeEntry& node) {
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
						//auto dumpFile = std::ofstream{ "vehicleGarageComponent.txt" };
						//std::println("{} in PersistencySystem", className.ToString());

						const auto redClass = rtti->GetClass(className);

						assert(redClass);

						//dumpRttiClassData(dumpFile, rtti, redClass);

						auto classBytes = aCursor.readBytes(classSize);
						auto subCursor = FileCursor{ classBytes.data(), classBytes.size() };

						auto redClassData = redRTTI::persistent::RedValueWrapper{};

						redClassData.m_value = redRTTI::persistent::reader::readClass(subCursor, rtti, className.ToString());
						redClassData.m_typeIndex = RED4ext::ERTTIType::Class;
						redClassData.m_typeName = className;

						classEntry.m_redClass = redClassData;

						redRTTI::persistent::dumper::dumpClass(redClassData);
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

			assert(chunkIt != m_redClasses.end());

			return *chunkIt;
		}
	};
}