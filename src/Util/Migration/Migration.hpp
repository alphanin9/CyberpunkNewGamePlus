#pragma once
#include <filesystem>

#include <Windows.h>

namespace migration
{
void RemoveUnusedFiles();
void SetupModulePath();
const std::filesystem::path& GetModulePath();
} // namespace migration