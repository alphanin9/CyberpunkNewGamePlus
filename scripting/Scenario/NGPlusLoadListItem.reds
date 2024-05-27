module NGPlus.LoadListItem

@addMethod(LoadListItem)
public final func SetMetadataForNGPlus(metadata: ref<SaveMetadataInfo>, opt isEp1Enabled: Bool) -> Void {
    let finalString: String;
    let hrs: Int32;
    let lvl: Int32;
    let mins: Int32;
    let playthroughTime: Float;
    let shrs: String;
    let smins: String;
    this.m_metadata = metadata;
    inkWidgetRef.SetVisible(this.m_wrapper, true);
    inkWidgetRef.SetVisible(this.m_label, true);
    inkWidgetRef.SetVisible(this.m_labelDate, true);
    inkWidgetRef.SetVisible(this.m_type, true);
    inkWidgetRef.SetVisible(this.m_playTime, true);
    inkWidgetRef.SetVisible(this.m_imageReplacement, true);
    inkWidgetRef.SetVisible(this.m_lifepath, true);
    inkWidgetRef.SetVisible(this.m_cloudStatus, true);
    inkWidgetRef.SetVisible(this.m_level, true);
    inkWidgetRef.SetVisible(this.m_quest, true);
    inkWidgetRef.SetVisible(this.m_characterLevel, true);
    inkWidgetRef.SetVisible(this.m_characterLevelLabel, true);
    inkWidgetRef.SetVisible(this.m_gameVersion, true);
    if !isEp1Enabled && metadata.IsEp1Save() {
        // Fix: non-EP1 builds have odd behavior with this
        inkTextRef.SetText(this.m_label, "LocKey#92500");
        inkWidgetRef.SetVisible(this.m_quest, false);
        inkWidgetRef.SetVisible(this.m_type, false);
    } else {
        inkTextRef.SetText(this.m_label, metadata.trackedQuest);
        inkTextRef.SetText(this.m_quest, metadata.internalName);
        inkTextRef.SetText(this.m_type, metadata.locationName);
    };
    this.SetEnabled(true);
    this.m_versionParams.UpdateString("version_num", metadata.gameVersion);
    this.m_initialLoadingID = metadata.initialLoadingScreenID;
    playthroughTime = MaxF(Cast<Float>(metadata.playthroughTime), Cast<Float>(metadata.playTime));
    hrs = RoundF(playthroughTime / 3600.00);
    mins = RoundF((playthroughTime % 3600.00) / 60.00);
    if hrs > 9 {
      shrs = ToString(hrs);
    } else {
      shrs = ToString(hrs);
    };
    if mins > 9 {
      smins = ToString(mins);
    } else {
      smins = ToString(mins);
    };
    if hrs != 0 {
      finalString = shrs + GetLocalizedText("UI-Labels-Units-Hours");
    };
    if mins != 0 {
      finalString = hrs != 0 ? finalString + " " : finalString;
      finalString = finalString + smins + GetLocalizedText("UI-Labels-Units-Minutes");
    };
    inkTextRef.SetText(this.m_playTime, finalString);
    inkTextRef.SetDateTimeByTimestamp(this.m_labelDate, metadata.timestamp);
    lvl = RoundF(Cast<Float>(metadata.level));
    inkTextRef.SetText(this.m_characterLevel, ToString(lvl));
    if lvl == 0 {
      inkWidgetRef.SetVisible(this.m_characterLevel, false);
      inkWidgetRef.SetVisible(this.m_characterLevelLabel, false);
    };
    if Equals(metadata.lifePath, inkLifePath.Corporate) {
      inkImageRef.SetTexturePart(this.m_lifepath, n"LifepathCorpo1");
      inkTextRef.SetText(this.m_level, "Gameplay-LifePaths-Corporate");
    };
    if Equals(metadata.lifePath, inkLifePath.Nomad) {
      inkImageRef.SetTexturePart(this.m_lifepath, n"LifepathNomad1");
      inkTextRef.SetText(this.m_level, "Gameplay-LifePaths-Nomad");
    };
    if Equals(metadata.lifePath, inkLifePath.StreetKid) {
      inkImageRef.SetTexturePart(this.m_lifepath, n"LifepathStreetKid1");
      inkTextRef.SetText(this.m_level, "Gameplay-LifePaths-Streetkid");
    };

    // We don't care about cloud...
    inkWidgetRef.SetVisible(this.m_cloudStatus, false);
}