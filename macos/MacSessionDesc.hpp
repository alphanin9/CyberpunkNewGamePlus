#pragma once

// gameGameSessionDesc has a different layout on macOS. The SDK struct is the Windows one (0x1D0 bytes); the Mac
// game's type is 0x1B0 bytes and its fields move from 0xB0 on (the game's copy at 0x1F46BCC reads a 0x10-byte
// handle at 0xB0 where Windows has a CString, mainQuests at 0x140, the character state at 0x178).
// Passing the SDK struct makes the game read the spawn orientation (1.0f) as an array size, and its overflow check
// in dynamicBuffer.cpp stops the game when the session argument is copied.
//
// SessionDesc lets the game construct and destruct its own object through RTTI (so every default the Mac
// constructor sets is right) and gives references to the three fields NG+ fills, under the SDK field names.

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/CharacterCustomizationState.hpp>
#include <RED4ext/Scripting/Natives/gameGameSessionDesc.hpp>
#include <RedLib.hpp>

#include <cstddef>
#include <cstdint>

namespace shared::raw::Ink::SessionData
{
struct Data;
}

namespace ngp::mac
{
class SessionDesc
{
public:
    static constexpr std::uint32_t Size = 0x1B0;
    static constexpr std::size_t WorldIdOffset = 0x0;
    static constexpr std::size_t MainQuestsOffset = 0x140;
    static constexpr std::size_t CharacterCustomizationStateOffset = 0x178;

    SessionDesc();
    ~SessionDesc();

    SessionDesc(const SessionDesc&) = delete;
    SessionDesc& operator=(const SessionDesc&) = delete;

    // False when the game's type is missing or its size is not the one these offsets were read from. Nothing may be
    // read or written through the references then.
    explicit operator bool() const;

    // Copies the object into the session's argument list (the game copies it; this object keeps its own).
    void AddTo(shared::raw::Ink::SessionData::Data& aSessionData, Red::CName aName);

private:
    Red::CBaseRTTIType* m_type;
    bool m_constructed;
    alignas(16) std::uint8_t m_storage[Size];

public:
    Red::world::WorldID& worldId;
    Red::DynArray<Red::game::MainQuestData>& mainQuests;
    Red::Handle<Red::game::ui::CharacterCustomizationState>& characterCustomizationState;
};
}
