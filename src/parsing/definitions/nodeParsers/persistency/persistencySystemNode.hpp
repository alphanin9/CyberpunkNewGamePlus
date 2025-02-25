#pragma once
#include <RED4ext/RED4ext.hpp>
#include <cassert>
#include <vector>

#include "Helpers/NativePersistencyReader.hpp"
#include <Parsing/Definitions/NodeParsers/InterfaceNodeData.hpp>

#include <RED4ext/Scripting/Natives/Generated/vehicle/GarageComponentPS.hpp>

namespace modsave
{
struct RedPersistentObject
{
    // Every persistent object should be ISerializable, I think?
    //... Could we just use a handle instead of rolling our own dtor?
    // Very silly, just use handles lol

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

class PersistencySystemNode : public NodeDataInterface
{
private:
    static constexpr auto m_onlyDoVehicleGarage = true;

    std::uint32_t m_unk1{};
    std::vector<Red::Handle<Red::game::PersistentState>> m_instances{};

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
                    if constexpr (m_onlyDoVehicleGarage)
                    {
                        auto handle = Red::MakeHandle<Red::GarageComponentPS>();

                        auto subCursor = aCursor.CreateSubCursor(classSize);

                        if (reader.TryReadClass(subCursor, handle.instance, type))
                        {
                            // Small optimization: instead of resizing the vector for all the persistent nodes (that we
                            // only use one of anyway), we only instantiate the needed one(s)
                            m_instances.emplace_back(std::move(handle));

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
    Red::Handle<Red::game::PersistentState>* LookupChunk(std::string_view aChunkName) noexcept
    {
        auto chunkIt = std::find_if(m_instances.begin(), m_instances.end(),
                                    [aChunkName](const Red::Handle<Red::game::PersistentState>& aBuffer)
                                    { return aBuffer->GetType()->GetName() == aChunkName.data(); });

        if (chunkIt == m_instances.end())
        {
            PluginContext::Error("Failed to find class {} in PersistencySystem2", aChunkName);
            return nullptr;
        }

        auto& iter = *chunkIt;

        return &iter;
    }

    bool HasChunk(Red::CName aChunkName) const
    {
        auto chunkIt = std::find_if(m_instances.begin(), m_instances.end(),
                                    [aChunkName](const Red::Handle<Red::game::PersistentState>& aBuffer)
                                    { return aBuffer->GetType()->GetName() == aChunkName; });

        return chunkIt != m_instances.end();
    }
};
} // namespace modsave