#pragma once
#include <cassert>
#include <vector>
#include <RED4ext/RED4ext.hpp>

#include "../interfaceNodeData.hpp"
#include "helpers/persistencyRttiClassCreator.hpp"
#include "helpers/nativePersistencyReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

namespace cyberpunk {
	struct UnknownRedBuffer {
		std::uint64_t m_id;
		RED4ext::CName m_className;
        redRTTI::RTTIValue m_redClass;
	};

	class PersistencySystemNode : public NodeDataInterface
    {
        std::vector<std::uint32_t> m_ids;
        std::uint32_t m_unk1;
        std::vector<UnknownRedBuffer> m_redClasses;

    private:
        void DumpUsedTypes(std::unordered_set<Red::CName>& aTypeSet, Red::CBaseRTTIType* aType, int aDepth = 0)
        {
            constexpr auto maxDepth = 10;

			if (aDepth >= maxDepth)
            {
                return;
			}

            const auto typeId = aType->GetType();

			if (typeId == Red::ERTTIType::Class)
            {
                const auto asClass = static_cast<Red::CClass*>(aType);

				Red::DynArray<Red::CProperty*> props{};

				asClass->GetProperties(props);

				for (auto prop : props)
                {
                    DumpUsedTypes(aTypeSet, prop->type, aDepth + 1);
				}

				return;
			}
            else if (typeId == Red::ERTTIType::Handle)
            {
                const auto asHandle = static_cast<Red::CRTTIHandleType*>(aType);
                const auto innerType = asHandle->GetInnerType();

				DumpUsedTypes(aTypeSet, innerType, aDepth + 1);

				return;
			}
            else if (typeId == Red::ERTTIType::WeakHandle)
            {
                const auto asHandle = static_cast<Red::CRTTIWeakHandleType*>(aType);
                const auto innerType = asHandle->GetInnerType();

                DumpUsedTypes(aTypeSet, innerType, aDepth + 1);

                return;
			}
            else if (typeId == Red::ERTTIType::Array || typeId == Red::ERTTIType::StaticArray)
            {
                const auto asArray = static_cast<Red::CRTTIBaseArrayType*>(aType);
                const auto innerType = asArray->GetInnerType();

                DumpUsedTypes(aTypeSet, innerType, aDepth + 1);

                return;
			}
            
			aTypeSet.insert(aType->GetName());
		}

        static constexpr auto m_testNativeReader = false;

	public:
		static constexpr std::wstring_view nodeName = L"PersistencySystem2";

        virtual void ReadData(FileCursor& aCursor, NodeEntry& node)
        {
            RED4EXT_UNUSED_PARAMETER(node);
            const auto idCount = aCursor.readInt();
            m_ids.reserve(idCount);
            for (auto i = 0; i < idCount; i++)
            {
                m_ids.push_back(aCursor.readUInt());
            }
            m_unk1 = aCursor.readUInt();
            const auto itemCount = aCursor.readInt();

            // std::unordered_set<Red::CName> persistentClassTypes;

            for (auto i = 0; i < itemCount; i++)
            {
                auto classEntry = UnknownRedBuffer{};
                classEntry.m_id = aCursor.readUInt64();

                if (classEntry.m_id)
                {
                    const auto classHash = aCursor.ReadCNameHash();
                    const auto classSize = aCursor.readUInt();

                    classEntry.m_className = classHash;

                    const auto type = PluginContext::m_rtti->GetType(classHash);

                    if (type)
                    {
                        // DumpUsedTypes(persistentClassTypes, type);
                    }

                    auto startOffset = aCursor.offset;
                    // For our purposes we only really want to parse
                    // vehicleGarageComponentPS,
                    // as we don't *really* need any other nodes
                    if (classHash == "vehicleGarageComponentPS" && type)
                    {
                        // auto classBytes = aCursor.readBytes(classSize);
                        auto subCursor =
                            aCursor.CreateSubCursor(classSize); // FileCursor{ classBytes.data(), classBytes.size() };

                        persistent::PersistentReader reader{};

                        redRTTI::RTTIValue wrapper{};

                        wrapper.m_typeIndex = Red::ERTTIType::Class;
                        wrapper.m_typeName = classHash;

                        try
                        {
                            wrapper.m_value = reader.ReadClass(subCursor, type);
                        }
                        catch (const std::exception& e)
                        {
                            PluginContext::Error(std::format(
                                "Failed to parse vehicle garage, error: {}, setting to default!", e.what()));
                            wrapper = reader.GetDefaultValue(type);
                        }

                        classEntry.m_redClass = wrapper;

                        // Seems to work, will need to refactor Redscript bit later
                        if constexpr (m_testNativeReader)
                        {
                            aCursor.seekTo(FileCursor::SeekTo::Start, startOffset);

                            auto subCursor = aCursor.CreateSubCursor(
                                classSize); // FileCursor{ classBytes.data(), classBytes.size() };

                            persistency::native::NativePersistencyReader reader{};

                            auto instance =
                                type->GetAllocator()->AllocAligned(type->GetSize(), type->GetAlignment()).memory;
                            type->Construct(instance);

                            reader.ReadClass(subCursor, instance, type);

                            auto garageComponent = Red::Cast<Red::GarageComponentPS>(reinterpret_cast<Red::ISerializable*>(instance));

                            for (auto& vehicle : garageComponent->unlockedVehicleArray)
                            {
                                Red::CString str{};
                                Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, vehicle.vehicleID.recordID);

                                PluginContext::Spew(std::format("Unlocked vehicle: {}", str.c_str()));
                            }

                            type->Destruct(instance);
                            type->GetAllocator()->Free(instance);
                        }
                    }

                    aCursor.seekTo(FileCursor::SeekTo::Start, startOffset + classSize);
                }

                m_redClasses.push_back(classEntry);
            }
            //PluginContext::Spew("Persistency used types");

			//for (auto name : persistentClassTypes)
            //{
            //    PluginContext::Spew(name.ToString());
			//}

			//PluginContext::Spew(" ");

			//std::println("{} nodes in PersistencySystem", m_redClasses.size());
		}

		const UnknownRedBuffer& LookupChunk(std::string_view aChunkName) const 
        {
			auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(), [aChunkName](const UnknownRedBuffer& aBuffer) 
                { 
                    return aBuffer.m_className == aChunkName.data();
			    });

			if (chunkIt == m_redClasses.end())
            {
                throw std::runtime_error{std::format("Failed to find class {} in PersistencySystem2", aChunkName)};
			}

			return *chunkIt;
		}

        bool HasChunk(std::string_view aChunkName) const
        {
            auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(),
                                        [aChunkName](const UnknownRedBuffer& aBuffer)
                                        { return aBuffer.m_className == aChunkName.data(); });

            return chunkIt != m_redClasses.end();
        }
	};
}