package org.robolectric.shadows;

import android.net.wifi.WifiConfiguration;
import android.os.Build;
import java.util.BitSet;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;

@Implements(WifiConfiguration.class)
public class ShadowWifiConfiguration {
  @RealObject private WifiConfiguration realObject;

  @Implementation
  public void __constructor__() {
    realObject.networkId = -1;
    realObject.SSID = null;
    realObject.BSSID = null;
    realObject.priority = 0;
    realObject.hiddenSSID = false;
    realObject.allowedKeyManagement = new BitSet();
    realObject.allowedProtocols = new BitSet();
    realObject.allowedAuthAlgorithms = new BitSet();
    realObject.allowedPairwiseCiphers = new BitSet();
    realObject.allowedGroupCiphers = new BitSet();
    realObject.wepKeys = new String[4];
    for (int i = 0; i < realObject.wepKeys.length; i++)
      realObject.wepKeys[i] = null;
//        for (EnterpriseField field : realObject.enterpriseFields) {
//            field.setValue(null);
//        }
  }

  public WifiConfiguration copy() {
    WifiConfiguration config = new WifiConfiguration();
    config.networkId = realObject.networkId;
    config.SSID = realObject.SSID;
    config.BSSID = realObject.BSSID;
    config.preSharedKey = realObject.preSharedKey;
    config.wepTxKeyIndex = realObject.wepTxKeyIndex;
    config.status = realObject.status;
    config.priority = realObject.priority;
    config.hiddenSSID = realObject.hiddenSSID;
    config.allowedKeyManagement = (BitSet) realObject.allowedKeyManagement.clone();
    config.allowedProtocols = (BitSet) realObject.allowedProtocols.clone();
    config.allowedAuthAlgorithms = (BitSet) realObject.allowedAuthAlgorithms.clone();
    config.allowedPairwiseCiphers = (BitSet) realObject.allowedPairwiseCiphers.clone();
    config.allowedGroupCiphers = (BitSet) realObject.allowedGroupCiphers.clone();
    config.wepKeys = new String[4];
    System.arraycopy(realObject.wepKeys, 0, config.wepKeys, 0, config.wepKeys.length);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      config.creatorName = realObject.creatorName;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      config.creatorUid = realObject.creatorUid;
    }
    return config;
  }

  // WifiConfiguration's toString() method crashes.
  @Override @Implementation
  public String toString() {
    return String.format("WifiConfiguration{ssid=%s}", realObject.SSID);
  }
}
