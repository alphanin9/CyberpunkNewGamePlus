// macOS: what the RED4ext.SDK Mac address table needs from the plugin, compiled once (RED4ext/Mac/PluginSupport.hpp): the memory
// vault, the CClass virtual slots and the two id counters, plus New Game Plus's own address rows (NgpSdkRows.inc) and its trap for
// a hash with no row (MacShims.cpp).
#define RED4EXT_MAC_PLUGIN_SUPPORT_IMPL
// New Game Plus's id bands. Each Mac plugin has its own, so no two plugins hand out the same id.
#define RED4EXT_MAC_RTTI_ASYNC_ID_BASE 0x7FF80000
#define RED4EXT_MAC_SERIALIZABLE_ID_BASE 0x7FC0000000000000 // bits 62 through 54
#define RED4EXT_MAC_PLUGIN_ROWS "NgpSdkRows.inc"
#define RED4EXT_MAC_UNKNOWN_HASH_HANDLER ngplus_mac_trap_for_hash
#define RED4EXT_MAC_LOG_PREFIX "[ngplus]"
#define RED4EXT_MAC_DIAG_ENV "NGPLUS_MAC_DIAG"
#include <RED4ext/Mac/PluginSupport.hpp>
