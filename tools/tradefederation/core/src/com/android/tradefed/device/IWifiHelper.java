/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tradefed.device;

import java.util.List;
import java.util.Map;


/**
 * Helper interface for manipulating wifi services on device.
 */
interface IWifiHelper {

    /**
     * The Wifi supplicant state. Should match states defined in android.net.wifi.SupplicantState.
     */
    public enum WifiState {
        COMPLETED, SCANNING, DISCONNECTED, OTHER;
    }

    /**
     * Enables wifi state on device.
     *
     * @return <code>true</code> if wifi was enabled successfully
     * @throws DeviceNotAvailableException
     */
    boolean enableWifi() throws DeviceNotAvailableException;

    /**
     * Disables wifi state on device.
     *
     * @return <code>true</code> if wifi was disabled successfully
     * @throws DeviceNotAvailableException
     */
    boolean disableWifi() throws DeviceNotAvailableException;

    /**
     * Waits until one of the expected wifi states occurs.
     *
     * @param expectedStates one or more wifi states to expect
     * @return <code>true</code> if the one of the expected states occurred. <code>false</code> if
     *         none of the states occurred before timeout is reached
     * @throws DeviceNotAvailableException
     */
    boolean waitForWifiState(WifiState... expectedStates) throws DeviceNotAvailableException;

    /**
     * Adds the open security network identified by ssid.
     * <p/>
     * To connect to any wifi network, a network profile must be created in wpa_supplicant
     * configuration first. This will call wpa_cli to add the open security network identified by
     * ssid.
     *
     * @param ssid the ssid of network to add.
     * @return <code>true</code> if network was added successfully, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean addOpenNetwork(String ssid) throws DeviceNotAvailableException;

    /**
     * Adds the open security network identified by ssid.
     * <p/>
     * To connect to any wifi network, a network profile must be created in wpa_supplicant
     * configuration first. This will call wpa_cli to add the open security network identified by
     * ssid.
     *
     * @param ssid the ssid of network to add.
     * @param scanSsid whether to scan for hidden SSID for this network.
     * @return <code>true</code> if network was added successfully, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean addOpenNetwork(String ssid, boolean scanSsid) throws DeviceNotAvailableException;

    /**
     * Adds the WPA-PSK security network identified by ssid.
     *
     * @param ssid the ssid of network to add.
     * @param psk the WPA-PSK passphrase to use
     * @return <code>true</code> if network was added successfully, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean addWpaPskNetwork(String ssid, String psk) throws DeviceNotAvailableException;

    /**
     * Adds the WPA-PSK security network identified by ssid.
     *
     * @param ssid the ssid of network to add.
     * @param psk the WPA-PSK passphrase to use
     * @param scanSsid whether to scan for hidden SSID for this network.
     * @return <code>true</code> if network was added successfully, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean addWpaPskNetwork(String ssid, String psk, boolean scanSsid) throws DeviceNotAvailableException;

    /**
     * Wait until an ip address is assigned to wifi adapter.
     *
     * @param timeout how long to wait
     * @return <code>true</code> if an ip address is assigned before timeout, <code>false</code>
     *         otherwise
     * @throws DeviceNotAvailableException
     */
    boolean waitForIp(long timeout) throws DeviceNotAvailableException;

    /**
     * Gets the IP address associated with the wifi interface. Returns <code>null</code> if there
     * was a failure retrieving ip address.
     */
    String getIpAddress() throws DeviceNotAvailableException;

    /**
     * Gets the service set identifier of the currently connected network.
     *
     * @see
     * <a href="http://developer.android.com/reference/android/net/wifi/WifiInfo.html#getSSID()">
     * http://developer.android.com/reference/android/net/wifi/WifiInfo.html#getSSID()</a>
     * @throws DeviceNotAvailableException
     */
    String getSSID() throws DeviceNotAvailableException;

    /**
     * Gets the basic service set identifier (BSSID) of the currently access point.
     *
     * @see
     * <a href="http://developer.android.com/reference/android/net/wifi/WifiInfo.html#getSSID()">
     * http://developer.android.com/reference/android/net/wifi/WifiInfo.html#getSSID()</a>
     * @throws DeviceNotAvailableException
     */
    String getBSSID() throws DeviceNotAvailableException;

    /**
     * Removes all known networks.
     *
     * @throws DeviceNotAvailableException
     */
    boolean removeAllNetworks() throws DeviceNotAvailableException;

