package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.LOLLIPOP;
import static android.os.Build.VERSION_CODES.M;
import static org.robolectric.RuntimeEnvironment.getApiLevel;

import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.robolectric.Shadows;
import org.robolectric.annotation.HiddenApi;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@Implements(ConnectivityManager.class)
public class ShadowConnectivityManager {

  // Package-private for tests.
  static final int NET_ID_WIFI = ConnectivityManager.TYPE_WIFI;
  static final int NET_ID_MOBILE = ConnectivityManager.TYPE_MOBILE;

  private NetworkInfo activeNetworkInfo;
  private boolean backgroundDataSetting;
  private int networkPreference = ConnectivityManager.DEFAULT_NETWORK_PREFERENCE;
  private final Map<Integer, NetworkInfo> networkTypeToNetworkInfo = new HashMap<>();

  private HashSet<ConnectivityManager.NetworkCallback> networkCallbacks = new HashSet<>();
  private final Map<Integer, Network> netIdToNetwork = new HashMap<>();
  private final Map<Integer, NetworkInfo> netIdToNetworkInfo = new HashMap<>();
  private Network processBoundNetwork;
  private boolean defaultNetworkActive;
  private HashSet<ConnectivityManager.OnNetworkActiveListener> onNetworkActiveListeners =
      new HashSet<>();

  public ShadowConnectivityManager() {
    NetworkInfo wifi = ShadowNetworkInfo.newInstance(NetworkInfo.DetailedState.DISCONNECTED,
        ConnectivityManager.TYPE_WIFI, 0, true, false);
    networkTypeToNetworkInfo.put(ConnectivityManager.TYPE_WIFI, wifi);

    NetworkInfo mobile = ShadowNetworkInfo.newInstance(NetworkInfo.DetailedState.CONNECTED,
        ConnectivityManager.TYPE_MOBILE, ConnectivityManager.TYPE_MOBILE_MMS, true, true);
    networkTypeToNetworkInfo.put(ConnectivityManager.TYPE_MOBILE, mobile);

    this.activeNetworkInfo = mobile;

    if (getApiLevel() >= LOLLIPOP) {
      netIdToNetwork.put(NET_ID_WIFI, ShadowNetwork.newInstance(NET_ID_WIFI));
      netIdToNetwork.put(NET_ID_MOBILE, ShadowNetwork.newInstance(NET_ID_MOBILE));
      netIdToNetworkInfo.put(NET_ID_WIFI, wifi);
      netIdToNetworkInfo.put(NET_ID_MOBILE, mobile);
    }
    defaultNetworkActive = true;
  }

  public Set<ConnectivityManager.NetworkCallback> getNetworkCallbacks() {
    return networkCallbacks;
  }

  @Implementation(minSdk = LOLLIPOP)
  public void registerNetworkCallback(NetworkRequest request, ConnectivityManager.NetworkCallback networkCallback) {
    networkCallbacks.add(networkCallback);
  }

  @Implementation(minSdk = LOLLIPOP)
  public void unregisterNetworkCallback (ConnectivityManager.NetworkCallback networkCallback) {
    if (networkCallback == null) {
      throw new IllegalArgumentException("Invalid NetworkCallback");
    }
    if (networkCallbacks.contains(networkCallback)) {
      networkCallbacks.remove(networkCallback);
    }
  }

  @Implementation
  public NetworkInfo getActiveNetworkInfo() {
    return activeNetworkInfo;
  }

  @Implementation(minSdk = M)
  public Network getActiveNetwork() {
    if (defaultNetworkActive) {
      return netIdToNetwork.get(getActiveNetworkInfo().getType());
    }
    return null;
  }

  @Implementation
  public NetworkInfo[] getAllNetworkInfo() {
    if (defaultNetworkActive) {
      return networkTypeToNetworkInfo
          .values()
          .toArray(new NetworkInfo[networkTypeToNetworkInfo.size()]);
    }
    return null;
  }

  @Implementation
  public NetworkInfo getNetworkInfo(int networkType) {
    return networkTypeToNetworkInfo.get(networkType);
  }

  @Implementation(minSdk = LOLLIPOP)
  public NetworkInfo getNetworkInfo(Network network) {
    ShadowNetwork shadowNetwork = Shadows.shadowOf(network);
    return netIdToNetworkInfo.get(shadowNetwork.getNetId());
  }

  @Implementation(minSdk = LOLLIPOP)
  public Network[] getAllNetworks() {
    return netIdToNetwork.values().toArray(new Network[netIdToNetwork.size()]);
  }

  @Implementation
  public boolean getBackgroundDataSetting() {
    return backgroundDataSetting;
  }

