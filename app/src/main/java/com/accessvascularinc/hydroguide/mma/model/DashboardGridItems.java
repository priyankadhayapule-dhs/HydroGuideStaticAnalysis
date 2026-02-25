package com.accessvascularinc.hydroguide.mma.model;

import com.accessvascularinc.hydroguide.mma.utils.ScreenRoute;

public class DashboardGridItems
{
  private String caption;
  private int icon;
  private ScreenRoute screenRoute;
  private boolean isEnabled;

  public DashboardGridItems(String caption, int icon, ScreenRoute screenRoute, boolean isEnabled) {
    this.caption = caption;
    this.icon = icon;
    this.screenRoute = screenRoute;
    this.isEnabled = isEnabled;
  }

  public String getCaption() {
    return caption;
  }

  public void setCaption(final String caption) {
    this.caption = caption;
  }

  public int getIcon() {
    return icon;
  }

  public void setIcon(final int icon) {
    this.icon = icon;
  }

  public ScreenRoute getDashboardMenu() {
    return screenRoute;
  }

  public void setDashboardMenu(final ScreenRoute screenRoute) {
    this.screenRoute = screenRoute;
  }

  public boolean isEnabled() {
    return isEnabled;
  }

  public void setEnabled(final boolean enabled) {
    isEnabled = enabled;
  }
}
