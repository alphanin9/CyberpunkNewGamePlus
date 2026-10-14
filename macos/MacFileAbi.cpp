#include "MacFileAbi.hpp"

namespace ngp::mac
{
namespace
{
// Eight bytes, with a user-provided destructor, so clang returns it through x8.
// OpenFileStream stores the BaseStream* at [x8] and does not put it in x0.
struct StreamOut
{
    Red::BaseStream* stream;

    ~StreamOut() {}
};

template<typename Fn>
Fn AddressOf(const auto& aRaw)
{
    return reinterpret_cast<Fn>(aRaw.GetAddress());
}
}

__attribute__((noinline)) Red::CString& GetSaveFolder()
{
    using Fn = Red::CString& (*)();
    auto& raw = shared::raw::Filesystem::Path::GetSaveFolder;
    if (!raw.IsAvailable())
    {
        static Red::CString standin{};
        return standin;
    }
    return AddressOf<Fn>(raw)();
}

__attribute__((noinline)) Red::CString MergeDirToPath(const Red::CString& aSrc, Red::StringView aName)
{
    using Fn = Red::CString (*)(const Red::CString*, const char*, std::uint32_t);
    auto& raw = shared::raw::Filesystem::Path::MergeDirToPath;
    if (!raw.IsAvailable())
    {
        return {};
    }
    return AddressOf<Fn>(raw)(&aSrc, aName.Data(), aName.Length());
}

__attribute__((noinline)) Red::CString MergeFileToPath(const Red::CString& aSrc, Red::StringView aName)
{
    using Fn = Red::CString (*)(const Red::CString*, const char*, std::uint32_t);
    auto& raw = shared::raw::Filesystem::Path::MergeFileToPath;
    if (!raw.IsAvailable())
    {
        return {};
    }
    return AddressOf<Fn>(raw)(&aSrc, aName.Data(), aName.Length());
}

__attribute__((noinline)) Red::DynArray<Red::CString> FindFiles(shared::raw::Filesystem::RedFileManager* aSelf,
                                                                const Red::CString& aRoot, Red::StringView aName,
                                                                bool aRecurse)
{
    using Fn = void (*)(shared::raw::Filesystem::RedFileManager*, const Red::CString*, const char*, std::uint32_t,
                        Red::DynArray<Red::CString>*, std::uint32_t);
    Red::DynArray<Red::CString> ret{};
    auto& raw = shared::raw::Filesystem::RedFileManager::FindFiles;
    if (!raw.IsAvailable())
    {
        return ret;
    }
    AddressOf<Fn>(raw)(aSelf, &aRoot, aName.Data(), aName.Length(), &ret, aRecurse ? 1u : 0u);
    return ret;
}

__attribute__((noinline)) Red::BaseStream* OpenFileStream(const Red::CString& aPath, std::uint32_t aFlags)
{
    using Fn = StreamOut (*)(void*, const Red::CString*, std::uint32_t);
    auto& raw = shared::raw::Filesystem::RedFileManager::OpenFileStreamInternal;
    if (!raw.IsAvailable())
    {
        return nullptr;
    }
    StreamOut slot = AddressOf<Fn>(raw)(nullptr, &aPath, aFlags);
    return slot.stream;
}

__attribute__((noinline)) void ToWorldID(Red::gsm::GameDefinition* aDefinition, Red::world::WorldID* aOut)
{
    // WorldID is 0x38 bytes, so the result goes through x8.
    using Fn = Red::world::WorldID (*)(Red::gsm::GameDefinition*);
    auto& raw = shared::raw::GameDefinition::ToWorldID;
    if (!raw.IsAvailable())
    {
        return;
    }
    *aOut = AddressOf<Fn>(raw)(aDefinition);
}
}
