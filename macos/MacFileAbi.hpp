#pragma once

// Mac calling convention for the game functions whose registers differ from SharedPunk's prototypes (those are the
// Windows register assignment; a plain call through those types would crash).
// Each function here is a noinline wrapper. The call inside it is what objdump checks.

#include <Shared/Raw/FileSystem/FileSystem.hpp>
#include <Shared/Raw/GameDefinition/GameDefinition.hpp>

namespace ngp::mac
{
// No arguments. The CString& comes back in x0.
Red::CString& GetSaveFolder();

// x0 src, x1 chars, w2 length, x8 destination. The returned CString is the x8 object.
Red::CString MergeDirToPath(const Red::CString& aSrc, Red::StringView aName);
Red::CString MergeFileToPath(const Red::CString& aSrc, Red::StringView aName);

// x0 this, x1 root, x2 chars, w3 length, x4 out array, w5 recurse. The array is x4.
Red::DynArray<Red::CString> FindFiles(shared::raw::Filesystem::RedFileManager* aSelf, const Red::CString& aRoot,
                                      Red::StringView aName, bool aRecurse);

// x0 unused, x1 path, w2 flags, x8 out BaseStream**. The pointer is read from that slot.
Red::BaseStream* OpenFileStream(const Red::CString& aPath, std::uint32_t aFlags);

// GameDefinition::ToWorldID: x0 definition, x8 the WorldID to fill (Windows passes it as a second argument).
void ToWorldID(Red::gsm::GameDefinition* aDefinition, Red::world::WorldID* aOut);
}
