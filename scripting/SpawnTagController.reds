module NGPlus.SpawnTags

public class NewGamePlusSpawnTagController {
    public static func SetSpawnTags(newSpawnTag: CName) {
        TweakDBManager.SetFlat(t"LifePaths.Nomad.newGameSpawnTag", newSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.Nomad");
        TweakDBManager.SetFlat(t"LifePaths.Corporate.newGameSpawnTag", newSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.Corporate");
        TweakDBManager.SetFlat(t"LifePaths.StreetKid.newGameSpawnTag", newSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.StreetKid");
    }

    public static func RestoreSpawnTags() {
        // Yes, those are hardcoded
        // This is not actually a problem, seeing as lifepath spawn tags are unlikely to change in our lifetime

        let nomadSpawnTag = n"#q000_nomad_wp_garage_start";
        let corpoSpawnTag = n"#q000_corpo_spwn_player_toilet_female";
        let streetKidSpawnTag = n"#q000_kid_spawn_start";

        TweakDBManager.SetFlat(t"LifePaths.Nomad.newGameSpawnTag", nomadSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.Nomad");
        TweakDBManager.SetFlat(t"LifePaths.Corporate.newGameSpawnTag", corpoSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.Corporate");
        TweakDBManager.SetFlat(t"LifePaths.StreetKid.newGameSpawnTag", streetKidSpawnTag);
        TweakDBManager.UpdateRecord(t"LifePaths.StreetKid");
    }
}