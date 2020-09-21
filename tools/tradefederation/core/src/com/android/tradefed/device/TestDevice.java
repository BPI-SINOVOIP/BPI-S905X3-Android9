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

import com.android.ddmlib.AdbCommandRejectedException;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.InstallException;
import com.android.ddmlib.InstallReceiver;
import com.android.ddmlib.RawImage;
import com.android.ddmlib.ShellCommandUnresponsiveException;
import com.android.ddmlib.SyncException;
import com.android.ddmlib.TimeoutException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.util.KeyguardControllerState;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Strings;

import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.imageio.ImageIO;

/**
 * Implementation of a {@link ITestDevice} for a full stack android device
 */
public class TestDevice extends NativeDevice {

    /** number of attempts made to clear dialogs */
    private static final int NUM_CLEAR_ATTEMPTS = 5;
    /** the command used to dismiss a error dialog. Currently sends a DPAD_CENTER key event */
    static final String DISMISS_DIALOG_CMD = "input keyevent 23";
    /** Commands that can be used to dismiss the keyguard. */
    static final String DISMISS_KEYGUARD_CMD = "input keyevent 82";

    /**
     * Alternative command to dismiss the keyguard by requesting the Window Manager service to do
     * it. Api 23 and after.
     */
    static final String DISMISS_KEYGUARD_WM_CMD = "wm dismiss-keyguard";

    /** Timeout to wait for input dispatch to become ready **/
    private static final long INPUT_DISPATCH_READY_TIMEOUT = 5 * 1000;
    /** command to test input dispatch readiness **/
    private static final String TEST_INPUT_CMD = "dumpsys input";

    private static final long AM_COMMAND_TIMEOUT = 10 * 1000;
    private static final long CHECK_NEW_USER = 1000;

    static final String LIST_PACKAGES_CMD = "pm list packages -f";
    private static final Pattern PACKAGE_REGEX = Pattern.compile("package:(.*)=(.*)");

    private static final int FLAG_PRIMARY = 1; // From the UserInfo class

    private static final String[] SETTINGS_NAMESPACE = {"system", "secure", "global"};

    /** user pattern in the output of "pm list users" = TEXT{<id>:<name>:<flags>} TEXT * */
    private static final String USER_PATTERN = "(.*?\\{)(\\d+)(:)(.*)(:)(\\d+)(\\}.*)";

    private static final int API_LEVEL_GET_CURRENT_USER = 24;
    /** Timeout to wait for a screenshot before giving up to avoid hanging forever */
    private static final long MAX_SCREENSHOT_TIMEOUT = 5 * 60 * 1000; // 5 min

    /** adb shell am dumpheap <service pid> <dump file path> */
    private static final String DUMPHEAP_CMD = "am dumpheap %s %s";
    /** Time given to a file to be dumped on device side */
    private static final long DUMPHEAP_TIME = 5000l;

    /** Timeout in minutes for the package installation */
    static final long INSTALL_TIMEOUT_MINUTES = 4;
    /** Max timeout to output for package installation */
    static final long INSTALL_TIMEOUT_TO_OUTPUT_MINUTES = 3;

    private boolean mWasWifiHelperInstalled = false;

    /**
     * @param device
     * @param stateMonitor
     * @param allocationMonitor
     */
    public TestDevice(IDevice device, IDeviceStateMonitor stateMonitor,
            IDeviceMonitor allocationMonitor) {
        super(device, stateMonitor, allocationMonitor);
    }

    /**
     * Core implementation of package installation, with retries around
     * {@link IDevice#installPackage(String, boolean, String...)}
     * @param packageFile
     * @param reinstall
     * @param extraArgs
     * @return the response from the installation
     * @throws DeviceNotAvailableException
     */
    private String internalInstallPackage(
            final File packageFile, final boolean reinstall, final List<String> extraArgs)
                    throws DeviceNotAvailableException {
        // use array to store response, so it can be returned to caller
        final String[] response = new String[1];
        DeviceAction installAction =
                new DeviceAction() {
                    @Override
                    public boolean run() throws InstallException {
                        try {
                            InstallReceiver receiver = createInstallReceiver();
                            getIDevice()
                                    .installPackage(
                                            packageFile.getAbsolutePath(),
                                            reinstall,
                                            receiver,
                                            INSTALL_TIMEOUT_MINUTES,
                                            INSTALL_TIMEOUT_TO_OUTPUT_MINUTES,
                                            TimeUnit.MINUTES,
                                            extraArgs.toArray(new String[] {}));
                            if (receiver.isSuccessfullyCompleted()) {
                                response[0] = null;
                            } else if (receiver.getErrorMessage() == null) {
                                response[0] =
                                        String.format(
                                                "Installation of %s timed out",
                                                packageFile.getAbsolutePath());
                            } else {
                                response[0] = receiver.getErrorMessage();
                            }
                        } catch (InstallException e) {
                            response[0] = e.getMessage();
                        }
                        return response[0] == null;
                    }
                };
        performDeviceAction(String.format("install %s", packageFile.getAbsolutePath()),
                installAction, MAX_RETRY_ATTEMPTS);
        return response[0];
    }

