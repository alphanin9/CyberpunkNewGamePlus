#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../parsing/fileReader.hpp"
#include "../filesystem/fs_util.hpp"

namespace redscript {
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