#include <algorithm>
#include <ranges>
#include <unordered_set>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/StatsStateMapStructure.hpp>
#include <RED4ext/Scripting/Natives/Generated/save/MetadataContainer.hpp>

#include "SaveFS.hpp"

#include <context/context.hpp>

#include <Shared/RTTI/PropertyAccessor.hpp>
#include <Shared/Raw/FileSystem/FileSystem.hpp>
#include <Shared/Raw/Package/ScriptableSystemsPackage.hpp>
#include <Shared/Raw/Save/Save.hpp>
#include <util/settings/settingsAccessor.hpp>

using namespace Red;

namespace files
{
CString GetRedPathToSaveFile(const char* aSaveName, const char* aFileName) noexcept
{
    static const auto& rootPath = shared::raw::Filesystem::Path::GetSaveFolder();

    CString saveFolder{};

    shared::raw::Filesystem::Path::MergeDirToPath(rootPath, &saveFolder, aSaveName);

    CString filePath{};

    shared::raw::Filesystem::Path::MergeFileToPath(saveFolder, &filePath, aFileName);

    return filePath;
}

void HasNewGamePlusSaveAsync(WeakHandle<IScriptable> aTarget, CName aCallback) noexcept
{
    static auto s_fileManager = shared::raw::Filesystem::RedFileManager::GetInstance();
    static const auto& s_saveFolder = shared::raw::Filesystem::Path::GetSaveFolder();

    JobQueue{}.Dispatch(
        [aTarget, aCallback](const JobGroup& aGroup)
        {
            if (aTarget.Expired())
            {
                return;
            }

            auto ref = aTarget.Lock();

            auto filePaths = s_fileManager->FindFilesByName(s_saveFolder, c_metadataFileName);

            PluginContext::DebugLog("[HasNewGamePlusAsync] Save count: {}", filePaths.size);

            for (const auto& i : filePaths)
            {
                // Having job for each save would likely cost a lot
                // So, we only have one job to offset cost a little

                PluginContext::DebugLog("[HasNewGamePlusSaveAsync {}] Processing save", i.c_str());

                if (IsValidForNewGamePlus(i))
                {
                    PluginContext::DebugLog("[HasNewGamePlusSaveAsync {}] Save was OK", i.c_str());
                    CallVirtual(ref, aCallback, true);
                    return;
                }
            }

            PluginContext::DebugLog("[HasNewGamePlusSaveAsync] No OK saves found!");

            CallVirtual(ref, aCallback, false);
        });
}

// Should be using saveVersion instead, but I don't know the proper save version for 2.00
constexpr std::int64_t c_minSupportedGameVersion = 2000;

// Hacky-ish way to get save selection to filter saves that are potentially bad (Post-epilogue, for example...)
constexpr std::array c_generatedPostPointOfNoReturnObjectives = {
    // Q307, some are stripped due to being accessible in basegame... (Not sure if I stripped all the right ones,
    // though?)
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/02_confirm_pickup"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/02c_talk_johnny"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/03c_text_friends"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/03a_wait_fia"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/02f_q115_abort"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/03b_talk_fia"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/04_enter_av"),
    FNV1a64("ep1/quests/main_quest/q307_before_tomorrow/00_hook/05_go_clinic"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/01_talk_reed"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/06a_listen_radio"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/02_go_chair"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/03b_call_more_friends"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/02a_get_up"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/03_call_friends"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/04_call_receptionist"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/04_talk_delamain"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/05_meet_viktor"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/05_wait_pickup"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/01_hospital_cabride/06b_leave_delamain"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/00_talk_zetatech"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/01_talk_viktor"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/01b_sit_viktor"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/02_leave_viktor"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/03_return_delamain"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/03a_deal_thugs"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/03b_wait_help"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/04_talk_misty"),
    FNV1a64("ep1/quests/main_quest/q307_tomorrow/02_mistys_alley/05_take_walk"),

    // Q113
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00_follow_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00b_get_out"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00c2_talk"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00c3_talk_jackie"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00c_talk_to_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00d_get_in_elevator"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/00e_sit_mikoshi"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/01_meeting"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/01a_shoo"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/01b_sit"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/02_survive"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/02b_rest"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/02c_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/03_elevator"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/02_arasaka_tower/03b_wait_takemura"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/05_top_atrium/01_get_to_elevator"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/02_go_to_boss"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/03c_defeat_men"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/05b_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/06_kasai"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/03b_decide_adam"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/02b_defeat_men"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/03_defeat_adam"),
    FNV1a64("quests/main_quest/act_01/q113_corpo/06_ceo_floor/05_confront_yorinobu"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/00_to_estate/06_drive_to_estate"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/00_to_estate/06b_deal_guard"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/00_to_estate/06c_hellman"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/00_to_estate/06d_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/00_to_estate/07_get_out"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/00_enter_estate"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/01_find_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/01b_defeat_guards"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/01c_defeat_rest"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/02_talk_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/02b_follow_hanako"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/03_get_in_av"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/01d_takemura"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/04_fly_to_arasaka"),
    FNV1a64("quests/main_quest/act_01/q113_rescuing_hanako/01_estate/01d2_takedown"),

    // Q114
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/enter_car"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/follow_mitch"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/meet_panam"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/plan"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/travel_nomad_camp"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/new_camp_wake_up/wake_up_talk_panam"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/line"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/sit_down"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/party"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/shoot_bottles"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/find_panam"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/party/talk_panam"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/board_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/dakota_alt_prep"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/dakota_carol_wrap"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/destroywrecks"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/drive_to_spot"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/enter_bathtub"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/follow_to_initiation"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/mitch_panzer_prep"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/mitch_panzer_talk"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/punch_through"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/return_to_camp"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/return_to_saulpanam"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/panzer"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/take_part_in_initiation"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/talk_alt"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/talk_to_crowd"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/talk_about_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/talk_to_panam_initiation"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/vendor"),
    FNV1a64("quests/main_quest/act_01/q114_01_nomad_initiation/preparations/warm_up"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/brief_nomads"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/drone"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/panam"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/panzer1"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/plan1"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/splinter"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/gear_up"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/03_planning/talk_to_panam"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/board_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/break_through"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/clear_bridge"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/clear_courtyard"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/continue_approach"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/convoy"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/defend_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/drive"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/gate"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/leave_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/santiago"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/04_approach/turrets"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/approach_controls"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/av"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/board_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/cleanup_site"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/construction"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/defeat_dropteam"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/defend"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/escort"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/intercept"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/leave_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/panzer"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/return"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/return_to_construction"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/tunnel1"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/tunnel_wait"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/wait_mitch_carol"),
    FNV1a64("quests/main_quest/act_01/q114_02_maglev_line_assault/05_construction/wait_regroup"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/deeperunderground"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/drill_wait"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/drive"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/enter_driller"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/hole"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/inspect_driller"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/leave_panzer"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/panzer"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/start_drill"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/talk_panam"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/talk_to_mitch"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/talk_to_saul_panam"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/wait"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/wait_for_driller"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/06_tunnel/walk_to_mitch_saul"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/climb_to_manufacturing"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/deeper"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/get_rid_of_transport"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/get_to_security"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/pass_hatch"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/search_dampening"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/secure_top_area"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/sneak_manufacturing"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/surveymanufacturing"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/talk_nomads_wrap"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/07_manufacturing/wait_for_saul"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/alt"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/breach_nest"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/enable_mainframe"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/mikoshi_floor"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/plan_approach"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/plug_alt"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/setup_splinter"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/08_netrunner/talk_nomads"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/00_access_mikoshi"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/00b_wait_for_alt"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/01_defeat_adam"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/03_go_to_access"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/04_talk_to_alt"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/02_decide_adam"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/05_jack_in"),
    FNV1a64("quests/main_quest/act_01/q114_03_attack_on_arasaka_tower/09_mikoshi/04b_talk_to_panam"),

    // Q115, Afterlife part
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/00_meet_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01c_grab_equipment"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01_talk_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/06c_prepare_for_fight"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01a_follow_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01b_grab_thrusters"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01d_grab_splinter"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01e_talk_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01f_get_up"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/01g_follow_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02_talk_alt"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02a_connect_to_chair"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02a_talk_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02b_put_on_suit"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02c2_get_up"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02c_go_net_room"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02e_talk_nix"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/02f_wait_nix"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/03_go_to_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/04_talk_with_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/06_follow_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/06b_elevator"),
    FNV1a64("quests/main_quest/act_01/q115_afterlife/02_afterlife/07_get_in_av"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/03_jump/00_fly"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/03_jump/01_jump_out"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/00_talk_to_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/02b_help_rogue_with_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/06_takedown_commander"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/07_loot_commander"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/08_get_to_elevator"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/02c_return_to_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/02d_talk_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/02e_takedown_first_guards"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/01_find_commander"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/06b_takedown_commander"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/08b_wait_rogue_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/09_take_elevator_down"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/02_follow_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/03_talk_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/04_takedown_av_guard"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/05_talk_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/06c_follow_rogue_to_commander"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/10_jump_down_shaft"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/11_get_in_elevator"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/04_jungle/12_jump_into_elev"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/0000_follow_rogue"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/000_atrium"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/00_get_down"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/01_security_room"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/02_lockdown"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/02b_connect_alt"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/03_elevator"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/05_atrium/04_take_elevator"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/00_access_mikoshi"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/00b_wait_for_alt"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/01_defeat_adam"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/03_go_to_access"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/04b_talk_to_weyland"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/04_talk_to_alt"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/02_decide_adam"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/05_jack_in"),
    FNV1a64("quests/main_quest/act_01/q115_rogues_last_flight/07_mikoshi/02b_rogue_gun"),

    // Q116
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/00_roof/01_go_to_roof"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/00_roof/02a_talk_jackie"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/00_roof/02b_pick_necklace"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/01_explore"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/02a_talk_v"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03b_cross_bridge_optional"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/02b_talk_johnny"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/02c_talk_johnny_alt"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03a_enter_well_optional"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/02d_talk_v_alt"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03_choose"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03a_enter_well"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03b_cross_bridge"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03c_touch_alt"),
    FNV1a64("quests/main_quest/act_01/q116_cyberspace/01_cyberspace/03c_touch_alt_optional"),

    // Q201
    FNV1a64("quests/main_quest/epilogues/q201_heir/02_operation_room/01_rest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/02_operation_room/01_wake_up"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/02_operation_room/02_follow_the_guard"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/02_operation_room/02b_talk_to_doctor"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/02_operation_room/05_investigate_noises"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/03_cooperate_with_doctor"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/04_link_to_suitcase"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/04_scan_letters"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/04_treadmill"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/05_squeeze"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/05_vktest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/06_rest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/07_investigate_noises"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/09_wake_up"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/10_walk_treadmill"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/11_nightmare_exit"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/04_cabin_day_1/12_listen_instructions"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/03_cooperate_with_doctor"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/04_treadmill"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/05_squeeze"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/05_squeeze1"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/05_vktest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/06_rest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/10_01_talk_to_dr_nightmare"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/11_wake_up"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/12_walk_treadmill"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/13_link_to_device"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/14_listen_instructions"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/15_wait_instructions"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/05_cabin_day_2/16_link_treadmill"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_1/02_follow_instructions"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_1/04_break_stuff"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_1/03_cooperate_with_doctor"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_1/16_wake_up"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_1/17_rest"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/01_talk_judy"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/02_talk_to_kerry"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/03_talk_to_panam"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/04_talk_to_river"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/05_talk_to_victor"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/06_cabin_day_3/00_call_friends"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/02_make_final_decision"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/03_follow_guard_ice"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/04_follow_guard_earth"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/05_take_bag"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/06_lie_on_bed"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/07_talk_haru"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/07_cabin_day_4/talk_to_haru"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/01_talk_to_takemura"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/02_make_final_decision"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/03_follow_guard_ice"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/04_follow_guard_earth"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/05_take_bag"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/06_enter_operation"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/08_cabin_day_30_takemura/07_talk_takemura"),
    FNV1a64("quests/main_quest/epilogues/q201_heir/space_station/farewell"),

    // Q202
    FNV1a64("quests/main_quest/epilogues/q202_nomads/motel/join_panam"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/motel/motel"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/motel/panams_car"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/badlands_drive"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/cross_border"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/follow_nomads"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/get_on_board"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/greet_mitch"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/prepare_plan"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/romance_surprise"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/say_goodbye"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/talk_judy"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/talk_kerry"),
    FNV1a64("quests/main_quest/epilogues/q202_nomads/temp_river/talk_panam"),

    // Q203
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/enter_afterlife"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/meet_contact"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/sit_with_blue_eye"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/talk_blue_eyes"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/drink"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/afterlife/talk_emmerick"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/activate_autopilot"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/depressurize"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/enter_airlock"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/exit_airlock"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/get_guns"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/get_helmet"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/leave_cabin"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/listen_to_briefing"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/wait_for_depressurization"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/cosmos/walk_with_blueeyes"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/enter_delemain"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/get_dressed"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/get_guns"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/get_katana"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/get_shower"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/join_river_balcony"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/join_river_kitchen"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/talk_ai"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/talk_judy"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/talk_kerry"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/talk_panam"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/talk_sobchak"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/grab_coffee"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/travel_to_afterlife"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/wait_for_delamain"),
    FNV1a64("quests/main_quest/epilogues/q203_legend/penthouse/put_on_clothes"),

    // Q204
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/01a_pack_bag"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/01d_take_rogue_gun"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/check_emails"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/leave_apartment"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/ring_the_door"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/take_ticket"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/talk_to_steve1"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/01_apartment/01d_take_jacket"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/deal_with_steves_dad"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/defeat_steves_dad"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/follow_steve"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/get_a_ride"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/get_into_car"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/talk_to_steve"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/02_dad/talk_to_steves_dad"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/02_enter_music_shop"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/08_put_guitar_into_trunk"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/buy_a_guitar"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/choose_a_guitar_to_buy"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/go_back_room"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/go_to_music_shop_with_steve"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/leave_the_store"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/talk_steve"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/talk_to_the_store_owner"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/talk_to_the_store_owner1"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/03_music_shop/try_out_the_guitar"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/00_car"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/01_drive_to_the_cementary_with_steve"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/02_check_map"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/06a_use_ticket"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/06b_put_bag"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/06c_sit_down"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/enter_bus"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/go_to_the_graves"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/leave_columbarium"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/leave_mementos"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/open_graves"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/pay_homage"),
    FNV1a64("quests/main_quest/epilogues/q204_reborn/04_columbarium/talk_steve"),

    // Q115, Hanako bit
    FNV1a64("quests/meta/02_sickness/q115/03_sit_hanako"), FNV1a64("quests/meta/02_sickness/q115/04_talk_hanako"),
    FNV1a64("quests/meta/02_sickness/q115/05_leave_restaurant"), FNV1a64("quests/meta/02_sickness/q115/06_talk_johnny"),
    FNV1a64("quests/meta/02_sickness/q115/07_reactivate_elevator"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/00_talk_johnny"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/00b_talk_johnny"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_0_leave_ripperdoc"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_1_talk_to_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_2_grab_inhalers"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_3_follow_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_3b_elevator"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_3c_follow"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_4_sit"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01_decide_johnny"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/01b_talk_johnny"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/02a1_call_river"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/02a2_call_kerry"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/02a3_call_judy"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/02a4_call_panam"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/02b_let_johnny_take_over"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/03a_call_panam"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/03b_call_hanako"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/04a_talk_to_panam"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/04b_talk_to_hanako"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/05_leave_roof"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/06_talk_to_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/07_follow_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/07b_talk_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/08_sit_misty"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/09_reading_chakras"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/10_get_up"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/11a_wait_for_takemura"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/11b_wait_for_takemura_2"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/11c_wait_for_panam"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/12a_talk_takemura"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/12b_talk_takemura_2"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/12c_talk_panam"),
    FNV1a64("quests/meta/02_sickness/q115_ripperdoc/13_leave_misty"),

    // Secret ending
    FNV1a64("quests/meta/09_solo/404/000_talk_johnny"), FNV1a64("quests/meta/09_solo/404/00b_find_way"),
    FNV1a64("quests/meta/09_solo/404/00c_gain_access_elev"), FNV1a64("quests/meta/09_solo/404/00d2_defeat_enemies"),
    FNV1a64("quests/meta/09_solo/404/00d_find_loot"), FNV1a64("quests/meta/09_solo/404/00e_take_elev"),
    FNV1a64("quests/meta/09_solo/404/01b_find_mikoshi"), FNV1a64("quests/meta/09_solo/404/01c2_clear_control"),
    FNV1a64("quests/meta/09_solo/404/01c_talk_johnny"), FNV1a64("quests/meta/09_solo/404/01d_connect_alt"),
    FNV1a64("quests/meta/09_solo/404/01e_to_mikoshi"), FNV1a64("quests/meta/09_solo/404/01f2_clear_nest"),
    FNV1a64("quests/meta/09_solo/404/01f_figure_out"), FNV1a64("quests/meta/09_solo/404/01g_raise_mainframe"),
    FNV1a64("quests/meta/09_solo/404/01h_wait_mainframe"), FNV1a64("quests/meta/09_solo/404/02_defeat_smasher"),
    FNV1a64("quests/meta/09_solo/404/02b_decide_smasher"), FNV1a64("quests/meta/09_solo/404/03_get_to_access"),
    FNV1a64("quests/meta/09_solo/404/04_jack_in")};

bool LoadSaveMetadata(const CString& aFilePath, save::Metadata& aMetadataObject) noexcept
{
    auto engineStream = shared::raw::Filesystem::RedFileManager::GetInstance()->OpenBufferedFileStream(aFilePath);

    if (!engineStream)
    {
        return false;
    }

    return shared::raw::SaveMetadata::LoadSaveMetadataFromStream(engineStream, aMetadataObject);
}

bool IsValidForNewGamePlus(const CString& aSaveFullPath, uint64_t& aPlaythroughHash) noexcept
{
    save::Metadata metadata{};

    if (!LoadSaveMetadata(aSaveFullPath, metadata))
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Failed to load save metadata", aSaveFullPath.c_str());
        return false;
    }

    constexpr CName c_pcPlatformName = "pc";
    constexpr CName c_steamDeckPlatformName = "steamdeck";

    const CName platform = metadata.platform.c_str();

    if (platform != c_pcPlatformName && platform != c_steamDeckPlatformName)
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Invalid platform", aSaveFullPath.c_str());
        return false;
    }

    if (c_minSupportedGameVersion > metadata.gameVersion)
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Game version is too old", aSaveFullPath.c_str());
        return false;
    }

    if (metadata.isEndGameSave)
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Ignoring end game save", aSaveFullPath.c_str());
        return false;
    }