  @Implementation
  public void setNetworkPreference(int preference) {
    networkPreference = preference;
  }

  @Implementation
  public int getNetworkPreference() {
    return networkPreference;
  }

  /**
   * Count {@link ConnectivityManager#TYPE_MOBILE} networks as metered.
   * Other types will be considered unmetered.
   *
   * @return True if the active network is metered.
   */
  @Implementation
  public boolean isActiveNetworkMetered() {
    if (defaultNetworkActive && activeNetworkInfo != null) {
      return activeNetworkInfo.getType() == ConnectivityManager.TYPE_MOBILE;
    } else {
      return false;
    }
  }

  @Implementation(minSdk = M)
  public boolean bindProcessToNetwork(Network network) {
    processBoundNetwork = network;
    return true;
  }

  @Implementation(minSdk = M)
  public Network getBoundNetworkForProcess() {
    return processBoundNetwork;
  }

  public void setNetworkInfo(int networkType, NetworkInfo networkInfo) {
    networkTypeToNetworkInfo.put(networkType, networkInfo);
  }

  @HiddenApi @Implementation
  public void setBackgroundDataSetting(boolean b) {
    backgroundDataSetting = b;
  }

  public void setActiveNetworkInfo(NetworkInfo info) {
    if (getApiLevel() >= LOLLIPOP) {
      activeNetworkInfo = info;
      if (info != null) {
        networkTypeToNetworkInfo.put(info.getType(), info);
        netIdToNetwork.put(info.getType(), ShadowNetwork.newInstance(info.getType()));
      } else {
        networkTypeToNetworkInfo.clear();
        netIdToNetwork.clear();
      }
    } else {
      activeNetworkInfo = info;
      if (info != null) {
        networkTypeToNetworkInfo.put(info.getType(), info);
      } else {
        networkTypeToNetworkInfo.clear();
      }
    }
  }

  /**
   * Adds new {@code network} to the list of all {@link android.net.Network}s.
   *
   * @param network The network.
   * @param networkInfo The network info paired with the {@link android.net.Network}.
   */
  public void addNetwork(Network network, NetworkInfo networkInfo) {
    ShadowNetwork shadowNetwork = Shadows.shadowOf(network);
    int netId = shadowNetwork.getNetId();
    netIdToNetwork.put(netId, network);
    netIdToNetworkInfo.put(netId, networkInfo);
  }

  /**
   * Removes the {@code network} from the list of all {@link android.net.Network}s.
   * @param network The network.
   */
  public void removeNetwork(Network network) {
    ShadowNetwork shadowNetwork = Shadows.shadowOf(network);
    int netId = shadowNetwork.getNetId();
    netIdToNetwork.remove(netId);
    netIdToNetworkInfo.remove(netId);
  }

  /**
   * Clears the list of all {@link android.net.Network}s.
   */
  public void clearAllNetworks() {
    netIdToNetwork.clear();
    netIdToNetworkInfo.clear();
  }

  /**
   * Sets the active state of the default network.
   *
   * <p>By default this is true and controls the result of {@link
   * ConnectivityManager#isActiveNetworkMetered()}, {@link
   * ConnectivityManager#isDefaultNetworkActivite()}, {@link ConnectivityManager#getActiveNetwork()}
   * and {@link ConnectivityManager#getAllNetworkInfo()}.
   *
   * <p>Calling this method with {@code true} after any listeners have been registered with {@link
   * ConnectivityManager#addDefaultNetworkActiveListener()} will result in those listeners being
   * fired.
   *
   * @param isActive The active state of the default network.
   */
  public void setDefaultNetworkActive(boolean isActive) {
    defaultNetworkActive = isActive;
    if (defaultNetworkActive) {
      for (ConnectivityManager.OnNetworkActiveListener l : onNetworkActiveListeners) {
        if (l != null) {
          l.onNetworkActive();
        }
      }
    }
  }

  @Implementation(minSdk = LOLLIPOP)
  public boolean isDefaultNetworkActive() {
    return defaultNetworkActive;
  }

  @Implementation(minSdk = LOLLIPOP)
  public void addDefaultNetworkActiveListener(final ConnectivityManager.OnNetworkActiveListener l) {
    onNetworkActiveListeners.add(l);
  }

  @Implementation(minSdk = LOLLIPOP)
  public void removeDefaultNetworkActiveListener(ConnectivityManager.OnNetworkActiveListener l) {
    if (l == null) {
      throw new IllegalArgumentException("Invalid OnNetworkActiveListener");
    }
    if (onNetworkActiveListeners.contains(l)) {
      onNetworkActiveListeners.remove(l);
    }
  }
}
