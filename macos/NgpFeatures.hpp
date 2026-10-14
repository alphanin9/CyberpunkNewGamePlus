#pragma once

// Apple feature gates. A gate is false until every hash it names resolves to a real Mac row.
// Adding that row turns the feature on. No other code change.

namespace RED4ext::save
{
struct Metadata;
}

namespace ngp::mac
{
// SetFact and QuestMutex. QuestsSystem's mutex is at +0x60 on macOS too (the game's lock function at QuestsSystem vtable
// +0x238 takes this+0x60). FactsDBManager's virtuals sit one slot later than on Windows (the game's quest load calls
// Windows +0x50 at +0x58), and clang puts SharedPunk's SetFact at +0x20 for the same reason (two destructor slots).
inline constexpr bool kEnableUnconfirmedFactAndMutex = true;

bool SaveReadingAvailable();
bool InteriorsAvailable();
bool LaunchAvailable();
bool NodeAccessorAvailable();
bool AddQuestAvailable();

// Reads Data.metadata of a save's metadata.9.json into the fields New Game Plus uses (NgpMetadata.cpp).
bool ReadSaveMetadataJson(const char* aPath, RED4ext::save::Metadata& aMetadata) noexcept;

void LogUnconfirmed(const char* aSite);
void LogDedupeSkipped();
}
