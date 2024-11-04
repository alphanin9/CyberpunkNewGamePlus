#pragma once
#include <detail/hashes.hpp>
#include <util/core.hpp>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/StringView.hpp>

#include <RED4ext/Scripting/Natives/Generated/save/Metadata.hpp>

#include <RedLib.hpp>

namespace raw
{
namespace Filesystem
{
struct RedFileStream
{
    static constexpr auto DestructFileStream =
        util::RawFunc<detail::Hashes::FileStream_dtor, void (*)(RedFileStream*)>();

    Red::BaseStream* m_stream;

    RedFileStream(Red::BaseStream* aStream)
        : m_stream(aStream)
    {
    }

    RedFileStream() = default;
    RedFileStream(const RedFileStream&) = delete;
    RedFileStream& operator=(const RedFileStream&) = delete;
    RedFileStream(RedFileStream&&) = default;
    ~RedFileStream()
    {
        if (m_stream)
        {
            DestructFileStream(this);
        }
    }

    Red::BaseStream* operator->()
    {
        return m_stream;
    }

    Red::BaseStream* const operator->() const
    {
        return m_stream;
    }

    operator Red::BaseStream*()
    {
        return m_stream;
    }

    operator Red::BaseStream* const() const
    {
        return m_stream;
    }

    operator bool() const
    {
        return m_stream != nullptr;
    }
};

struct RedFileManager
{
    static constexpr auto Instance = util::RawPtr<detail::Hashes::RedFileManager_Instance, RedFileManager*>();

    static constexpr auto FindFiles =
        util::RawFunc<detail::Hashes::RedFileManager_FindFiles,
                      void* (*)(RedFileManager*, const Red::CString& aRootPath, const Red::StringView& aFileName,
                                Red::DynArray<Red::CString>&, bool aRecurse)>();

    static constexpr auto OpenFileStreamInternal =
        util::RawFunc<detail::Hashes::RedFileManager_OpenFileStream,
                      void* (*)(RedFileManager*, RedFileStream&, const Red::CString&, char)>();

    static RedFileManager* GetInstance()
    {
        return Instance;
    }

    Red::DynArray<Red::CString> FindFilesByName(const Red::CString& aRootPath, const Red::StringView& aFileName)
    {
        Red::DynArray<Red::CString> ret{};

        FindFiles(this, aRootPath, aFileName, ret, true);

        return ret;
    }

    RedFileStream OpenFileStream(const Red::CString& aPath)
    {
        RedFileStream ret{};
        OpenFileStreamInternal(this, ret, aPath, 0);

        return ret;
    }
};

namespace Path
{
constexpr auto GetSaveFolder = util::RawFunc<detail::Hashes::FileSystem_GetSaveFolder, Red::CString& (*)()>();

constexpr auto MergeFileToPath =
    util::RawFunc<detail::Hashes::FileSystem_MergeFileToPath,
                  Red::CString* (*)(const Red::CString&, Red::CString*, const Red::StringView&)>();
constexpr auto MergeDirToPath =
    util::RawFunc<detail::Hashes::FileSystem_MergeDirToPath,
                  Red::CString* (*)(const Red::CString&, Red::CString*, const Red::StringView&)>();
} // namespace Path
} // namespace Filesystem
namespace SaveMetadata
{
constexpr auto LoadSaveMetadataFromStream = util::RawFunc<detail::Hashes::SaveMetadata_LoadSaveMetadataFromFile,
                                                          bool (*)(Red::BaseStream*, Red::save::Metadata&)>();
}
} // namespace raw
