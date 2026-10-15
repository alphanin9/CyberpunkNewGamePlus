cmake_minimum_required(VERSION 3.21)
# arm64 build of New Game Plus for the native macOS game. Windows builds keep xmake.lua.

set(NGP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
find_package(Git QUIET)
set(_ngp_tag "")
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${NGP_DIR}" describe --tags --abbrev=0 --match "v[0-9]*"
                    OUTPUT_VARIABLE _ngp_tag OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
string(REGEX MATCH "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ngp_ver "${_ngp_tag}")
if(_ngp_ver)
    set(NGP_VER_MAJOR ${CMAKE_MATCH_1})
    set(NGP_VER_MINOR ${CMAKE_MATCH_2})
    set(NGP_VER_PATCH ${CMAKE_MATCH_3})
else()
    message(WARNING "no v<major>.<minor>.<patch> tag found, using 1.3.2")
    set(NGP_VER_MAJOR 1)
    set(NGP_VER_MINOR 3)
    set(NGP_VER_PATCH 2)
endif()
project(NewGamePlus VERSION ${NGP_VER_MAJOR}.${NGP_VER_MINOR}.${NGP_VER_PATCH} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

if(NOT APPLE)
    message(FATAL_ERROR "This target builds the macOS New Game Plus dylib")
endif()

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")

set(RED4EXT_SDK_DIR "${NGP_DIR}/../../RED4ext.SDK-macos" CACHE PATH "macOS RED4ext SDK checkout")
get_filename_component(RED4EXT_SDK_DIR "${RED4EXT_SDK_DIR}" ABSOLUTE)
set(SDK_INCLUDE "${RED4EXT_SDK_DIR}/include")
set(CODEWARE_DIR "${NGP_DIR}/../../cp2077-codeware" CACHE PATH "macOS Codeware checkout (its SDK-core address rows are reused)")
get_filename_component(CODEWARE_DIR "${CODEWARE_DIR}" ABSOLUTE)
set(SHARED_PUNK_DIR "${NGP_DIR}/../sharedpunk" CACHE PATH "SharedPunk checkout")
get_filename_component(SHARED_PUNK_DIR "${SHARED_PUNK_DIR}" ABSOLUTE)
set(HOPSCOTCH_DIR "${NGP_DIR}/../../cet/vendor-mac/hopscotch-map" CACHE PATH "hopscotch-map headers")
get_filename_component(HOPSCOTCH_DIR "${HOPSCOTCH_DIR}" ABSOLUTE)
set(REDLIB_ROOT "${NGP_DIR}/../in_world_navigation/deps/red_lib" CACHE PATH "RedLib tree the In-World Navigation port uses")
get_filename_component(REDLIB_ROOT "${REDLIB_ROOT}" ABSOLUTE)
set(LZ4_INCLUDE "/opt/homebrew/include" CACHE PATH "lz4 headers")
set(NLOHMANN_JSON_INCLUDE "${NGP_DIR}/../../cet/vendor-mac/json/single_include" CACHE PATH "nlohmann/json headers (metadata.9.json reader)")
set(LZ4_STATIC "/opt/homebrew/lib/liblz4.a" CACHE FILEPATH "static lz4")

if(NOT EXISTS "${SDK_INCLUDE}/RED4ext/Relocation-inl.hpp")
    message(FATAL_ERROR "no RED4ext SDK headers at ${SDK_INCLUDE} (set -DRED4EXT_SDK_DIR=...)")
endif()
if(NOT EXISTS "${HOPSCOTCH_DIR}/include/tsl/hopscotch_set.h")
    message(FATAL_ERROR "no hopscotch-map headers at ${HOPSCOTCH_DIR}")
endif()
if(NOT EXISTS "${LZ4_STATIC}" OR NOT EXISTS "${LZ4_INCLUDE}/lz4.h")
    message(FATAL_ERROR "no Homebrew lz4 static library (looked for ${LZ4_STATIC} and ${LZ4_INCLUDE}/lz4.h)")
endif()
if(NOT EXISTS "${REDLIB_ROOT}/include/RedLib.hpp")
    message(FATAL_ERROR "no RedLib headers at ${REDLIB_ROOT}")
endif()

add_compile_definitions(
    RED4EXT_PLATFORM_MACOS
    RED4EXT_HEADER_ONLY
    MOD_VERSION_STR="${PROJECT_VERSION}"
    MOD_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MOD_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MOD_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

get_filename_component(_ngp_root "${NGP_DIR}" ABSOLUTE)
add_compile_options("-ffile-prefix-map=${_ngp_root}=new_game_plus"
                    "-ffile-prefix-map=${SHARED_PUNK_DIR}=sharedpunk"
                    "-ffile-prefix-map=${RED4EXT_SDK_DIR}=RED4ext.SDK-macos"
                    "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build")

# Generated SDK overlay: the Mac address table plus Codeware's macos/MacSdkRows.inc plus macos/NgpSdkRows.inc.
# An unknown hash goes to ngplus_mac_trap_for_hash.
set(SDK_OVERLAY "${CMAKE_CURRENT_BINARY_DIR}/sdk-overlay")
set(SDK_RELOC "${SDK_INCLUDE}/RED4ext/Relocation-inl.hpp")
set(SDK_ROWS_INC "${CMAKE_CURRENT_SOURCE_DIR}/NgpSdkRows.inc")
file(READ "${SDK_RELOC}" SDK_RELOC_TEXT)
file(READ "${SDK_ROWS_INC}" SDK_ROWS_TEXT)
set(CW_ROWS_INC "${CODEWARE_DIR}/macos/MacSdkRows.inc")
if(NOT EXISTS "${CW_ROWS_INC}")
    message(FATAL_ERROR "no Codeware Mac rows at ${CW_ROWS_INC} (set -DCODEWARE_DIR=...)")
endif()
file(READ "${CW_ROWS_INC}" CW_ROWS_TEXT)
set(SDK_ROWS_MARKER "        { 4201325776, 0x21739E8 },  // CGlobalFunction_ctor\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_ROWS_MARKER}" SDK_ROWS_POS)
if(SDK_ROWS_POS EQUAL -1)
    message(FATAL_ERROR "the anchor row (CGlobalFunction_ctor) is not in ${SDK_RELOC}")
endif()
string(REPLACE "${SDK_ROWS_MARKER}" "${SDK_ROWS_MARKER}        // --- cp2077-codeware/macos/MacSdkRows.inc ---\n${CW_ROWS_TEXT}        // --- new_game_plus/macos/NgpSdkRows.inc ---\n${SDK_ROWS_TEXT}" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")
set(SDK_TAIL_START "    // Address not found or placeholder - return a non-zero sentinel value")
set(SDK_TAIL_END "    return 0;\n#else\n    if constexpr (Detail::AddressResolverOverride<uint32_t>::value)")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_START}" SDK_TAIL_A)
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_END}" SDK_TAIL_B)
if(SDK_TAIL_A EQUAL -1 OR SDK_TAIL_B EQUAL -1)
    message(FATAL_ERROR "the unknown-hash tail of Resolve was not found in ${SDK_RELOC}")
endif()
string(SUBSTRING "${SDK_RELOC_TEXT}" 0 ${SDK_TAIL_A} SDK_HEAD)
math(EXPR SDK_REST_FROM "${SDK_TAIL_B} + 14")
string(SUBSTRING "${SDK_RELOC_TEXT}" ${SDK_REST_FROM} -1 SDK_REST)
set(SDK_RELOC_TEXT "${SDK_HEAD}    // New Game Plus: unknown hash logs once and returns a stub. Never returns 0.\n    return ngplus_mac_trap_for_hash(aHash);\n${SDK_REST}")
set(SDK_DECL_MARKER "extern \"C\" uintptr_t tweakxl_class_vslot(uint32_t aByteOffset);\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_DECL_MARKER}" SDK_DECL_POS)
if(SDK_DECL_POS EQUAL -1)
    message(FATAL_ERROR "the extern declaration anchor was not found in ${SDK_RELOC}")
endif()
string(REPLACE "${SDK_DECL_MARKER}" "${SDK_DECL_MARKER}extern \"C\" uintptr_t ngplus_mac_trap_for_hash(uint32_t aHash);\n" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")
file(WRITE "${SDK_OVERLAY}/RED4ext/Relocation-inl.hpp" "${SDK_RELOC_TEXT}")

# Mac DynArray stores length in a uint32_t field named size. Upstream NG+ calls size().
# NgpSizeWord is 4 bytes, converts to uint32_t, and is callable, so both forms work.
set(SDK_DYN "${SDK_INCLUDE}/RED4ext/DynArray.hpp")
file(READ "${SDK_DYN}" SDK_DYN_TEXT)
set(SDK_DYN_ANCHOR "namespace RED4ext\n{\nnamespace Memory")
string(FIND "${SDK_DYN_TEXT}" "${SDK_DYN_ANCHOR}" SDK_DYN_POS)
if(SDK_DYN_POS EQUAL -1)
    message(FATAL_ERROR "DynArray namespace anchor was not found in ${SDK_DYN}")
endif()
set(SDK_DYN_INJECT "namespace RED4ext\n{\nstruct NgpSizeWord\n{\n    uint32_t value;\n\n    constexpr NgpSizeWord() noexcept : value(0) {}\n    constexpr NgpSizeWord(uint32_t aValue) noexcept : value(aValue) {}\n    constexpr operator uint32_t() const noexcept { return value; }\n    constexpr uint32_t operator()() const noexcept { return value; }\n    constexpr NgpSizeWord& operator=(uint32_t aValue) noexcept { value = aValue; return *this; }\n    constexpr NgpSizeWord& operator++() noexcept { ++value; return *this; }\n    constexpr uint32_t operator++(int) noexcept { uint32_t old = value; ++value; return old; }\n    constexpr NgpSizeWord& operator--() noexcept { --value; return *this; }\n    constexpr uint32_t operator--(int) noexcept { uint32_t old = value; --value; return old; }\n    constexpr NgpSizeWord& operator+=(uint32_t aValue) noexcept { value += aValue; return *this; }\n    constexpr NgpSizeWord& operator-=(uint32_t aValue) noexcept { value -= aValue; return *this; }\n};\nstatic_assert(sizeof(NgpSizeWord) == sizeof(uint32_t));\nstatic_assert(alignof(NgpSizeWord) == alignof(uint32_t));\n\nnamespace Memory")
string(REPLACE "${SDK_DYN_ANCHOR}" "${SDK_DYN_INJECT}" SDK_DYN_TEXT "${SDK_DYN_TEXT}")
set(SDK_DYN_SIZE "    uint32_t size;     // 0C")
string(FIND "${SDK_DYN_TEXT}" "${SDK_DYN_SIZE}" SDK_DYN_SIZE_POS)
if(SDK_DYN_SIZE_POS EQUAL -1)
    message(FATAL_ERROR "DynArray size field was not found in ${SDK_DYN}")
endif()
string(REPLACE "${SDK_DYN_SIZE}" "    NgpSizeWord size;  // 0C" SDK_DYN_TEXT "${SDK_DYN_TEXT}")
file(WRITE "${SDK_OVERLAY}/RED4ext/DynArray.hpp" "${SDK_DYN_TEXT}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SDK_RELOC}" "${SDK_ROWS_INC}" "${CW_ROWS_INC}" "${SDK_DYN}")

# IGameSystem as Codeware's macOS port declares it. The game returns the SaveLock of IsSavingLocked through x8 and
# passes the bool in x1; the SDK's Windows signature makes a system write through a null pointer on the first save.
# NewGamePlusSystem derives from IGameSystem, so it uses the same header.
foreach(CW_INC_FILE gameIGameSystem.hpp gameIGameSystem-inl.hpp)
    set(CW_INC_SRC "${CODEWARE_DIR}/macos/inc/RED4ext/Scripting/Natives/${CW_INC_FILE}")
    if(NOT EXISTS "${CW_INC_SRC}")
        message(FATAL_ERROR "no Codeware macOS IGameSystem header at ${CW_INC_SRC}")
    endif()
    configure_file("${CW_INC_SRC}" "${SDK_OVERLAY}/RED4ext/Scripting/Natives/${CW_INC_FILE}" COPYONLY)
endforeach()

# RedLib: copy the headers the In-World Navigation port uses, then overlay the clang registration fixes.
set(REDLIB_GEN "${CMAKE_CURRENT_BINARY_DIR}/redlib")
set(REDLIB_SRC "${REDLIB_ROOT}/include")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${REDLIB_SRC}" "${REDLIB_GEN}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Registrar.hpp"
    "${REDLIB_GEN}/Red/TypeInfo/Registrar.hpp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Resolving.hpp"
    "${REDLIB_GEN}/Red/TypeInfo/Resolving.hpp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Macros/Definition.hpp"
    "${REDLIB_GEN}/Red/TypeInfo/Macros/Definition.hpp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/Utils.hpp"
    "${REDLIB_GEN}/Red/Utils.hpp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Definition.hpp"
    "${REDLIB_GEN}/Red/TypeInfo/Definition.hpp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/Framework.hpp"
    "${REDLIB_GEN}/Red/Framework.hpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Registrar.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Resolving.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Macros/Definition.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/Utils.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/TypeInfo/Definition.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/redlib/Red/Framework.hpp")

set(VERSION "${PROJECT_VERSION}")
configure_file("${NGP_DIR}/config/projectTemplate.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/Config/ProjectTemplate.hpp")

set(SRC "${NGP_DIR}/src")
set(SP_CODE "${SHARED_PUNK_DIR}/src/code")
set(NGP_SOURCES
    ${SRC}/Main.cpp
    ${SRC}/Hooking/Hooking.cpp
    ${SRC}/Filesystem/SaveFS.cpp
    ${SRC}/GameSystem/NewGamePlusSystem.cpp
    ${SRC}/GameSystem/Definitions/PlayerSaveData.cpp
    ${SRC}/GameSystem/DataReaders/CraftingSystem/CraftingSystemReader.cpp
    ${SRC}/GameSystem/DataReaders/EquipmentExOutfitSystem/OutfitSystemReader.cpp
    ${SRC}/GameSystem/DataReaders/EquipmentExViewManager/ViewManagerReader.cpp
    ${SRC}/GameSystem/DataReaders/EquipmentSystem/EquipmentSystemReader.cpp
    ${SRC}/GameSystem/DataReaders/Inventory/InventoryReader.cpp
    ${SRC}/GameSystem/DataReaders/PlayerDevelopmentSystem/PlayerDevelopmentSystemReader.cpp
    ${SRC}/GameSystem/DataReaders/StatsSystem/StatsSystemReader.cpp
    ${SRC}/GameSystem/DataReaders/VehicleGarage/VehicleGarageReader.cpp
    ${SRC}/GameSystem/DataReaders/WardrobeSystem/WardrobeReader.cpp
    ${SRC}/Parsing/Definitions/NodeParsers/ParserHelper.cpp
    ${SRC}/Parsing/Definitions/NodeParsers/Quest/FactsDBNode.cpp
    ${SRC}/Parsing/Definitions/NodeParsers/Scriptable/ScriptableContainerNodeV2.cpp
    ${SRC}/Parsing/Definitions/NodeParsers/Stats/StatsSystemNode.cpp
    ${SRC}/Parsing/Definitions/NodeParsers/Wardrobe/WardrobeSystemNode.cpp
    ${SRC}/Parsing/Definitions/Package/PackageReader.cpp
    ${SRC}/Parsing/FileReader.cpp
    ${SRC}/Util/Migration/Migration.cpp
    ${SRC}/Util/Settings/SettingsAccessor.cpp
    ${SP_CODE}/Hooks/HookingDriver.cpp
    ${SP_CODE}/Mac/FileAbi.cpp
    ${SP_CODE}/Raw/FileSystem/FileSystem.cpp
    ${SP_CODE}/Raw/Ink/InkSystem.cpp
    ${SP_CODE}/Raw/Save/Save.cpp
    ${SP_CODE}/Util/BufferCreator.cpp
    MacShims.cpp
    MacVault.cpp
    MacSessionDesc.cpp
    NgpGate.cpp
    NgpMetadata.cpp
    NgpGameInstance.cpp
)

add_library(NewGamePlus SHARED ${NGP_SOURCES})
set_target_properties(NewGamePlus PROPERTIES OUTPUT_NAME "NewGamePlus" PREFIX "" SUFFIX ".dylib")

target_include_directories(NewGamePlus PRIVATE
    "${SDK_OVERLAY}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${SDK_INCLUDE}"
    "${SRC}"
    "${SHARED_PUNK_DIR}/src/include"
    "${SHARED_PUNK_DIR}/src/include/Public"
    "${REDLIB_GEN}"
    "${REDLIB_ROOT}/vendor"
    "${HOPSCOTCH_DIR}/include"
    "${LZ4_INCLUDE}"
    "${NLOHMANN_JSON_INCLUDE}"
)

target_link_libraries(NewGamePlus PRIVATE "${LZ4_STATIC}")

target_compile_options(NewGamePlus PRIVATE
    -fexceptions
    -fms-extensions
    -Wno-deprecated-declarations
    -Wno-microsoft-template
    -Wno-invalid-offsetof
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/MacPrelude.hpp"
)

add_custom_command(TARGET NewGamePlus POST_BUILD
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/fix-install-names.sh" "$<TARGET_FILE:NewGamePlus>"
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/check-undefined.sh" "$<TARGET_FILE:NewGamePlus>"
    VERBATIM
)

message(STATUS "New Game Plus ${PROJECT_VERSION} configured for macOS")
