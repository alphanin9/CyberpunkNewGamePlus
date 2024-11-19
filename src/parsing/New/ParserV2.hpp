#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/StringView.hpp>

#include <Shared/Raw/FileSystem/FileSystem.hpp>
#include <Shared/Raw/Save/Save.hpp>

#include <RED4ext/Scripting/Natives/Generated/save/MetadataContainer.hpp>

namespace parser
{
struct LoadStreamContainer
{
    Red::save::Metadata m_metadata{};

    shared::raw::Filesystem::BufferedRedFileStream m_fileStream{};
    shared::raw::Save::Stream::LoadStream m_loadStream{};

    bool Setup(Red::StringView& aSaveName) noexcept;
};

// Parser using native save stream
class ParserV2
{
    LoadStreamContainer m_container{};
public:

};
}