    /**
     * Check if wifi is currently enabled.
     */
    boolean isWifiEnabled() throws DeviceNotAvailableException;

    /**
     * Wait for {@link #isWifiEnabled()} to be true with a default timeout.
     *
     * @return <code>true</code> if wifi was enabled before timeout, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean waitForWifiEnabled() throws DeviceNotAvailableException;

    /**
     * Wait for {@link #isWifiEnabled()} to be true.
     *
     * @param timeout time in ms to wait
     * @return <code>true</code> if wifi was enabled before timeout, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean waitForWifiEnabled(long timeout) throws DeviceNotAvailableException;

    /**
     * Wait for {@link #isWifiEnabled()} to be false with a default timeout.
     *
     * @return <code>true</code> if wifi was disabled before timeout, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean waitForWifiDisabled() throws DeviceNotAvailableException;

    /**
     * Wait for {@link #isWifiEnabled()} to be false.
     *
     * @param timeout time in ms to wait
     * @return <code>true</code> if wifi was disabled before timeout, <code>false</code> otherwise.
     * @throws DeviceNotAvailableException
     */
    boolean waitForWifiDisabled(long timeout) throws DeviceNotAvailableException;

    /**
     * @return <code>true</code> if device has a valid IP address
     * @throws DeviceNotAvailableException
     */
    boolean hasValidIp() throws DeviceNotAvailableException;

    /**
     * Gets the current wifi connection information.
     * <p/>
     * This includes SSID, BSSID, IP address, link speed, and RSSI.
     * @return a map containing wifi connection information.
     * @throws DeviceNotAvailableException
     */
    Map<String, String> getWifiInfo() throws DeviceNotAvailableException;

    /**
     * Checks connectivity by sending HTTP request to the given url.
     *
     * @param urlToCheck a destination url for a HTTP request check
     * @return <code>true</code> if the device pass connectivity check.
     * @throws DeviceNotAvailableException
     */
    boolean checkConnectivity(String urlToCheck) throws DeviceNotAvailableException;

    /**
     * Connects to a wifi network and check connectivity.
     *
     * @param ssid the ssid of network to connect
     * @param psk the WPA-PSK passphrase to use. This can be null.
     * @param urlToCheck a destination url for a HTTP request check
     * @return <code>true</code> if the device pass connectivity check.
     * @throws DeviceNotAvailableException
     */
    boolean connectToNetwork(String ssid, String psk, String urlToCheck)
            throws DeviceNotAvailableException;

    /**
     * Connects to a wifi network and check connectivity.
     *
     * @param ssid the ssid of network to connect
     * @param psk the WPA-PSK passphrase to use. This can be null.
     * @param urlToCheck a destination url for a HTTP request check
     * @param scanSsid whether to scan for hidden SSID for this network
     * @return <code>true</code> if the device pass connectivity check.
     * @throws DeviceNotAvailableException
     */
    boolean connectToNetwork(String ssid, String psk, String urlToCheck, boolean scanSsid)
            throws DeviceNotAvailableException;

    /**
     * Disconnect from the current wifi network and disable wifi.
     *
     * @return <code>true</code> if the operation succeeded.
     * @throws DeviceNotAvailableException
     */
    boolean disconnectFromNetwork() throws DeviceNotAvailableException;

    /**
     * Starts network connectivity monitoring.
     *
     * @param interval interval between connectivity checks.
     * @param urlToCheck a URL to check connectivity with.
     * @return <code>true</code> if the operation succeeded.
     * @throws DeviceNotAvailableException
     */
    boolean startMonitor(long interval, String urlToCheck) throws DeviceNotAvailableException;

    /**
     * Stops network connectivity monitoring.
     * <p/>
     * This also returns the latency history since the last
     * {@link IWifiHelper#startMonitor(long, String)} call.
     *
     * @return the latency history.
     * @throws DeviceNotAvailableException
     */
    List<Long> stopMonitor() throws DeviceNotAvailableException;

    /**
     * Clean up the resources and the wifi helper packaged install. This should only be called when
     * Wifi is not needed anymore for the invocation since the device would lose the wifi connection
     * when the helper is uninstalled.
     *
     * @throws DeviceNotAvailableException
     */
    void cleanUp() throws DeviceNotAvailableException;
}
