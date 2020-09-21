/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.ddmlib.MultiLineReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Helper class for manipulating wifi services on device.
 */
public class WifiHelper implements IWifiHelper {

    private static final String NULL = "null";
    private static final String NULL_IP_ADDR = "0.0.0.0";
    private static final String INSTRUMENTATION_CLASS = ".WifiUtil";
    public static final String INSTRUMENTATION_PKG = "com.android.tradefed.utils.wifi";
    static final String FULL_INSTRUMENTATION_NAME =
            String.format("%s/%s", INSTRUMENTATION_PKG, INSTRUMENTATION_CLASS);

    static final String CHECK_PACKAGE_CMD =
            String.format("dumpsys package %s", INSTRUMENTATION_PKG);
    static final Pattern PACKAGE_VERSION_PAT = Pattern.compile("versionCode=(\\d*)");
    static final int PACKAGE_VERSION_CODE = 21;

    private static final String WIFIUTIL_APK_NAME = "WifiUtil.apk";
    /** the default WifiUtil command timeout in minutes */
    private static final long WIFIUTIL_CMD_TIMEOUT_MINUTES = 5;

    /** the default time in ms to wait for a wifi state */
    private static final long DEFAULT_WIFI_STATE_TIMEOUT = 30*1000;

    private final ITestDevice mDevice;
    private File mWifiUtilApkFile;

    public WifiHelper(ITestDevice device) throws DeviceNotAvailableException {
        this(device, null, true);
    }

    public WifiHelper(ITestDevice device, String wifiUtilApkPath)
            throws DeviceNotAvailableException {
        this(device, wifiUtilApkPath, true);
    }

    /** Alternative constructor that can skip the setup of the wifi apk. */
    public WifiHelper(ITestDevice device, String wifiUtilApkPath, boolean doSetup)
            throws DeviceNotAvailableException {
        mDevice = device;
        if (doSetup) {
            ensureDeviceSetup(wifiUtilApkPath);
        }
    }

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    void ensureDeviceSetup(String wifiUtilApkPath) throws DeviceNotAvailableException {
        final String inst = mDevice.executeShellCommand(CHECK_PACKAGE_CMD);
        if (inst != null) {
            Matcher matcher = PACKAGE_VERSION_PAT.matcher(inst);
            if (matcher.find()) {
                try {
                    if (PACKAGE_VERSION_CODE <= Integer.parseInt(matcher.group(1))) {
                        return;
                    }
                } catch (NumberFormatException e) {
                    CLog.w("failed to parse WifiUtil version code: %s", matcher.group(1));
                }
            }
        }

        // Attempt to install utility
        try {
            setupWifiUtilApkFile(wifiUtilApkPath);

            final String error = mDevice.installPackage(mWifiUtilApkFile, true);
            if (error == null) {
                // Installed successfully; good to go.
                return;
            } else {
                throw new RuntimeException(String.format(
                        "Unable to install WifiUtil utility: %s", error));
            }
        } catch (IOException e) {
            throw new RuntimeException(String.format(
                    "Failed to unpack WifiUtil utility: %s", e.getMessage()));
        } finally {
            // Delete the tmp file only if the APK is copied from classpath
            if (wifiUtilApkPath == null) {
                FileUtil.deleteFile(mWifiUtilApkFile);
            }
        }
    }

    private void setupWifiUtilApkFile(String wifiUtilApkPath) throws IOException {
        if (wifiUtilApkPath != null) {
            mWifiUtilApkFile = new File(wifiUtilApkPath);
        } else {
            mWifiUtilApkFile = extractWifiUtilApk();
        }
    }

    /**
     * Get the {@link File} object of the APK file.
     *
     * <p>Exposed for unit testing.
     */
    @VisibleForTesting
    File getWifiUtilApkFile() {
        return mWifiUtilApkFile;
    }

