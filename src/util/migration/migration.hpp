#pragma once
#include <filesystem>

#include <Windows.h>

namespace migration
{
void RemoveUnusedFiles();
void SetupModulePath(HMODULE aModule);
const std::filesystem::path& GetModulePath();
}