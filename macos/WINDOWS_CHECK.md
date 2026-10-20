# Windows-view check: New Game Plus and SharedPunk

Base: New Game Plus tag `v1.3.2`. SharedPunk commit `5779848`.

Method: for every shared file this branch changes outside `macos/`,

`diff -u <(git show <base>:<f> | unifdef -U__APPLE__) <(unifdef -U__APPLE__ <f>)`

`unifdef -U__APPLE__` drops `#ifdef __APPLE__` bodies, keeps `#else` and `#ifndef __APPLE__` bodies, and removes the directives.
`macos/` is new and is not in this set.

`unifdef` prints `EOF in comment` on several of these files (a `/*` it does not close). The byte compare below is of the
full outputs. Where the table says match, those outputs were identical.

## Result

The Windows view matches the base except for a final newline on four files. The tag copies of those four end on `}`
with no newline. This branch puts `#endif` after that line, so the kept Windows text ends with a newline. No other hunk.

| file | repo | result |
|---|---|---|
| `config/projectTemplate.hpp.in` | New Game Plus | final newline only |
| `src/Context/Context.hpp` | New Game Plus | match |
| `src/Filesystem/SaveFS.cpp` | New Game Plus | match |
| `src/GameSystem/NewGamePlusSystem.cpp` | New Game Plus | match |
| `src/Hooking/Hooking.cpp` | New Game Plus | match |
| `src/Main.cpp` | New Game Plus | final newline only |
| `src/Parsing/CursorDef.hpp` | New Game Plus | match |
| `src/Util/Migration/Migration.cpp` | New Game Plus | match |
| `src/Util/Migration/Migration.hpp` | New Game Plus | match |
| `src/code/Hooks/HookingDriver.cpp` | SharedPunk | final newline only |
| `src/code/Raw/FileSystem/FileSystem.cpp` | SharedPunk | match |
| `src/include/Impl/Hooks/Driver/HookingDriver.hpp` | SharedPunk | final newline only |
| `src/include/Impl/Hooks/HookManager/HookDetail.hpp` | SharedPunk | match |
| `src/include/Public/Shared/Hooks/HookManager.hpp` | SharedPunk | match |
| `src/include/Public/Shared/Raw/Assert/AssertionFailed.hpp` | SharedPunk | match |
| `src/include/Public/Shared/Util/Core.hpp` | SharedPunk | match |

The Windows `Main` / `Query` / `Supports`, `scripts->Add`, `semver::parse`, `WideCharToMultiByte`, `wil`, and the
safetyhook driver are the base text. The Mac entry points, `ngplus_registerNatives`, the save gate, and the loader hook
driver are inside `#ifdef __APPLE__`.

`HookManager.hpp` keeps the namespace-scope overloads that name `this` in the Windows view. Clang rejects those while
parsing, so they are under `#ifndef __APPLE__`. NG+ installs free-function hooks. The Windows text of that region matches
`5779848`.