    // Note: maybe for future implementation of an interesting idea, not sure how the technical side will work yet
    // It's going to be rad, though
    if (metadata.debugString == "pre_replay_ponr_save")
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Is pre-replay PONR save", aSaveFullPath.c_str());
        return false;
    }
    
    aPlaythroughHash = FNV1a64(metadata.playthroughID.c_str());

    if (std::find(c_generatedPostPointOfNoReturnObjectives.begin(), c_generatedPostPointOfNoReturnObjectives.end(),
                  FNV1a64(metadata.trackedQuestEntry.c_str())) != c_generatedPostPointOfNoReturnObjectives.end())
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Bad tracked quest objective {}", aSaveFullPath.c_str(),
                                metadata.trackedQuestEntry.c_str());
        return false;
    }

    using std::operator""sv;
    auto questsSplitRange = std::views::split(std::string_view{metadata.finishedQuests.c_str()}, " "sv);

    std::unordered_set<std::string_view> questsSet(questsSplitRange.begin(), questsSplitRange.end());

    if (questsSet.contains("q104") && questsSet.contains("q110") && questsSet.contains("q112"))
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Quests are OK", aSaveFullPath.c_str());
        return true;
    }

    if (metadata.isPointOfNoReturn)
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] PONR save", aSaveFullPath.c_str());
        return true;
    }

    // Do we have the ~~Songbird chip~~ ahem, *neural matrix* waiting for us?
    constexpr CName c_q307ActiveFact = "q307_blueprint_acquired=1";

    // Debug thing, allows us to unlock NG+ on any save...
    constexpr CName c_ngplusDebugUnlocker = "debug_unlock_ngplus=1";

    for (auto& i : metadata.facts)
    {
        const CName hashed = i.c_str();

        if (hashed == c_q307ActiveFact || hashed == c_ngplusDebugUnlocker)
        {
            PluginContext::DebugLog("[IsValidForNewGamePlus {}] NG+ activating fact found", aSaveFullPath.c_str());
            return true;
        }
    }

    // Note: sorta expensive, but we instantiate + parse RTTI class here anyway, so who cares?
    const auto settings = settings::GetModSettings();

    PluginContext::DebugLog("[IsValidForNewGamePlus {}] Save is not good", aSaveFullPath.c_str());
    if (settings.m_disableSaveFileValidation)
    {
        PluginContext::DebugLog("[IsValidForNewGamePlus {}] Save is not good, but we don't care!",
                                aSaveFullPath.c_str());
    }

    return settings.m_disableSaveFileValidation;
}

