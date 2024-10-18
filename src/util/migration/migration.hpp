#pragma once
#include <Windows.h>

namespace migration
{
void RemoveUnusedFiles();
void SetupModulePath(HMODULE aModule);
}