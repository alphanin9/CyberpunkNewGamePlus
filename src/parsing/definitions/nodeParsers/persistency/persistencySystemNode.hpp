#pragma once
#include <RED4ext/RED4ext.hpp>
#include <cassert>
#include <vector>

#include "../interfaceNodeData.hpp"
#include "helpers/nativePersistencyReader.hpp"
#include "helpers/persistencyRttiClassCreator.hpp"

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

namespace cyberpunk
{
struct RedPersistentObject
{
    // Every persistent object should be ISerializable, I think?
    Red::ISerializable* m_ptr;

    RedPersistentObject(Red::ScriptInstance aInstance)
        : m_ptr(reinterpret_cast<Red::ISerializable*>(aInstance))
    {
    }

    void SetInstance(Red::ScriptInstance aInstance)
    {
        m_ptr = reinterpret_cast<Red::ISerializable*>(aInstance);
    }

    template<typename NativeClass>
    NativeClass* As()
    {
        return Red::Cast<NativeClass>(m_ptr);
    }

    Red::ISerializable* Raw()
    {
        return m_ptr;
    }

    RedPersistentObject() = default;

    ~RedPersistentObject()
    {
        if (!m_ptr)
        {
            return;
        }
        if (m_ptr->CanBeDestructed())
        {
            m_ptr->GetType()->Destruct(m_ptr);
        }

        m_ptr->GetAllocator()->Free(m_ptr);
    }
};

struct PersistentBuffer
{
    std::uint64_t m_id;
    RED4ext::CName m_className;
    redRTTI::RTTIValue m_redClass;

    RedPersistentObject m_classInstance;
};

class PersistencySystemNode : public NodeDataInterface
{
private:
    static constexpr auto m_onlyDoVehicleGarage = true;

    std::vector<std::uint32_t> m_ids;
    std::uint32_t m_unk1;
    std::vector<PersistentBuffer> m_redClasses;

public:
    static constexpr std::wstring_view nodeName = L"PersistencySystem2";

    virtual void ReadData(FileCursor& aCursor, NodeEntry& node)
    {
        RED4EXT_UNUSED_PARAMETER(node);
        const auto idCount = aCursor.readInt();

        m_ids = aCursor.ReadMultiplePOD<std::uint32_t>(idCount);
        /* m_ids.reserve(idCount);
        for (auto i = 0; i < idCount; i++)
        {
            m_ids.push_back(aCursor.readUInt());
        }
        */
        m_unk1 = aCursor.readUInt();
        
        persistency::native::NativePersistencyReader reader{};

        // std::unordered_set<Red::CName> persistentClassTypes;

        const auto itemCount = aCursor.readInt();

        m_redClasses.resize(itemCount); // Hack to get m_classInstance to be less wonky with moves

        for (auto i = 0; i < itemCount; i++)
        {
            auto& classEntry = m_redClasses.at(i);
            classEntry.m_id = aCursor.readUInt64();

            if (classEntry.m_id)
            {
                const auto classHash = aCursor.ReadCNameHash();
                const auto classSize = aCursor.readUInt();

                classEntry.m_className = classHash;

                const auto type = PluginContext::m_rtti->GetType(classHash);

                auto startOffset = aCursor.offset;
                // For our purposes we only really want to parse
                // vehicleGarageComponentPS,
                // as we don't *really* need any other nodes

                // Let's test reading all persistent types for the lulz
                // Worst case scenario: we'll have a bunch of types with unfinished parsing
                // NEVERMIND, IT'S UNSTABLE
                if (type && (!m_onlyDoVehicleGarage || classHash == "vehicleGarageComponentPS"))
                {
                    // Seems to work, will need to refactor Redscript bit later

                    auto subCursor =
                        aCursor.CreateSubCursor(classSize); // FileCursor{ classBytes.data(), classBytes.size() };
                    
                    auto instance = static_cast<Red::CClass*>(type)->CreateInstance();

                    classEntry.m_classInstance.SetInstance(instance);

                    try
                    {
                        reader.ReadClass(subCursor, classEntry.m_classInstance.Raw(), type);
                    }
                    catch (const std::exception& e)
                    {
                        // Don't do anything
                    }
                }

                aCursor.seekTo(FileCursor::SeekTo::Start, startOffset + classSize);
            }
        }
        // PluginContext::Spew("Persistency used types");

        // for (auto name : persistentClassTypes)
        //{
        //     PluginContext::Spew(name.ToString());
        // }

        // PluginContext::Spew(" ");

        // std::println("{} nodes in PersistencySystem", m_redClasses.size());
    }

    PersistentBuffer& LookupChunk(std::string_view aChunkName)
    {
        auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(),
                                    [aChunkName](const PersistentBuffer& aBuffer)
                                    { return aBuffer.m_className == aChunkName.data(); });

        if (chunkIt == m_redClasses.end())
        {
            throw std::runtime_error{std::format("Failed to find class {} in PersistencySystem2", aChunkName)};
        }

        return *chunkIt;
    }

    bool HasChunk(std::string_view aChunkName) const
    {
        auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(),
                                    [aChunkName](const PersistentBuffer& aBuffer)
                                    { return aBuffer.m_className == aChunkName.data(); });

        return chunkIt != m_redClasses.end();
    }
};
} // namespace cyberpunk