bool IsValidForNewGamePlus(const CString& aSaveFullPath) noexcept
{
    uint64_t dummy{};
    return IsValidForNewGamePlus(aSaveFullPath, dummy);
}

bool ReadSaveFileToBuffer(const Red::CString& aSaveName, std::vector<std::byte>& aBuffer) noexcept
{
    const auto fileManager = shared::raw::Filesystem::RedFileManager::GetInstance();

    if (!fileManager)
    {
        return false;
    }

    // Use engine reader to read file to buffer, since fstream seems to be failing...
    auto filePath = GetRedPathToSaveFile(aSaveName.c_str(), c_saveFileName);
    auto stream = fileManager->OpenBufferedFileStream(filePath);

    if (!stream)
    {
        return false;
    }

    const auto fileSize = stream->GetLength();

    // At this point, why not just use std::unique_ptr or some new struct for RAII of game allocator results? We don't
    // care about vector's additional stuff anyway
    aBuffer = std::vector<std::byte>(fileSize);

    stream->ReadWrite(&aBuffer[0], static_cast<std::uint32_t>(fileSize));

    // Tests look OK, can move to this from WKit way once we figure out FDB/persistency/inventory
    constexpr auto TestSaveStream = false;

    if constexpr (TestSaveStream)
    {
        save::Metadata metadata{};

        if (!LoadSaveMetadata(GetRedPathToSaveFile(aSaveName.c_str(), c_metadataFileName), metadata))
        {
            return true;
        }

        auto saveStream = shared::raw::Filesystem::RedFileManager::GetInstance()->OpenBufferedFileStream(
            GetRedPathToSaveFile(aSaveName.c_str(), c_saveFileName));

        auto loadStream = shared::raw::Save::Stream::LoadStream::Create(saveStream, metadata);
        loadStream.Initialize();

        {
            shared::raw::Save::NodeAccessor sessionDesc(loadStream, "GameSessionDesc", true, false);

            if (sessionDesc.IsGood())
            {
                shared::raw::Save::NodeAccessor sessionConfig(loadStream, "game::SessionConfig", true, false);

                if (sessionConfig.IsGood())
                {
                    PluginContext::Spew("Ebin");

                    Red::ResourcePath path1{};
                    Red::ResourcePath path2{};

                    loadStream->ReadWriteEx(&path1);
                    loadStream->ReadWriteEx(&path2);

                    PluginContext::Spew("Path 1: {}", path1.hash);
                    PluginContext::Spew("Path 2: {}", path2.hash);
                }
            }
        }

        {
            shared::raw::Save::NodeAccessor scriptableSystemsContainer(loadStream, "ScriptableSystemsContainer", true,
                                                                       false);

            if (scriptableSystemsContainer.IsGood())
            {
                auto buffer = loadStream.ReadBuffer();

                shared::raw::ScriptablePackage::ScriptablePackageReader reader(buffer);

                PackageHeader header{};

                // The game does this, don't ask
                reader.ReadHeader(header);
                reader.ReadHeader(header);

                shared::raw::ScriptablePackage::ScriptablePackageExtractor extractor(header);

                constexpr auto c_testClassName = "PlayerDevelopmentSystem";
                auto ref = MakeScriptedHandle(GetClass<"PlayerDevelopmentSystem">());

                std::uint32_t classIndex{};

                for (; classIndex < reader.rootChunkTypes.size; classIndex++)
                {
                    if (reader.rootChunkTypes[classIndex] == c_testClassName)
                    {
                        break;
                    }
                }

                extractor.GetObjectById(ref, classIndex);

                auto& data = shared::rtti::GetClassProperty<DynArray<Handle<IScriptable>>, "playerData">(ref);

                PluginContext::Spew("{}", header.size);
                PluginContext::Spew("PDS data size: {}", data.size);

                if (data.size > 0u)
                {
                    PluginContext::Spew("Data class name: {}", data[0]->GetType()->name.ToString());
                }
            }
        }

        {
            shared::raw::Save::NodeAccessor statsSystem(loadStream, "StatsSystem", true, false);

            if (statsSystem.IsGood())
            {
                auto stateMap = loadStream.ReadPackage(GetClass<game::StatsStateMapStructure>());

                // It is what it is, should fix it sometime
                auto& ptr = *reinterpret_cast<game::StatsStateMapStructure*>(stateMap.instance);

                PluginContext::Spew("Keys: {}, values: {}", ptr.keys.size, ptr.values.size);
            }
        }
    }

    return true;
}
} // namespace files
