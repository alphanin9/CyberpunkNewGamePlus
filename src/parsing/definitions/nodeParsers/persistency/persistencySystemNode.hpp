#pragma once
#include <RED4ext/RED4ext.hpp>
#include <cassert>
#include <vector>

#include "../interfaceNodeData.hpp"
#include "helpers/nativePersistencyReader.hpp"

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

namespace save
{
struct RedPersistentObject
{
    // Every persistent object should be ISerializable, I think?
    //... Could we just use a handle instead of rolling our own dtor?
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
    RedPersistentObject m_classInstance;
};

class PersistencySystemNode : public NodeDataInterface
{
private:
    static constexpr auto m_onlyDoVehicleGarage = true;

    std::uint32_t m_unk1;
    std::vector<PersistentBuffer> m_redClasses;

public:
    static constexpr Red::CName m_nodeName = "PersistencySystem2";

    virtual void ReadData(FileCursor& aCursor, NodeEntry& node)
    {
        const auto idCount = aCursor.readInt();

        aCursor.ReadSpan<std::uint32_t>(idCount); // Ignore m_ids, they're irrelevant for the usecase
        // Could just skip ahead...

        m_unk1 = aCursor.readUInt();
        
        persistency::native::NativePersistencyReader reader{};

        const auto itemCount = aCursor.readInt();

        for (auto i = 0; i < itemCount; i++)
        {
            auto classId = aCursor.readUInt64();

            if (classId)
            {
                const auto classHash = aCursor.ReadCNameHash();
                const auto classSize = aCursor.readUInt();

                auto startOffset = aCursor.offset;

                if (m_onlyDoVehicleGarage && classHash != "vehicleGarageComponentPS")
                {
                    aCursor.seekTo(startOffset + classSize);
                    continue;
                }

                const auto type = PluginContext::m_rtti->GetType(classHash);

                // For our purposes we only really want to parse
                // vehicleGarageComponentPS,
                // as we don't *really* need any other nodes

                // Let's test reading all persistent types for the lulz
                // Worst case scenario: we'll have a bunch of types with unfinished parsing
                // NEVERMIND, IT'S UNSTABLE
                if (type)
                {
                    auto instance = static_cast<Red::CClass*>(type)->CreateInstance();
                    auto subCursor = aCursor.CreateSubCursor(classSize);

                    if (reader.TryReadClass(subCursor, instance, type))
                    {
                        // Small optimization: instead of resizing the vector for all the persistent nodes (that we only
                        // use one of anyway), we only instantiate the needed one(s)
                        m_redClasses.emplace_back();

                        auto& entry = m_redClasses.back();

                        entry.m_classInstance.SetInstance(instance);
                        entry.m_className = classHash;
                        entry.m_id = classId;

                        if constexpr (m_onlyDoVehicleGarage)
                        {
                            // We're done here LOL
                            // Seek to the end of the node so LoadNodes doesn't whine

                            aCursor.seekTo(node.offset + node.GetExpectedSize());

                            break;
                        }
                    }
                }

                aCursor.seekTo(startOffset + classSize);
            }
        }
    }

    // Use std::string_view instead of CName to report missing chunks better
    PersistentBuffer* LookupChunk(std::string_view aChunkName) noexcept
    {
        auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(),
                                    [aChunkName](const PersistentBuffer& aBuffer)
                                    { return aBuffer.m_className == aChunkName.data(); });

        if (chunkIt == m_redClasses.end())
        {
            PluginContext::Error(std::format("Failed to find class {} in PersistencySystem2", aChunkName));
            return nullptr;
        }

        return &*chunkIt;
    }

    template<typename RedClass>
    RedClass* LookupInstanceAs(std::string_view aChunkName)
    {
        auto chunkPtr = LookupChunk(aChunkName);

        if (!chunkPtr)
        {
            return nullptr;
        }

        return chunkPtr->m_classInstance.As<RedClass>();
    }

    bool HasChunk(std::string_view aChunkName) const
    {
        auto chunkIt = std::find_if(m_redClasses.begin(), m_redClasses.end(),
                                    [aChunkName](const PersistentBuffer& aBuffer)
                                    { return aBuffer.m_className == aChunkName.data(); });

        return chunkIt != m_redClasses.end();
    }
};
}