    /**
     * Helper method to extract the wifi util apk from the classpath
     */
    public static File extractWifiUtilApk() throws IOException {
        File apkTempFile;
        apkTempFile = FileUtil.createTempFile(WIFIUTIL_APK_NAME, ".apk");
        InputStream apkStream = WifiHelper.class.getResourceAsStream(
            String.format("/apks/wifiutil/%s", WIFIUTIL_APK_NAME));
        FileUtil.writeToFile(apkStream, apkTempFile);
        return apkTempFile;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean enableWifi() throws DeviceNotAvailableException {
        return asBool(runWifiUtil("enableWifi"));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean disableWifi() throws DeviceNotAvailableException {
        return asBool(runWifiUtil("disableWifi"));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForWifiState(WifiState... expectedStates) throws DeviceNotAvailableException {
        return waitForWifiState(DEFAULT_WIFI_STATE_TIMEOUT, expectedStates);
    }

    /**
     * Waits the given time until one of the expected wifi states occurs.
     *
     * @param expectedStates one or more wifi states to expect
     * @param timeout max time in ms to wait
     * @return <code>true</code> if the one of the expected states occurred. <code>false</code> if
     *         none of the states occurred before timeout is reached
     * @throws DeviceNotAvailableException
     */
     boolean waitForWifiState(long timeout, WifiState... expectedStates)
            throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() < (startTime + timeout)) {
            String state = runWifiUtil("getSupplicantState");
            for (WifiState expectedState : expectedStates) {
                if (expectedState.name().equals(state)) {
                    return true;
                }
            }
            getRunUtil().sleep(getPollTime());
        }
        return false;
    }

    /**
     * Gets the time to sleep between poll attempts
     */
    long getPollTime() {
        return 1*1000;
    }

    /**
     * Remove the network identified by an integer network id.
     *
     * @param networkId the network id identifying its profile in wpa_supplicant configuration
     * @throws DeviceNotAvailableException
     */
    boolean removeNetwork(int networkId) throws DeviceNotAvailableException {
        if (!asBool(runWifiUtil("removeNetwork", "id", Integer.toString(networkId)))) {
            return false;
        }
        if (!asBool(runWifiUtil("saveConfiguration"))) {
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addOpenNetwork(String ssid) throws DeviceNotAvailableException {
        return addOpenNetwork(ssid, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addOpenNetwork(String ssid, boolean scanSsid)
            throws DeviceNotAvailableException {
        int id = asInt(runWifiUtil("addOpenNetwork", "ssid", ssid, "scanSsid",
                Boolean.toString(scanSsid)));
        if (id < 0) {
            return false;
        }
        if (!asBool(runWifiUtil("associateNetwork", "id", Integer.toString(id)))) {
            return false;
        }
        if (!asBool(runWifiUtil("saveConfiguration"))) {
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addWpaPskNetwork(String ssid, String psk) throws DeviceNotAvailableException {
        return addWpaPskNetwork(ssid, psk, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addWpaPskNetwork(String ssid, String psk, boolean scanSsid)
            throws DeviceNotAvailableException {
        int id = asInt(runWifiUtil("addWpaPskNetwork", "ssid", ssid, "psk", psk, "scan_ssid",
                Boolean.toString(scanSsid)));
        if (id < 0) {
            return false;
        }
        if (!asBool(runWifiUtil("associateNetwork", "id", Integer.toString(id)))) {
            return false;
        }
        if (!asBool(runWifiUtil("saveConfiguration"))) {
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForIp(long timeout) throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();

        while (System.currentTimeMillis() < (startTime + timeout)) {
            if (hasValidIp()) {
                return true;
            }
            getRunUtil().sleep(getPollTime());
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean hasValidIp() throws DeviceNotAvailableException {
        final String ip = getIpAddress();
        return ip != null && !ip.isEmpty() && !NULL_IP_ADDR.equals(ip);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getIpAddress() throws DeviceNotAvailableException {
        return runWifiUtil("getIpAddress");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSSID() throws DeviceNotAvailableException {
        return runWifiUtil("getSSID");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getBSSID() throws DeviceNotAvailableException {
        return runWifiUtil("getBSSID");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean removeAllNetworks() throws DeviceNotAvailableException {
        if (!asBool(runWifiUtil("removeAllNetworks"))) {
            return false;
        }
        if (!asBool(runWifiUtil("saveConfiguration"))) {
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isWifiEnabled() throws DeviceNotAvailableException {
        return asBool(runWifiUtil("isWifiEnabled"));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForWifiEnabled() throws DeviceNotAvailableException {
        return waitForWifiEnabled(DEFAULT_WIFI_STATE_TIMEOUT);
    }

    @Override
    public boolean waitForWifiEnabled(long timeout) throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();

        while (System.currentTimeMillis() < (startTime + timeout)) {
            if (isWifiEnabled()) {
                return true;
            }
            getRunUtil().sleep(getPollTime());
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForWifiDisabled() throws DeviceNotAvailableException {
        return waitForWifiDisabled(DEFAULT_WIFI_STATE_TIMEOUT);
    }

    @Override
    public boolean waitForWifiDisabled(long timeout) throws DeviceNotAvailableException {
        long startTime = System.currentTimeMillis();

        while (System.currentTimeMillis() < (startTime + timeout)) {
            if (!isWifiEnabled()) {
                return true;
            }
            getRunUtil().sleep(getPollTime());
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<String, String> getWifiInfo() throws DeviceNotAvailableException {
        Map<String, String> info = new HashMap<>();

        final String result = runWifiUtil("getWifiInfo");
        if (result != null) {
            try {
                final JSONObject json = new JSONObject(result);
                final Iterator<?> keys = json.keys();
                while (keys.hasNext()) {
                    final String key = (String)keys.next();
                    info.put(key, json.getString(key));
                }
            } catch(final JSONException e) {
                CLog.w("Failed to parse wifi info: %s", e.getMessage());
            }
        }

        return info;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean checkConnectivity(String urlToCheck) throws DeviceNotAvailableException {
        return asBool(runWifiUtil("checkConnectivity", "urlToCheck", urlToCheck));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean connectToNetwork(String ssid, String psk, String urlToCheck)
            throws DeviceNotAvailableException {
        return connectToNetwork(ssid, psk, urlToCheck, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean connectToNetwork(String ssid, String psk, String urlToCheck,
            boolean scanSsid) throws DeviceNotAvailableException {
        return asBool(runWifiUtil("connectToNetwork", "ssid", ssid, "psk", psk, "urlToCheck",
                urlToCheck, "scan_ssid", Boolean.toString(scanSsid)));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean disconnectFromNetwork() throws DeviceNotAvailableException {
        return asBool(runWifiUtil("disconnectFromNetwork"));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean startMonitor(long interval, String urlToCheck) throws DeviceNotAvailableException {
        return asBool(runWifiUtil("startMonitor", "interval", Long.toString(interval), "urlToCheck",
                urlToCheck));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<Long> stopMonitor() throws DeviceNotAvailableException {
        final String output = runWifiUtil("stopMonitor");
        if (output == null || output.isEmpty() || NULL.equals(output)) {
            return new ArrayList<Long>(0);
        }

        String[] tokens = output.split(",");
        List<Long> values = new ArrayList<Long>(tokens.length);
        for (final String token : tokens) {
            values.add(Long.parseLong(token));
        }
        return values;
    }

    /**
     * Run a WifiUtil command and return the result
     *
     * @param method the WifiUtil method to call
     * @param args a flat list of [arg-name, value] pairs to pass
     * @return The value of the result field in the output, or <code>null</code> if result could
     * not be parsed
     */
    private String runWifiUtil(String method, String... args) throws DeviceNotAvailableException {
        final String cmd = buildWifiUtilCmd(method, args);

        WifiUtilOutput parser = new WifiUtilOutput();
        mDevice.executeShellCommand(cmd, parser, WIFIUTIL_CMD_TIMEOUT_MINUTES, TimeUnit.MINUTES, 0);
        if (parser.getError() != null) {
            CLog.e(parser.getError());
        }
        return parser.getResult();
    }

    /**
     * Build and return a WifiUtil command for the specified method and args
     *
     * @param method the WifiUtil method to call
     * @param args a flat list of [arg-name, value] pairs to pass
     * @return the command to be executed on the device shell
     */
    static String buildWifiUtilCmd(String method, String... args) {
        Map<String, String> argMap = new HashMap<String, String>();
        argMap.put("method", method);
        if ((args.length & 0x1) == 0x1) {
            throw new IllegalArgumentException(
                    "args should have even length, consisting of key and value pairs");
        }
        for (int i = 0; i < args.length; i += 2) {
            // Skip null parameters
            if (args[i+1] == null) {
                continue;
            }
            argMap.put(args[i], args[i+1]);
        }
        return buildWifiUtilCmdFromMap(argMap);
    }

    /**
     * Build and return a WifiUtil command for the specified args
     *
     * @param args A Map of (arg-name, value) pairs to pass as "-e" arguments to the `am` command
     * @return the commadn to be executed on the device shell
     */
    static String buildWifiUtilCmdFromMap(Map<String, String> args) {
        StringBuilder sb = new StringBuilder("am instrument");

        for (Map.Entry<String, String> arg : args.entrySet()) {
            sb.append(" -e ");
            sb.append(arg.getKey());
            sb.append(" ");
            sb.append(quote(arg.getValue()));
        }

        sb.append(" -w ");
        sb.append(INSTRUMENTATION_PKG);
        sb.append("/");
        sb.append(INSTRUMENTATION_CLASS);

        return sb.toString();
    }

    /**
     * Helper function to convert a String to an Integer
     */
    private static int asInt(String str) {
        if (str == null) {
            return -1;
        }
        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException e) {
            return -1;
        }
    }

    /**
     * Helper function to convert a String to a boolean.  Maps "true" to true, and everything else
     * to false.
     */
    private static boolean asBool(String str) {
        return "true".equals(str);
    }

    /**
     * Helper function to wrap the specified String in double-quotes to prevent shell interpretation
     */
    private static String quote(String str) {
        return String.format("\"%s\"", str);
    }

    /**
     * Processes the output of a WifiUtil invocation
     */
    private static class WifiUtilOutput extends MultiLineReceiver {
        private static final Pattern RESULT_PAT =
                Pattern.compile("INSTRUMENTATION_RESULT: result=(.*)");
        private static final Pattern ERROR_PAT =
                Pattern.compile("INSTRUMENTATION_RESULT: error=(.*)");

        private String mResult = null;
        private String mError = null;

        /**
         * {@inheritDoc}
         */
        @Override
        public void processNewLines(String[] lines) {
            for (String line : lines) {
                Matcher resultMatcher = RESULT_PAT.matcher(line);
                if (resultMatcher.matches()) {
                    mResult = resultMatcher.group(1);
                    continue;
                }

                Matcher errorMatcher = ERROR_PAT.matcher(line);
                if (errorMatcher.matches()) {
                    mError = errorMatcher.group(1);
                }
            }
        }

        /**
         * Return the result flag parsed from instrumentation output. <code>null</code> is returned
         * if result output was not present.
         */
        String getResult() {
            return mResult;
        }

        String getError() {
            return mError;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isCancelled() {
            return false;
        }
    }

    /** {@inheritDoc} */
    @Override
    public void cleanUp() throws DeviceNotAvailableException {
        String output = mDevice.uninstallPackage(INSTRUMENTATION_PKG);
        if (output != null) {
            CLog.w("Error '%s' occurred when uninstalling %s", output, INSTRUMENTATION_PKG);
        } else {
            CLog.d("Successfully clean up WifiHelper.");
        }
    }
}

