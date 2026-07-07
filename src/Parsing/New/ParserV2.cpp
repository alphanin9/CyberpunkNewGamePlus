#include "ParserV2.hpp"

#include <filesystem/SaveFS.hpp>

using namespace Red;
//using namespace shared::raw;

bool parser::LoadStreamContainer::Setup(StringView& aName) noexcept
{
    const auto savePath = files::GetRedPathToSaveFile(aName.Data(), files::c_saveFileName);
    const auto metadataPath = files::GetRedPathToSaveFile(aName.Data(), files::c_metadataFileName);

    if (!files::LoadSaveMetadata(metadataPath, m_metadata))
    {
        return false;
    }

    auto fileManager = shared::raw::Filesystem::RedFileManager::GetInstance();

    m_fileStream = std::move(fileManager->OpenBufferedFileStream(savePath));

    if (!m_fileStream)
    {
        return false;
    }

    m_loadStream = std::move(shared::raw::Save::Stream::LoadStream::Create(m_fileStream, m_metadata));

    if (!m_loadStream)
    {
        return false;
    }

    return m_loadStream.Initialize();
}

bool parser::ParserV2::ParseSavegame(StringView aSaveName) noexcept
{
    if (!m_container.Setup(aSaveName))
    {
        return false;
    }

    // Each node opens its own Save::NodeAccessor, which seeks to the node by name. NodeAccessor is
    // random-access by name, so read order is not load-bearing for correctness; this ordering just
    // mirrors the game for readability. A later phase can parallelize independent nodes if the
    // native LoadStream proves safe for concurrent NodeAccessor reads (needs confirmation before
    // wiring into LoadSaveData).
    m_statsSystem.OnRead(m_container.m_loadStream);
    m_scriptableSystems.OnRead(m_container.m_loadStream);
    m_inventory.OnRead(m_container.m_loadStream);

    m_isValid = true;
    return true;
}