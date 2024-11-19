#include "ParserV2.hpp"

#include <filesystem/filesystem.hpp>

using namespace Red;
using namespace shared::raw;

bool parser::LoadStreamContainer::Setup(StringView& aName) noexcept
{
    const auto savePath = files::GetRedPathToSaveFile(aName.Data(), files::c_saveFileName);
    const auto metadataPath = files::GetRedPathToSaveFile(aName.Data(), files::c_metadataFileName);

    if (!files::LoadSaveMetadata(metadataPath, m_metadata))
    {
        return false;
    }

    auto fileManager = Filesystem::RedFileManager::GetInstance();

    m_fileStream = std::move(fileManager->OpenBufferedFileStream(savePath));

    if (!m_fileStream)
    {
        return false;
    }

    m_loadStream = std::move(Save::Stream::LoadStream::Create(m_fileStream, m_metadata));

    if (!m_loadStream)
    {
        return false;
    }

    return m_loadStream.Initialize();
}