package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
import static android.os.Build.VERSION_CODES.LOLLIPOP;

import android.net.DhcpInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Pair;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.robolectric.Shadows;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

/**
 * Shadow for {@link android.net.wifi.WifiManager}.
 */
@Implements(WifiManager.class)
public class ShadowWifiManager {
  private static float sSignalLevelInPercent = 1f;
  private boolean accessWifiStatePermission = true;
  private boolean wifiEnabled = true;
  private boolean wasSaved = false;
  private WifiInfo wifiInfo;
  private List<ScanResult> scanResults;
  private final Map<Integer, WifiConfiguration> networkIdToConfiguredNetworks = new LinkedHashMap<>();
  private Pair<Integer, Boolean> lastEnabledNetwork;
  private DhcpInfo dhcpInfo;
  private boolean isScanAlwaysAvailable = true;

  @Implementation
  public boolean setWifiEnabled(boolean wifiEnabled) {
    checkAccessWifiStatePermission();
    this.wifiEnabled = wifiEnabled;
    return true;
  }

  @Implementation
  public boolean isWifiEnabled() {
    checkAccessWifiStatePermission();
    return wifiEnabled;
  }

  @Implementation
  public int getWifiState() {
    if (isWifiEnabled()) {
      return WifiManager.WIFI_STATE_ENABLED;
    } else {
      return WifiManager.WIFI_STATE_DISABLED;
    }
  }

  @Implementation
  public WifiInfo getConnectionInfo() {
    checkAccessWifiStatePermission();
    if (wifiInfo == null) {
      wifiInfo = ReflectionHelpers.callConstructor(WifiInfo.class);
    }
    return wifiInfo;
  }

  /**
   * Sets the connection info as the provided {@link WifiInfo}.
   */
  public void setConnectionInfo(WifiInfo wifiInfo) {
    this.wifiInfo = wifiInfo;
  }

  @Implementation
  public List<ScanResult> getScanResults() {
    return scanResults;
  }

  @Implementation
  public List<WifiConfiguration> getConfiguredNetworks() {
    final ArrayList<WifiConfiguration> wifiConfigurations = new ArrayList<>();
    for (WifiConfiguration wifiConfiguration : networkIdToConfiguredNetworks.values()) {
      wifiConfigurations.add(wifiConfiguration);
    }
    return wifiConfigurations;
  }

  @Implementation(minSdk = LOLLIPOP)
  public List<WifiConfiguration> getPrivilegedConfiguredNetworks() {
    return getConfiguredNetworks();
  }

  @Implementation
  public int addNetwork(WifiConfiguration config) {
    int networkId = networkIdToConfiguredNetworks.size();
    config.networkId = -1;
    networkIdToConfiguredNetworks.put(networkId, makeCopy(config, networkId));
    return networkId;
  }

  @Implementation
  public boolean removeNetwork(int netId) {
    networkIdToConfiguredNetworks.remove(netId);
    return true;
  }

  @Implementation
  public int updateNetwork(WifiConfiguration config) {
    if (config == null || config.networkId < 0) {
      return -1;
    }
    networkIdToConfiguredNetworks.put(config.networkId, makeCopy(config, config.networkId));
    return config.networkId;
  }

  @Implementation
  public boolean saveConfiguration() {
    wasSaved = true;
    return true;
  }

  @Implementation
  public boolean enableNetwork(int netId, boolean disableOthers) {
    lastEnabledNetwork = new Pair<>(netId, disableOthers);
    return true;
  }

  @Implementation
  public WifiManager.WifiLock createWifiLock(int lockType, java.lang.String tag) {
    return ReflectionHelpers.callConstructor(WifiManager.WifiLock.class);
  }

  @Implementation
  public WifiManager.WifiLock createWifiLock(java.lang.String tag) {
    return createWifiLock(WifiManager.WIFI_MODE_FULL, tag);
  }

  @Implementation
  public static int calculateSignalLevel(int rssi, int numLevels) {
    return (int) (sSignalLevelInPercent * (numLevels - 1));
  }

  @Implementation
  public boolean startScan() {
    return true;
  }

  @Implementation
  public DhcpInfo getDhcpInfo() {
    return dhcpInfo;
  }

  @Implementation(minSdk = JELLY_BEAN_MR2)
  public boolean isScanAlwaysAvailable() {
    return isScanAlwaysAvailable;
  }

  public static void setSignalLevelInPercent(float level) {
    if (level < 0 || level > 1) {
      throw new IllegalArgumentException("level needs to be between 0 and 1");
    }
    sSignalLevelInPercent = level;
  }

  public void setAccessWifiStatePermission(boolean accessWifiStatePermission) {
    this.accessWifiStatePermission = accessWifiStatePermission;
  }

  public void setScanResults(List<ScanResult> scanResults) {
    this.scanResults = scanResults;
  }

  public void setDhcpInfo(DhcpInfo dhcpInfo) {
    this.dhcpInfo = dhcpInfo;
  }

  public Pair<Integer, Boolean> getLastEnabledNetwork() {
    return lastEnabledNetwork;
  }

  public boolean wasConfigurationSaved() {
    return wasSaved;
  }

  public void setIsScanAlwaysAvailable(boolean isScanAlwaysAvailable) {
    this.isScanAlwaysAvailable = isScanAlwaysAvailable;
  }

  private void checkAccessWifiStatePermission() {
    if (!accessWifiStatePermission) {
      throw new SecurityException();
    }
  }

  private WifiConfiguration makeCopy(WifiConfiguration config, int networkId) {
    WifiConfiguration copy = Shadows.shadowOf(config).copy();
    copy.networkId = networkId;
    return copy;
  }

  @Implements(WifiManager.WifiLock.class)
  public static class ShadowWifiLock {
    private int refCount;
    private boolean refCounted = true;
    private boolean locked;
    public static final int MAX_ACTIVE_LOCKS = 50;

    @Implementation
    public synchronized void acquire() {
      if (refCounted) {
        if (++refCount >= MAX_ACTIVE_LOCKS) throw new UnsupportedOperationException("Exceeded maximum number of wifi locks");
      } else {
        locked = true;
      }
    }

    @Implementation
    public synchronized void release() {
      if (refCounted) {
        if (--refCount < 0) throw new RuntimeException("WifiLock under-locked");
      } else {
        locked = false;
      }
    }

    @Implementation
    public synchronized boolean isHeld() {
      return refCounted ? refCount > 0 : locked;
    }

    @Implementation
    public void setReferenceCounted(boolean refCounted) {
      this.refCounted = refCounted;
    }
  }
}