    /**
     * Creates and return an {@link InstallReceiver} for {@link #internalInstallPackage(File,
     * boolean, List)} and {@link #installPackage(File, File, boolean, String...)} testing.
     */
    @VisibleForTesting
    InstallReceiver createInstallReceiver() {
        return new InstallReceiver();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String installPackage(final File packageFile, final boolean reinstall,
            final String... extraArgs) throws DeviceNotAvailableException {
        boolean runtimePermissionSupported = isRuntimePermissionSupported();
        List<String> args = new ArrayList<>(Arrays.asList(extraArgs));
        // grant all permissions by default if feature is supported
        if (runtimePermissionSupported) {
            args.add("-g");
        }
        return internalInstallPackage(packageFile, reinstall, args);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String installPackage(File packageFile, boolean reinstall, boolean grantPermissions,
            String... extraArgs) throws DeviceNotAvailableException {
        ensureRuntimePermissionSupported();
        List<String> args = new ArrayList<>(Arrays.asList(extraArgs));
        if (grantPermissions) {
            args.add("-g");
        }
        return internalInstallPackage(packageFile, reinstall, args);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String installPackageForUser(File packageFile, boolean reinstall, int userId,
            String... extraArgs) throws DeviceNotAvailableException {
        boolean runtimePermissionSupported = isRuntimePermissionSupported();
        List<String> args = new ArrayList<>(Arrays.asList(extraArgs));
        // grant all permissions by default if feature is supported
        if (runtimePermissionSupported) {
            args.add("-g");
        }
        args.add("--user");
        args.add(Integer.toString(userId));
        return internalInstallPackage(packageFile, reinstall, args);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String installPackageForUser(File packageFile, boolean reinstall,
            boolean grantPermissions, int userId, String... extraArgs)
                    throws DeviceNotAvailableException {
        ensureRuntimePermissionSupported();
        List<String> args = new ArrayList<>(Arrays.asList(extraArgs));
        if (grantPermissions) {
            args.add("-g");
        }
        args.add("--user");
        args.add(Integer.toString(userId));
        return internalInstallPackage(packageFile, reinstall, args);
    }

    public String installPackage(final File packageFile, final File certFile,
            final boolean reinstall, final String... extraArgs) throws DeviceNotAvailableException {
        // use array to store response, so it can be returned to caller
        final String[] response = new String[1];
        DeviceAction installAction =
                new DeviceAction() {
                    @Override
                    public boolean run()
                            throws InstallException, SyncException, IOException, TimeoutException,
                                    AdbCommandRejectedException {
                        // TODO: create a getIDevice().installPackage(File, File...) method when the
                        // dist cert functionality is ready to be open sourced
                        String remotePackagePath =
                                getIDevice().syncPackageToDevice(packageFile.getAbsolutePath());
                        String remoteCertPath =
                                getIDevice().syncPackageToDevice(certFile.getAbsolutePath());
                        // trick installRemotePackage into issuing a 'pm install <apk> <cert>'
                        // command, by adding apk path to extraArgs, and using cert as the
                        // 'apk file'.
                        String[] newExtraArgs = new String[extraArgs.length + 1];
                        System.arraycopy(extraArgs, 0, newExtraArgs, 0, extraArgs.length);
                        newExtraArgs[newExtraArgs.length - 1] =
                                String.format("\"%s\"", remotePackagePath);
                        try {
                            InstallReceiver receiver = createInstallReceiver();
                            getIDevice()
                                    .installRemotePackage(
                                            remoteCertPath,
                                            reinstall,
                                            receiver,
                                            INSTALL_TIMEOUT_MINUTES,
                                            INSTALL_TIMEOUT_TO_OUTPUT_MINUTES,
                                            TimeUnit.MINUTES,
                                            newExtraArgs);
                            if (receiver.isSuccessfullyCompleted()) {
                                response[0] = null;
                            } else if (receiver.getErrorMessage() == null) {
                                response[0] =
                                        String.format(
                                                "Installation of %s timed out.",
                                                packageFile.getAbsolutePath());
                            } else {
                                response[0] = receiver.getErrorMessage();
                            }
                        } catch (InstallException e) {
                            response[0] = e.getMessage();
                        } finally {
                            getIDevice().removeRemotePackage(remotePackagePath);
                            getIDevice().removeRemotePackage(remoteCertPath);
                        }
                        return true;
                    }
                };
        performDeviceAction(String.format("install %s", packageFile.getAbsolutePath()),
                installAction, MAX_RETRY_ATTEMPTS);
        return response[0];
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String uninstallPackage(final String packageName) throws DeviceNotAvailableException {
        // use array to store response, so it can be returned to caller
        final String[] response = new String[1];
        DeviceAction uninstallAction = new DeviceAction() {
            @Override
            public boolean run() throws InstallException {
                CLog.d("Uninstalling %s", packageName);
                String result = getIDevice().uninstallPackage(packageName);
                response[0] = result;
                return result == null;
            }
        };
        performDeviceAction(String.format("uninstall %s", packageName), uninstallAction,
                MAX_RETRY_ATTEMPTS);
        return response[0];
    }

    /** {@inheritDoc} */
    @Override
    public InputStreamSource getScreenshot() throws DeviceNotAvailableException {
        return getScreenshot("PNG");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public InputStreamSource getScreenshot(String format) throws DeviceNotAvailableException {
        return getScreenshot(format, true);
    }

    /** {@inheritDoc} */
    @Override
    public InputStreamSource getScreenshot(String format, boolean rescale)
            throws DeviceNotAvailableException {
        if (!format.equalsIgnoreCase("PNG") && !format.equalsIgnoreCase("JPEG")){
            CLog.e("Screenshot: Format %s is not supported, defaulting to PNG.", format);
            format = "PNG";
        }
        ScreenshotAction action = new ScreenshotAction();
        if (performDeviceAction("screenshot", action, MAX_RETRY_ATTEMPTS)) {
            byte[] imageData =
                    compressRawImage(action.mRawScreenshot, format.toUpperCase(), rescale);
            if (imageData != null) {
                return new ByteArrayInputStreamSource(imageData);
            }
        }
        // Return an error in the buffer
        return new ByteArrayInputStreamSource(
                "Error: device reported null for screenshot.".getBytes());
    }

    private class ScreenshotAction implements DeviceAction {

        RawImage mRawScreenshot;

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean run() throws IOException, TimeoutException, AdbCommandRejectedException,
                ShellCommandUnresponsiveException, InstallException, SyncException {
            mRawScreenshot =
                    getIDevice().getScreenshot(MAX_SCREENSHOT_TIMEOUT, TimeUnit.MILLISECONDS);
            return mRawScreenshot != null;
        }
    }

    /**
     * Helper to compress a rawImage obtained from the screen.
     *
     * @param rawImage {@link RawImage} to compress.
     * @param format resulting format of compressed image. PNG and JPEG are supported.
     * @param rescale if rescaling should be done to further reduce size of compressed image.
     * @return compressed image.
     */
    @VisibleForTesting
    byte[] compressRawImage(RawImage rawImage, String format, boolean rescale) {
        BufferedImage image = rawImageToBufferedImage(rawImage, format);

        // Rescale to reduce size if needed
        // Screenshot default format is 1080 x 1920, 8-bit/color RGBA
        // By cutting in half we can easily keep good quality and smaller size
        if (rescale) {
            image = rescaleImage(image);
        }

        return getImageData(image, format);
    }

    /**
     * Converts {@link RawImage} to {@link BufferedImage} in specified format.
     *
     * @param rawImage {@link RawImage} to convert.
     * @param format resulting format of image. PNG and JPEG are supported.
     * @return converted image.
     */
    @VisibleForTesting
    BufferedImage rawImageToBufferedImage(RawImage rawImage, String format) {
        BufferedImage image = null;

        if ("JPEG".equalsIgnoreCase(format)) {
            //JPEG does not support ARGB without a special encoder
            image =
                    new BufferedImage(
                            rawImage.width, rawImage.height, BufferedImage.TYPE_3BYTE_BGR);
        }
        else {
            image = new BufferedImage(rawImage.width, rawImage.height, BufferedImage.TYPE_INT_ARGB);
        }

        // borrowed conversion logic from platform/sdk/screenshot/.../Screenshot.java
        int index = 0;
        int IndexInc = rawImage.bpp >> 3;
        for (int y = 0 ; y < rawImage.height ; y++) {
            for (int x = 0 ; x < rawImage.width ; x++) {
                int value = rawImage.getARGB(index);
                index += IndexInc;
                image.setRGB(x, y, value);
            }
        }

        return image;
    }

    /**
     * Rescales image cutting it in half.
     *
     * @param image source {@link BufferedImage}.
     * @return resulting scaled image.
     */
    @VisibleForTesting
    BufferedImage rescaleImage(BufferedImage image) {
        int shortEdge = Math.min(image.getHeight(), image.getWidth());
        if (shortEdge > 720) {
            Image resized =
                    image.getScaledInstance(
                            image.getWidth() / 2, image.getHeight() / 2, Image.SCALE_SMOOTH);
            image =
                    new BufferedImage(
                            image.getWidth() / 2, image.getHeight() / 2, Image.SCALE_REPLICATE);
            image.getGraphics().drawImage(resized, 0, 0, null);
        }
        return image;
    }

    /**
     * Gets byte array representation of {@link BufferedImage}.
     *
     * @param image source {@link BufferedImage}.
     * @param format resulting format of image. PNG and JPEG are supported.
     * @return byte array representation of the image.
     */
    @VisibleForTesting
    byte[] getImageData(BufferedImage image, String format) {
        // store compressed image in memory, and let callers write to persistent storage
        // use initial buffer size of 128K
        byte[] imageData = null;
        ByteArrayOutputStream imageOut = new ByteArrayOutputStream(128*1024);
        try {
            if (ImageIO.write(image, format, imageOut)) {
                imageData = imageOut.toByteArray();
            } else {
                CLog.e("Failed to compress screenshot to png");
            }
        } catch (IOException e) {
            CLog.e("Failed to compress screenshot to png");
            CLog.e(e);
        }
        StreamUtil.close(imageOut);
        return imageData;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean clearErrorDialogs() throws DeviceNotAvailableException {
        // attempt to clear error dialogs multiple times
        for (int i = 0; i < NUM_CLEAR_ATTEMPTS; i++) {
            int numErrorDialogs = getErrorDialogCount();
            if (numErrorDialogs == 0) {
                return true;
            }
            doClearDialogs(numErrorDialogs);
        }
        if (getErrorDialogCount() > 0) {
            // at this point, all attempts to clear error dialogs completely have failed
            // it might be the case that the process keeps showing new dialogs immediately after
            // clearing. There's really no workaround, but to dump an error
            CLog.e("error dialogs still exist on %s.", getSerialNumber());
            return false;
        }
        return true;
    }

    /**
     * Detects the number of crash or ANR dialogs currently displayed.
     * <p/>
     * Parses output of 'dump activity processes'
     *
     * @return count of dialogs displayed
     * @throws DeviceNotAvailableException
     */
    private int getErrorDialogCount() throws DeviceNotAvailableException {
        int errorDialogCount = 0;
        Pattern crashPattern = Pattern.compile(".*crashing=true.*AppErrorDialog.*");
        Pattern anrPattern = Pattern.compile(".*notResponding=true.*AppNotRespondingDialog.*");
        String systemStatusOutput = executeShellCommand("dumpsys activity processes");
        Matcher crashMatcher = crashPattern.matcher(systemStatusOutput);
        while (crashMatcher.find()) {
            errorDialogCount++;
        }
        Matcher anrMatcher = anrPattern.matcher(systemStatusOutput);
        while (anrMatcher.find()) {
            errorDialogCount++;
        }

        return errorDialogCount;
    }

    private void doClearDialogs(int numDialogs) throws DeviceNotAvailableException {
        CLog.i("Attempted to clear %d dialogs on %s", numDialogs, getSerialNumber());
        for (int i=0; i < numDialogs; i++) {
            // send DPAD_CENTER
            executeShellCommand(DISMISS_DIALOG_CMD);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void disableKeyguard() throws DeviceNotAvailableException {
        long start = System.currentTimeMillis();
        while (true) {
            Boolean ready = isDeviceInputReady();
            if (ready == null) {
                // unsupported API level, bail
                break;
            }
            if (ready) {
                // input dispatch is ready, bail
                break;
            }
            long timeSpent = System.currentTimeMillis() - start;
            if (timeSpent > INPUT_DISPATCH_READY_TIMEOUT) {
                CLog.w("Timeout after waiting %dms on enabling of input dispatch", timeSpent);
                // break & proceed anyway
                break;
            } else {
                getRunUtil().sleep(1000);
            }
        }
        if (getApiLevel() >= 23) {
            CLog.i(
                    "Attempting to disable keyguard on %s using %s",
                    getSerialNumber(), DISMISS_KEYGUARD_WM_CMD);
            String output = executeShellCommand(DISMISS_KEYGUARD_WM_CMD);
            CLog.i("output of %s: %s", DISMISS_KEYGUARD_WM_CMD, output);
        } else {
            CLog.i("Command: %s, is not supported, falling back to %s", DISMISS_KEYGUARD_WM_CMD,
                    DISMISS_KEYGUARD_CMD);
            executeShellCommand(DISMISS_KEYGUARD_CMD);
        }
        // TODO: check that keyguard was actually dismissed.
    }

    /** {@inheritDoc} */
    @Override
    public KeyguardControllerState getKeyguardState() throws DeviceNotAvailableException {
        String output =
                executeShellCommand("dumpsys activity activities | grep -A3 KeyguardController:");
        CLog.d("Output from KeyguardController: %s", output);
        KeyguardControllerState state =
                KeyguardControllerState.create(Arrays.asList(output.trim().split("\n")));
        return state;
    }

    /**
     * Tests the device to see if input dispatcher is ready
     *
     * @return <code>null</code> if not supported by platform, or the actual readiness state
     * @throws DeviceNotAvailableException
     */
    Boolean isDeviceInputReady() throws DeviceNotAvailableException {
        CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        executeShellCommand(TEST_INPUT_CMD, receiver);
        String output = receiver.getOutput();
        Matcher m = INPUT_DISPATCH_STATE_REGEX.matcher(output);
        if (!m.find()) {
            // output does not contain the line at all, implying unsupported API level, bail
            return null;
        }
        return "1".equals(m.group(1));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void prePostBootSetup() throws DeviceNotAvailableException {
        if (mOptions.isDisableKeyguard()) {
            disableKeyguard();
        }
    }

    /**
     * Performs an reboot via framework power manager
     *
     * Must have root access, device must be API Level 18 or above
     *
     * @param into the mode to reboot into, currently supported: bootloader, recovery, leave it
     *         null for a plain reboot
     * @return <code>true</code> if the device rebooted, <code>false</code> if not successful or
     *          unsupported
     * @throws DeviceNotAvailableException
     */
    private boolean doAdbFrameworkReboot(final String into) throws DeviceNotAvailableException {
        // use framework reboot when:
        // 1. device API level >= 18
        // 2. has adb root
        // 3. framework is running
        if (!isEnableAdbRoot()) {
            CLog.i("framework reboot is not supported; when enable root is disabled");
            return false;
        }
        enableAdbRoot();
        if (getApiLevel() >= 18 && isAdbRoot()) {
            try {
                // check framework running
                String output = executeShellCommand("pm path android");
                if (output == null || !output.contains("package:")) {
                    CLog.v("framework reboot: can't detect framework running");
                    return false;
                }
                String command = "svc power reboot";
                if (into != null && !into.isEmpty()) {
                    command = String.format("%s %s", command, into);
                }
                executeShellCommand(command);
            } catch (DeviceUnresponsiveException due) {
                CLog.v("framework reboot: device unresponsive to shell command, using fallback");
                return false;
            }
            return waitForDeviceNotAvailable(30 * 1000);
        } else {
            CLog.v("framework reboot: not supported");
            return false;
        }
    }

    /**
     * Perform a adb reboot.
     *
     * @param into the bootloader name to reboot into, or <code>null</code> to just reboot the
     *            device.
     * @throws DeviceNotAvailableException
     */
    @Override
    protected void doAdbReboot(final String into) throws DeviceNotAvailableException {
        if (!doAdbFrameworkReboot(into)) {
            DeviceAction rebootAction = new DeviceAction() {
                @Override
                public boolean run() throws TimeoutException, IOException,
                        AdbCommandRejectedException {
                    getIDevice().reboot(into);
                    return true;
                }
            };
            performDeviceAction("reboot", rebootAction, MAX_RETRY_ATTEMPTS);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Set<String> getInstalledPackageNames() throws DeviceNotAvailableException {
        return getInstalledPackageNames(new PkgFilter() {
            @Override
            public boolean accept(String pkgName, String apkPath) {
                return true;
            }
        });
    }

    /**
     * A {@link com.android.tradefed.device.NativeDevice.DeviceAction}
     * for retrieving package system service info, and do retries on
     * failures.
     */
    private class DumpPkgAction implements DeviceAction {

        Map<String, PackageInfo> mPkgInfoMap;

        DumpPkgAction() {
        }

        @Override
        public boolean run() throws IOException, TimeoutException, AdbCommandRejectedException,
                ShellCommandUnresponsiveException, InstallException, SyncException {
            DumpsysPackageReceiver receiver = new DumpsysPackageReceiver();
            getIDevice().executeShellCommand("dumpsys package p", receiver);
            mPkgInfoMap = receiver.getPackages();
            if (mPkgInfoMap.size() == 0) {
                // Package parsing can fail if package manager is currently down. throw exception
                // to retry
                CLog.w("no packages found from dumpsys package p.");
                throw new IOException();
            }
            return true;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Set<String> getUninstallablePackageNames() throws DeviceNotAvailableException {
        DumpPkgAction action = new DumpPkgAction();
        performDeviceAction("dumpsys package p", action, MAX_RETRY_ATTEMPTS);

        Set<String> pkgs = new HashSet<String>();
        for (PackageInfo pkgInfo : action.mPkgInfoMap.values()) {
            if (!pkgInfo.isSystemApp() || pkgInfo.isUpdatedSystemApp()) {
                CLog.d("Found uninstallable package %s", pkgInfo.getPackageName());
                pkgs.add(pkgInfo.getPackageName());
            }
        }
        return pkgs;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public PackageInfo getAppPackageInfo(String packageName) throws DeviceNotAvailableException {
        DumpPkgAction action = new DumpPkgAction();
        performDeviceAction("dumpsys package", action, MAX_RETRY_ATTEMPTS);
        return action.mPkgInfoMap.get(packageName);
    }

    private static interface PkgFilter {
        boolean accept(String pkgName, String apkPath);
    }

    // TODO: convert this to use DumpPkgAction
    private Set<String> getInstalledPackageNames(PkgFilter filter)
            throws DeviceNotAvailableException {
        Set<String> packages= new HashSet<String>();
        String output = executeShellCommand(LIST_PACKAGES_CMD);
        if (output != null) {
            Matcher m = PACKAGE_REGEX.matcher(output);
            while (m.find()) {
                String packagePath = m.group(1);
                String packageName = m.group(2);
                if (filter.accept(packageName, packagePath)) {
                    packages.add(packageName);
                }
            }
        }
        return packages;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ArrayList<Integer> listUsers() throws DeviceNotAvailableException {
        ArrayList<String[]> users = tokenizeListUsers();
        ArrayList<Integer> userIds = new ArrayList<Integer>(users.size());
        for (String[] user : users) {
            userIds.add(Integer.parseInt(user[1]));
        }
        return userIds;
    }

    /**
     * Tokenizes the output of 'pm list users'.
     * The returned tokens for each user have the form: {"\tUserInfo", Integer.toString(id), name,
     * Integer.toHexString(flag), "[running]"}; (the last one being optional)
     * @return a list of arrays of strings, each element of the list representing the tokens
     * for a user, or {@code null} if there was an error while tokenizing the adb command output.
     */
    private ArrayList<String[]> tokenizeListUsers() throws DeviceNotAvailableException {
        String command = "pm list users";
        String commandOutput = executeShellCommand(command);
        // Extract the id of all existing users.
        String[] lines = commandOutput.split("\\r?\\n");
        if (!lines[0].equals("Users:")) {
            throw new DeviceRuntimeException(
                    String.format("'%s' in not a valid output for 'pm list users'", commandOutput));
        }
        ArrayList<String[]> users = new ArrayList<String[]>(lines.length - 1);
        for (int i = 1; i < lines.length; i++) {
            // Individual user is printed out like this:
            // \tUserInfo{$id$:$name$:$Integer.toHexString(flags)$} [running]
            String[] tokens = lines[i].split("\\{|\\}|:");
            if (tokens.length != 4 && tokens.length != 5) {
                throw new DeviceRuntimeException(
                        String.format(
                                "device output: '%s' \nline: '%s' was not in the expected "
                                        + "format for user info.",
                                commandOutput, lines[i]));
            }
            users.add(tokens);
        }
        return users;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getMaxNumberOfUsersSupported() throws DeviceNotAvailableException {
        String command = "pm get-max-users";
        String commandOutput = executeShellCommand(command);
        try {
            return Integer.parseInt(commandOutput.substring(commandOutput.lastIndexOf(" ")).trim());
        } catch (NumberFormatException e) {
            CLog.e("Failed to parse result: %s", commandOutput);
        }
        return 0;
    }

    /** {@inheritDoc} */
    @Override
    public int getMaxNumberOfRunningUsersSupported() throws DeviceNotAvailableException {
        checkApiLevelAgainstNextRelease("get-max-running-users", 28);
        String command = "pm get-max-running-users";
        String commandOutput = executeShellCommand(command);
        try {
            return Integer.parseInt(commandOutput.substring(commandOutput.lastIndexOf(" ")).trim());
        } catch (NumberFormatException e) {
            CLog.e("Failed to parse result: %s", commandOutput);
        }
        return 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isMultiUserSupported() throws DeviceNotAvailableException {
        return getMaxNumberOfUsersSupported() > 1;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int createUser(String name) throws DeviceNotAvailableException, IllegalStateException {
        return createUser(name, false, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int createUser(String name, boolean guest, boolean ephemeral)
            throws DeviceNotAvailableException, IllegalStateException {
        String command ="pm create-user " + (guest ? "--guest " : "")
                + (ephemeral ? "--ephemeral " : "") + name;
        final String output = executeShellCommand(command);
        if (output.startsWith("Success")) {
            try {
                return Integer.parseInt(output.substring(output.lastIndexOf(" ")).trim());
            } catch (NumberFormatException e) {
                CLog.e("Failed to parse result: %s", output);
            }
        }
        throw new IllegalStateException(String.format("Failed to create user: %s", output));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean removeUser(int userId) throws DeviceNotAvailableException {
        final String output = executeShellCommand(String.format("pm remove-user %s", userId));
        if (output.startsWith("Error")) {
            CLog.e("Failed to remove user: %s", output);
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean startUser(int userId) throws DeviceNotAvailableException {
        final String output = executeShellCommand(String.format("am start-user %s", userId));
        if (output.startsWith("Error")) {
            CLog.e("Failed to start user: %s", output);
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean stopUser(int userId) throws DeviceNotAvailableException {
        // No error or status code is returned.
        return stopUser(userId, false, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean stopUser(int userId, boolean waitFlag, boolean forceFlag)
            throws DeviceNotAvailableException {
        final int apiLevel = getApiLevel();
        if (waitFlag && apiLevel < 23) {
            throw new IllegalArgumentException("stop-user -w requires API level >= 23");
        }
        if (forceFlag && apiLevel < 24) {
            throw new IllegalArgumentException("stop-user -f requires API level >= 24");
        }
        StringBuilder cmd = new StringBuilder("am stop-user ");
        if (waitFlag) {
            cmd.append("-w ");
        }
        if (forceFlag) {
            cmd.append("-f ");
        }
        cmd.append(userId);

        CLog.d("stopping user with command: %s", cmd.toString());
        final String output = executeShellCommand(cmd.toString());
        if (output.contains("Error: Can't stop system user")) {
            CLog.e("Cannot stop System user.");
            return false;
        }
        if (output.contains("Can't stop current user")) {
            CLog.e("Cannot stop current user.");
            return false;
        }
        if (isUserRunning(userId)) {
            CLog.w("User Id: %s is still running after the stop-user command.", userId);
            return false;
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Integer getPrimaryUserId() throws DeviceNotAvailableException {
        ArrayList<String[]> users = tokenizeListUsers();
        for (String[] user : users) {
            int flag = Integer.parseInt(user[3], 16);
            if ((flag & FLAG_PRIMARY) != 0) {
                return Integer.parseInt(user[1]);
            }
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getCurrentUser() throws DeviceNotAvailableException {
        checkApiLevelAgainstNextRelease("get-current-user", API_LEVEL_GET_CURRENT_USER);
        final String output = executeShellCommand("am get-current-user");
        try {
            int userId = Integer.parseInt(output.trim());
            return userId;
        } catch (NumberFormatException e) {
            CLog.e(e);
        }
        return INVALID_USER_ID;
    }

    private Matcher findUserInfo(String pmListUsersOutput) {
        Pattern pattern = Pattern.compile(USER_PATTERN);
        Matcher matcher = pattern.matcher(pmListUsersOutput);
        return matcher;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getUserFlags(int userId) throws DeviceNotAvailableException {
        checkApiLevelAgainst("getUserFlags", 22);
        final String commandOutput = executeShellCommand("pm list users");
        Matcher matcher = findUserInfo(commandOutput);
        while(matcher.find()) {
            if (Integer.parseInt(matcher.group(2)) == userId) {
                return Integer.parseInt(matcher.group(6), 16);
            }
        }
        CLog.w("Could not find any flags for userId: %d in output: %s", userId, commandOutput);
        return INVALID_USER_ID;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isUserRunning(int userId) throws DeviceNotAvailableException {
        checkApiLevelAgainst("isUserIdRunning", 22);
        final String commandOutput = executeShellCommand("pm list users");
        Matcher matcher = findUserInfo(commandOutput);
        while(matcher.find()) {
            if (Integer.parseInt(matcher.group(2)) == userId) {
                if (matcher.group(7).contains("running")) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getUserSerialNumber(int userId) throws DeviceNotAvailableException {
        checkApiLevelAgainst("getUserSerialNumber", 22);
        final String commandOutput = executeShellCommand("dumpsys user");
        // example: UserInfo{0:Test:13} serialNo=0
        String userSerialPatter = "(.*\\{)(\\d+)(.*\\})(.*=)(\\d+)";
        Pattern pattern = Pattern.compile(userSerialPatter);
        Matcher matcher = pattern.matcher(commandOutput);
        while(matcher.find()) {
            if (Integer.parseInt(matcher.group(2)) == userId) {
                return Integer.parseInt(matcher.group(5));
            }
        }
        CLog.w("Could not find user serial number for userId: %d, in output: %s",
                userId, commandOutput);
        return INVALID_USER_ID;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean switchUser(int userId) throws DeviceNotAvailableException {
        return switchUser(userId, AM_COMMAND_TIMEOUT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean switchUser(int userId, long timeout) throws DeviceNotAvailableException {
        checkApiLevelAgainstNextRelease("switchUser", API_LEVEL_GET_CURRENT_USER);
        if (userId == getCurrentUser()) {
            CLog.w("Already running as user id: %s. Nothing to be done.", userId);
            return true;
        }
        executeShellCommand(String.format("am switch-user %d", userId));
        long initialTime = getHostCurrentTime();
        while (getHostCurrentTime() - initialTime <= timeout) {
            if (userId == getCurrentUser()) {
                // disable keyguard if option is true
                prePostBootSetup();
                return true;
            } else {
                RunUtil.getDefault().sleep(getCheckNewUserSleep());
            }
        }
        CLog.e("User did not switch in the given %d timeout", timeout);
        return false;
    }

    /**
     * Exposed for testing.
     */
    protected long getCheckNewUserSleep() {
        return CHECK_NEW_USER;
    }

    /**
     * Exposed for testing
     */
    protected long getHostCurrentTime() {
        return System.currentTimeMillis();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean hasFeature(String feature) throws DeviceNotAvailableException {
        final String output = executeShellCommand("pm list features");
        if (output.contains(feature)) {
            return true;
        }
        CLog.w("Feature: %s is not available on %s", feature, getSerialNumber());
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSetting(String namespace, String key) throws DeviceNotAvailableException {
        return getSettingInternal("", namespace.trim(), key.trim());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getSetting(int userId, String namespace, String key)
            throws DeviceNotAvailableException {
        return getSettingInternal(String.format("--user %d", userId), namespace.trim(), key.trim());
    }

    /**
     * Internal Helper to get setting with or without a userId provided.
     */
    private String getSettingInternal(String userFlag, String namespace, String key)
            throws DeviceNotAvailableException {
        namespace = namespace.toLowerCase();
        if (Arrays.asList(SETTINGS_NAMESPACE).contains(namespace)) {
            String cmd = String.format("settings %s get %s %s", userFlag, namespace, key);
            String output = executeShellCommand(cmd);
            if ("null".equals(output)) {
                CLog.w("settings returned null for command: %s. "
                        + "please check if the namespace:key exists", cmd);
                return null;
            }
            return output.trim();
        }
        CLog.e("Namespace requested: '%s' is not part of {system, secure, global}", namespace);
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSetting(String namespace, String key, String value)
            throws DeviceNotAvailableException {
        setSettingInternal("", namespace.trim(), key.trim(), value.trim());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSetting(int userId, String namespace, String key, String value)
            throws DeviceNotAvailableException {
        setSettingInternal(String.format("--user %d", userId), namespace.trim(), key.trim(),
                value.trim());
    }

    /**
     * Internal helper to set a setting with or without a userId provided.
     */
    private void setSettingInternal(String userFlag, String namespace, String key, String value)
            throws DeviceNotAvailableException {
        checkApiLevelAgainst("Changing settings", 22);
        if (Arrays.asList(SETTINGS_NAMESPACE).contains(namespace.toLowerCase())) {
            executeShellCommand(String.format("settings %s put %s %s %s",
                    userFlag, namespace, key, value));
        } else {
            throw new IllegalArgumentException("Namespace must be one of system, secure, global."
                    + " You provided: " + namespace);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAndroidId(int userId) throws DeviceNotAvailableException {
        if (isAdbRoot()) {
            String cmd = String.format(
                    "sqlite3 /data/user/%d/com.google.android.gsf/databases/gservices.db "
                    + "'select value from main where name = \"android_id\"'", userId);
            String output = executeShellCommand(cmd).trim();
            if (!output.contains("unable to open database")) {
                return output;
            }
            CLog.w("Couldn't find android-id, output: %s", output);
        } else {
            CLog.w("adb root is required.");
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<Integer, String> getAndroidIds() throws DeviceNotAvailableException {
        ArrayList<Integer> userIds = listUsers();
        if (userIds == null) {
            return null;
        }
        Map<Integer, String> androidIds = new HashMap<Integer, String>();
        for (Integer id : userIds) {
            String androidId = getAndroidId(id);
            androidIds.put(id, androidId);
        }
        return androidIds;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    IWifiHelper createWifiHelper() throws DeviceNotAvailableException {
        mWasWifiHelperInstalled = true;
        return new WifiHelper(this, mOptions.getWifiUtilAPKPath());
    }

    /**
     * Alternative to {@link #createWifiHelper()} where we can choose whether to do the wifi helper
     * setup or not.
     */
    @VisibleForTesting
    IWifiHelper createWifiHelper(boolean doSetup) throws DeviceNotAvailableException {
        if (doSetup) {
            mWasWifiHelperInstalled = true;
        }
        return new WifiHelper(this, mOptions.getWifiUtilAPKPath(), doSetup);
    }

    /** {@inheritDoc} */
    @Override
    public void postInvocationTearDown() {
        super.postInvocationTearDown();
        // If wifi was installed and it's a real device, attempt to clean it.
        if (mWasWifiHelperInstalled) {
            mWasWifiHelperInstalled = false;
            if (getIDevice() instanceof StubDevice) {
                return;
            }
            if (!TestDeviceState.ONLINE.equals(getDeviceState())) {
                return;
            }
            try {
                // Uninstall the wifi utility if it was installed.
                IWifiHelper wifi = createWifiHelper(false);
                wifi.cleanUp();
            } catch (DeviceNotAvailableException e) {
                CLog.e("Device became unavailable while uninstalling wifi util.");
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean setDeviceOwner(String componentName, int userId)
            throws DeviceNotAvailableException {
        final String command = "dpm set-device-owner --user " + userId + " '" + componentName + "'";
        final String commandOutput = executeShellCommand(command);
        return commandOutput.startsWith("Success:");
    }

    /** {@inheritDoc} */
    @Override
    public boolean removeAdmin(String componentName, int userId)
            throws DeviceNotAvailableException {
        final String command =
                "dpm remove-active-admin --user " + userId + " '" + componentName + "'";
        final String commandOutput = executeShellCommand(command);
        return commandOutput.startsWith("Success:");
    }

    /** {@inheritDoc} */
    @Override
    public void removeOwners() throws DeviceNotAvailableException {
        String command = "dumpsys device_policy";
        String commandOutput = executeShellCommand(command);
        String[] lines = commandOutput.split("\\r?\\n");
        for (int i = 0; i < lines.length; ++i) {
            String line = lines[i].trim();
            if (line.contains("Profile Owner")) {
                // Line is "Profile owner (User <id>):
                String[] tokens = line.split("\\(|\\)| ");
                int userId = Integer.parseInt(tokens[4]);
                i++;
                line = lines[i].trim();
                // Line is admin=ComponentInfo{<component>}
                tokens = line.split("\\{|\\}");
                String componentName = tokens[1];
                CLog.d("Cleaning up profile owner " + userId + " " + componentName);
                removeAdmin(componentName, userId);
            } else if (line.contains("Device Owner:")) {
                i++;
                line = lines[i].trim();
                // Line is admin=ComponentInfo{<component>}
                String[] tokens = line.split("\\{|\\}");
                String componentName = tokens[1];
                // Skip to user id line.
                i += 3;
                line = lines[i].trim();
                // Line is User ID: <N>
                tokens = line.split(":");
                int userId = Integer.parseInt(tokens[1].trim());
                CLog.d("Cleaning up device owner " + userId + " " + componentName);
                removeAdmin(componentName, userId);
            }
        }
    }

    /**
     * Helper for Api level checking of features in the new release before we incremented the api
     * number.
     */
    private void checkApiLevelAgainstNextRelease(String feature, int strictMinLevel)
            throws DeviceNotAvailableException {
        String codeName = getProperty(BUILD_CODENAME_PROP).trim();
        int apiLevel = getApiLevel() + ("REL".equals(codeName) ? 0 : 1);
        if (apiLevel < strictMinLevel){
            throw new IllegalArgumentException(String.format("%s not supported on %s. "
                    + "Must be API %d.", feature, getSerialNumber(), strictMinLevel));
        }
    }

    @Override
    public File dumpHeap(String process, String devicePath) throws DeviceNotAvailableException {
        if (Strings.isNullOrEmpty(devicePath) || Strings.isNullOrEmpty(process)) {
            throw new IllegalArgumentException("devicePath or process cannot be null or empty.");
        }
        String pid = getProcessPid(process);
        if (pid == null) {
            return null;
        }
        File dump = dumpAndPullHeap(pid, devicePath);
        // Clean the device.
        executeShellCommand(String.format("rm %s", devicePath));
        return dump;
    }

    /** Dump the heap file and pull it from the device. */
    private File dumpAndPullHeap(String pid, String devicePath) throws DeviceNotAvailableException {
        executeShellCommand(String.format(DUMPHEAP_CMD, pid, devicePath));
        // Allow a little bit of time for the file to populate on device side.
        int attempt = 0;
        // TODO: add an API to check device file size
        while (!doesFileExist(devicePath) && attempt < 3) {
            getRunUtil().sleep(DUMPHEAP_TIME);
            attempt++;
        }
        File dumpFile = pullFile(devicePath);
        return dumpFile;
    }
}
