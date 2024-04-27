module NGPlus.LoadListItem

@addMethod(LoadListItem)
public func SetCloudStatusVisibility(state: Bool) {
    inkWidgetRef.SetVisible(this.m_cloudStatus, state);
}