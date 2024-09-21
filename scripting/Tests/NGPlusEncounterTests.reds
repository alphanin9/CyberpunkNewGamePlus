
public native class NGPlusDynamicSpawnSystem {
    public native static func RequestDynamicSpawnSystemSpawn(ids: [TweakDBID]) -> Void;

    public native static func RequestDynamicSpawnSystemDespawn() -> Void;
}

public exec static func RunJackieEncounter() {
    let arr = [
        t"DynamicSpawnSystem.NewGamePlus_WellesBossEncounter",
        t"DynamicSpawnSystem.NewGamePlus_WellesBossEncounter_Car2",
        t"DynamicSpawnSystem.NewGamePlus_WellesBossEncounter_Car3"
    ];

    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunBlackhandEncounter() {
    // Todo...
}

public exec static func RunBrightmanEncounter() {
    // Todo...
    // Similar to Burondo, but with more weaker Arasaka troops?
}

public exec static func RunBurondoEncounter() {
    let arr = [
        t"DynamicSpawnSystem.NewGamePlus_NetrunnerBossEncounter",
        t"DynamicSpawnSystem.NewGamePlus_NetrunnerBossSupport",
        t"DynamicSpawnSystem.NewGamePlus_NetrunnerBossSupport2",
        t"DynamicSpawnSystem.NewGamePlus_NetrunnerBossSupport3"
    ];
    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunAyumiEncounter() {
    let arr = [
        t"DynamicSpawnSystem.NewGamePlus_FastSoloBossEncounter",
        t"DynamicSpawnSystem.NewGamePlus_FastSoloBossSupport"
    ];
    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunCrossEncounter() {
    let arr = [
        t"DynamicSpawnSystem.NewGamePlus_StrongSoloBossEncounter",
        t"DynamicSpawnSystem.NewGamePlus_StrongSoloBossSupport",
        t"DynamicSpawnSystem.NewGamePlus_StrongSoloBossSupport1"
    ];
    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunRangedHordeEncounter() {
    let arr: [TweakDBID];

    let i = 0;
    let carCount = 16;
    while i < carCount {
        ArrayPush(arr, t"DynamicSpawnSystem.NewGamePlus_MilitechShootyMooks");

        i += 1;
    }

    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunMeleeHordeEncounter() {
    let arr: [TweakDBID];

    let i = 0;
    let carCount = 19;
    while i < carCount {
        ArrayPush(arr, t"DynamicSpawnSystem.NewGamePlus_MaelstromPunchyMooks");

        i += 1;
    }

    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}

public exec static func RunNetrunnerHordeEncounter() {
    let arr: [TweakDBID];
    
    // Aim for about 25 to 30-ish runners?
    // More will get problematic quickly...

    // 4 runners per car, 6 comes out to 24

    let i = 0;
    while i < 6 {
        ArrayPush(arr, t"DynamicSpawnSystem.NewGamePlus_NetrunnerClownCarMooks");
        i += 1;
    }

    NGPlusDynamicSpawnSystem.RequestDynamicSpawnSystemSpawn(arr);
}
