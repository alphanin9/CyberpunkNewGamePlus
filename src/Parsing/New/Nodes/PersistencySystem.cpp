#include <Shared/Raw/Persistency/Persistency.hpp>
#include <Shared/Util/NamePoolRegistrar.hpp>

#include "PersistencySystem.hpp"

using namespace parser::node;
using namespace Red;
using namespace shared::raw;
using namespace shared::util;

bool PersistencySystemNode::OnRead(Save::Stream::LoadStream& aStream) noexcept
{
    static const auto c_vehicleGarage = NamePoolRegistrar<"vehicleGarageComponentPS">::Get();

    Save::NodeAccessor node(aStream, GetName(), true, false);

    if (!aStream.IsGood())
    {
        return false;
    }

    BaseStream* stream = aStream; // LoadStream -> BaseStream* conversion operator

    // "PersistencySystem2" layout (validated against game::PersistencySystem::OnGameLoad on 2.31):
    //   u32 activeIndexCount; u32 activeIndices[activeIndexCount];   // presence table, unused here
    //   u32 reservedBlobSize; u8  reservedBlob[reservedBlobSize];    // currently always written as 0
    //   u32 slotCount;
    //   slotCount x { u64 persistencyId; if (id != 0) { CName className; u32 blobSize; u8 blob[blobSize]; } }
    // All counts/ids are fixed-size raw reads (stream vtable +0x10). Seek() is absolute.

    std::uint32_t activeIndexCount = 0u;
    stream->ReadWriteEx(&activeIndexCount);
    stream->Seek(stream->GetPointerPosition() + static_cast<std::size_t>(activeIndexCount) * sizeof(std::uint32_t));

    std::uint32_t reservedBlobSize = 0u;
    stream->ReadWriteEx(&reservedBlobSize);
    if (reservedBlobSize)
    {
        stream->Seek(stream->GetPointerPosition() + reservedBlobSize);
    }

    std::uint32_t slotCount = 0u;
    stream->ReadWriteEx(&slotCount);

    for (std::uint32_t i = 0u; i < slotCount; i++)
    {
        std::uint64_t persistencyId = 0u;
        stream->ReadWriteEx(&persistencyId);

        if (!persistencyId)
        {
            continue; // empty slot, no payload follows
        }

        CName className{};
        stream->ReadWriteEx(&className);

        std::uint32_t blobSize = 0u;
        stream->ReadWriteEx(&blobSize);

        const auto blobEnd = stream->GetPointerPosition() + blobSize;

        if (className != c_vehicleGarage)
        {
            stream->Seek(blobEnd); // skip this slot's blob
            continue;
        }

        auto handle = MakeHandle<GarageComponentPS>();

        // Reuse the game's own deserializer on the live stream. The class body is self-terminating,
        // so this consumes exactly the blob; keep the handle best-effort even on a soft type mismatch.
        Persistency::ReadObjectFromStream(stream, handle.instance, GetClass<GarageComponentPS>());
        m_vehicleGarage = std::move(handle);

        // Only the garage is needed; realign to the blob end (defensive) and stop scanning.
        stream->Seek(blobEnd);
        break;
    }

    return aStream.IsGood();
}

CName PersistencySystemNode::GetName() noexcept
{
    return NamePoolRegistrar<"PersistencySystem2">::Get();
}

RTTI_DEFINE_CLASS(parser::node::PersistencySystemNode, { RTTI_PARENT(parser::node::SaveNodeData); });
