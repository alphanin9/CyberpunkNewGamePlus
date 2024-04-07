#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../parsing/fileReader.hpp"
#include "../filesystem/fs_util.hpp"

namespace redscript {
	struct PlayerSaveData {
		// We do not save actual perks - what if the player wants a respec?
		// Excessive cyberware can be fixed by not giving it all in one go 
		// and debug cyberware capacity shards
		int m_playerPerkPoints{};
		// Not sure how progression builds handle this, but you get Relic perks even without doing anything there
		int m_playerRelicPoints{};
		int m_playerAttributePoints{};

		// Cap this to 50 on the scripting side, so the player still has something to level towards
		int m_playerLevel{};

		// Special treatment :D
		int m_playerMoney{};

		// No other inventories matter too much
		// Johnny and Kurtz get their own shit anyway
		// Filter stash items from useless shit like vehicle weapons 
		// in native or on the Redscript side?
		Red::DynArray<Red::ItemID> m_playerItems{};
		Red::DynArray<Red::ItemID> m_playerStashItems{};

		// Most distinctive cyberware needs to be equipped 
		// by the scripting side to make 
		// the player feel at home from the get-go
		// Clothing does not, as the endpoint in Q101
		// will still end up with the player naked
		Red::ItemID m_playerEquippedOperatingSystem{};
		Red::ItemID m_playerEquippedKiroshis{};
		Red::ItemID m_playerEquippedLegCyberware{};
		Red::ItemID m_playerEquippedArmCyberware{};

		// Filter this from quest vehicles?
		// (Quadra, Demiurge, ETC)
		// Nah, shit idea, maybe for the Demiurge
		// Seeing as acquiring it is pretty cool
		Red::DynArray<Red::TweakDBID> m_playerVehicleGarage{};
	};

	class TopSecretSystem : public Red::IGameSystem {
	public:
		bool isAttached() const {
			return m_isAttached;
		}

		bool parsePointOfNoReturnSaveData() {
			auto savePath = files::findLastPointOfNoReturnSave(files::getCpSaveFolder()) / L"sav.dat";

			parser::Parser parser{};

			return parser.parseSavegame(savePath);
		}
	private:
		void OnWorldAttached(Red::world::RuntimeScene* aScene) {
			m_isAttached = true;
		}

		void OnWorldDetached(Red::world::RuntimeScene* aScene) {
			m_isAttached = false;
		}

		bool m_isAttached{};

		RTTI_IMPL_TYPEINFO(TopSecretSystem);
		RTTI_IMPL_ALLOCATOR();
	};
}

RTTI_DEFINE_CLASS(redscript::TopSecretSystem, {
	RTTI_METHOD(isAttached);
	RTTI_METHOD(parsePointOfNoReturnSaveData);
});

RTTI_DEFINE_CLASS(redscript::PlayerSaveData, {
	RTTI_PROPERTY(m_playerPerkPoints);
	RTTI_PROPERTY(m_playerRelicPoints);
	RTTI_PROPERTY(m_playerAttributePoints);
	RTTI_PROPERTY(m_playerLevel);
	RTTI_PROPERTY(m_playerMoney);
	RTTI_PROPERTY(m_playerItems);
	RTTI_PROPERTY(m_playerStashItems);
	RTTI_PROPERTY(m_playerEquippedOperatingSystem);
	RTTI_PROPERTY(m_playerEquippedKiroshis);
	RTTI_PROPERTY(m_playerEquippedLegCyberware);
	RTTI_PROPERTY(m_playerEquippedArmCyberware);
	RTTI_PROPERTY(m_playerVehicleGarage);
});