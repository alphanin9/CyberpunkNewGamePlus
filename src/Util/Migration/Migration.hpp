#pragma once
#include <filesystem>

#ifndef __APPLE__
#include <Windows.h>
#endif

namespace migration
{
void RemoveUnusedFiles();
void SetupModulePath();
const std::filesystem::path& GetModulePath();
} // namespace migration