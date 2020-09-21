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

import com.android.ddmlib.AndroidDebugBridge.IDeviceChangeListener;
import com.android.ddmlib.DdmPreferences;
import com.android.ddmlib.EmulatorConsole;
import com.android.ddmlib.IDevice;
import com.android.ddmlib.IDevice.DeviceState;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IGlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.IDeviceMonitor.DeviceLister;
import com.android.tradefed.device.IManagedTestDevice.DeviceEventResponse;
import com.android.tradefed.host.IHostOptions;
import com.android.tradefed.log.ILogRegistry.EventType;
import com.android.tradefed.log.LogRegistry;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.SizeLimitedOutputStream;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.TableFormatter;
import com.android.tradefed.util.hostmetric.IHostMonitor;

import com.google.common.annotations.VisibleForTesting;

import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

@OptionClass(alias = "dmgr", global_namespace = false)
public class DeviceManager implements IDeviceManager {

    /** Display string for unknown properties */
    public static final String UNKNOWN_DISPLAY_STRING = "unknown";

    /** max wait time in ms for fastboot devices command to complete */
    private static final long FASTBOOT_CMD_TIMEOUT = 1 * 60 * 1000;
    /** time to wait in ms between fastboot devices requests */
    private static final long FASTBOOT_POLL_WAIT_TIME = 5 * 1000;
    /**
     * time to wait for device adb shell responsive connection before declaring it unavailable for
     * testing
     */
    private static final int CHECK_WAIT_DEVICE_AVAIL_MS = 30 * 1000;

    /* the max size of the emulator output in bytes */
    private static final long MAX_EMULATOR_OUTPUT = 20 * 1024 * 1024;

    /* the emulator output log name */
    private static final String EMULATOR_OUTPUT = "emulator_log";

    /** a {@link DeviceSelectionOptions} that matches any device. Visible for testing. */
    static final IDeviceSelection ANY_DEVICE_OPTIONS = new DeviceSelectionOptions();
    private static final String NULL_DEVICE_SERIAL_PREFIX = "null-device";
    private static final String EMULATOR_SERIAL_PREFIX = "emulator";
    private static final String TCP_DEVICE_SERIAL_PREFIX = "tcp-device";

    /**
     * Pattern for a device listed by 'adb devices':
     *
     * <p>List of devices attached
     *
     * <p>serial1 device
     *
     * <p>serial2 device
     */
    private static final String DEVICE_LIST_PATTERN = "(.*)(\n)(%s)(\\s+)(device)(.*?)";

    private Semaphore mConcurrentFlashLock = null;

    /**
     * This serves both as an indication of whether the flash lock should be used, and as an
     * indicator of whether or not the flash lock has been initialized -- if this is true
     * and {@code mConcurrentFlashLock} is {@code null}, then it has not yet been initialized.
     */
    private Boolean mShouldCheckFlashLock = true;

    protected DeviceMonitorMultiplexer mDvcMon = new DeviceMonitorMultiplexer();
    private Boolean mDvcMonRunning = false;

    private boolean mIsInitialized = false;

    private ManagedDeviceList mManagedDeviceList;

    private IAndroidDebugBridge mAdbBridge;
    private ManagedDeviceListener mManagedDeviceListener;
    protected boolean mFastbootEnabled;
    private Set<IFastbootListener> mFastbootListeners;
    private FastbootMonitor mFastbootMonitor;
    private boolean mIsTerminated = false;
    private IDeviceSelection mGlobalDeviceFilter;

    @Option(name = "max-emulators",
            description = "the maximum number of emulators that can be allocated at one time")
    private int mNumEmulatorSupported = 1;
    @Option(name = "max-null-devices",
            description = "the maximum number of no device runs that can be allocated at one time.")
    private int mNumNullDevicesSupported = 5;
    @Option(name = "max-tcp-devices",
            description = "the maximum number of tcp devices that can be allocated at one time")
    private int mNumTcpDevicesSupported = 1;

    private boolean mSynchronousMode = false;

    @Option(name = "device-recovery-interval",
            description = "the interval in ms between attempts to recover unavailable devices.",
            isTimeVal = true)
    private long mDeviceRecoveryInterval = 30 * 60 * 1000;

    @Option(name = "adb-path", description = "path of the adb binary to use, "
            + "default use the one in $PATH.")
    private String mAdbPath = "adb";

    @Option(name = "fastboot-path", description = "path of the fastboot binary to use, "
            + "default use the one in $PATH.")
    private String mFastbootPath = "fastboot";

    private DeviceRecoverer mDeviceRecoverer;

    private List<IHostMonitor> mGlobalHostMonitors = null;

    /** Counter to wait for the first physical connection before proceeding **/
    private CountDownLatch mFirstDeviceAdded = new CountDownLatch(1);

    /** Flag to remember if adb bridge has been disconnected and needs to be reset * */
    private boolean mAdbBridgeNeedRestart = false;

    /**
     * The DeviceManager should be retrieved from the {@link GlobalConfiguration}
     */
    public DeviceManager() {
    }

    @Override
    public void init() {
        init(null, null);
    }

    /**
     * Initialize the device manager. This must be called once and only once before any other
     * methods are called.
     */
    @Override
    public void init(IDeviceSelection globalDeviceFilter,
            List<IDeviceMonitor> globalDeviceMonitors) {
        init(globalDeviceFilter, globalDeviceMonitors,
                new ManagedTestDeviceFactory(mFastbootEnabled, DeviceManager.this, mDvcMon));
    }

    /**
     * Initialize the device manager. This must be called once and only once before any other
     * methods are called.
     */
    public synchronized void init(IDeviceSelection globalDeviceFilter,
            List<IDeviceMonitor> globalDeviceMonitors, IManagedTestDeviceFactory deviceFactory) {
        if (mIsInitialized) {
            throw new IllegalStateException("already initialized");
        }

        if (globalDeviceFilter == null) {
            globalDeviceFilter = getGlobalConfig().getDeviceRequirements();
        }

        if (globalDeviceMonitors == null) {
            globalDeviceMonitors = getGlobalConfig().getDeviceMonitors();
        }

        mGlobalHostMonitors = getGlobalConfig().getHostMonitors();
        if (mGlobalHostMonitors != null) {
            for (IHostMonitor hm : mGlobalHostMonitors) {
                hm.start();
            }
        }

        mIsInitialized = true;
        mGlobalDeviceFilter = globalDeviceFilter;
        if (globalDeviceMonitors != null) {
            mDvcMon.addMonitors(globalDeviceMonitors);
        }
        mManagedDeviceList = new ManagedDeviceList(deviceFactory);

        final FastbootHelper fastboot = new FastbootHelper(getRunUtil(), mFastbootPath);
        if (fastboot.isFastbootAvailable()) {
            mFastbootListeners = Collections.synchronizedSet(new HashSet<IFastbootListener>());
            mFastbootMonitor = new FastbootMonitor();
            startFastbootMonitor();
            // don't set fastboot enabled bit until mFastbootListeners has been initialized
            mFastbootEnabled = true;
            deviceFactory.setFastbootEnabled(mFastbootEnabled);
            // Populate the fastboot devices
            // TODO: remove when refactoring fastboot handling
            addFastbootDevices();
        } else {
            CLog.w("Fastboot is not available.");
            mFastbootListeners = null;
            mFastbootMonitor = null;
            mFastbootEnabled = false;
            deviceFactory.setFastbootEnabled(mFastbootEnabled);
        }

        // don't start adding devices until fastboot support has been established
        startAdbBridgeAndDependentServices();
    }

    /** Initialize adb connection and services depending on adb connection. */
    private synchronized void startAdbBridgeAndDependentServices() {
        // TODO: Temporarily increase default timeout as workaround for syncFiles timeouts
        DdmPreferences.setTimeOut(120 * 1000);
        mAdbBridge = createAdbBridge();
        mManagedDeviceListener = new ManagedDeviceListener();
        // It's important to add the listener before initializing the ADB bridge to avoid a race
        // condition when detecting devices.
        mAdbBridge.addDeviceChangeListener(mManagedDeviceListener);
        if (mDvcMon != null && !mDvcMonRunning) {
            mDvcMon.setDeviceLister(new DeviceLister() {
                @Override
                public List<DeviceDescriptor> listDevices() {
                    return listAllDevices();
                }
            });
            mDvcMon.run();
            mDvcMonRunning = true;
        }

        mAdbBridge.init(false /* client support */, mAdbPath);
        addEmulators();
        addNullDevices();
        addTcpDevices();

        List<IMultiDeviceRecovery> recoverers = getGlobalConfig().getMultiDeviceRecoveryHandlers();
        if (recoverers != null) {
            for (IMultiDeviceRecovery recoverer : recoverers) {
                recoverer.setFastbootPath(mFastbootPath);
            }
            mDeviceRecoverer = new DeviceRecoverer(recoverers);
            startDeviceRecoverer();
        }
    }


    /**
     * Return if adb bridge has been stopped and needs restart.
     *
     * <p>Exposed for unit testing.
     */
    @VisibleForTesting
    boolean shouldAdbBridgeBeRestarted() {
        return mAdbBridgeNeedRestart;
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void restartAdbBridge() {
        if (mAdbBridgeNeedRestart) {
            mAdbBridgeNeedRestart = false;
            startAdbBridgeAndDependentServices();
        }
    }

    /**
     * Instruct DeviceManager whether to use background threads or not.
     * <p/>
     * Exposed to make unit tests more deterministic.
     *
     * @param syncMode
     */
    void setSynchronousMode(boolean syncMode) {
        mSynchronousMode = syncMode;
    }

    private void checkInit() {
        if (!mIsInitialized) {
            throw new IllegalStateException("DeviceManager has not been initialized");
        }
    }

    /**
     * Start fastboot monitoring.
     * <p/>
     * Exposed for unit testing.
     */
    void startFastbootMonitor() {
        mFastbootMonitor.start();
    }

    /**
     * Start device recovery.
     * <p/>
     * Exposed for unit testing.
     */
    void startDeviceRecoverer() {
        mDeviceRecoverer.start();
    }

    /**
     * Get the {@link IGlobalConfiguration} instance to use.
     * <p />
     * Exposed for unit testing.
     */
    IGlobalConfiguration getGlobalConfig() {
        return GlobalConfiguration.getInstance();
    }

    /**
     * Gets the {@link IHostOptions} instance to use.
     * <p/>
     * Exposed for unit testing
     */
    IHostOptions getHostOptions() {
        return getGlobalConfig().getHostOptions();
    }

    /**
     * Get the {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * Create a {@link RunUtil} instance to use.
     * <p/>
     * Exposed for unit testing.
     */
    IRunUtil createRunUtil() {
        return new RunUtil();
    }

    /**
     * Asynchronously checks if device is available, and adds to queue
     *
     * @param testDevice
     */
    private void checkAndAddAvailableDevice(final IManagedTestDevice testDevice) {
        if (mGlobalDeviceFilter != null && !mGlobalDeviceFilter.matches(testDevice.getIDevice())) {
            CLog.logAndDisplay(LogLevel.INFO, "device %s doesn't match global filter, ignoring",
                    testDevice.getSerialNumber());
            mManagedDeviceList.handleDeviceEvent(testDevice, DeviceEvent.AVAILABLE_CHECK_IGNORED);
            return;
        }

        final String threadName = String.format("Check device %s", testDevice.getSerialNumber());
        Runnable checkRunnable = new Runnable() {
            @Override
            public void run() {
                CLog.d("checking new '%s' '%s' responsiveness", testDevice.getClass().getName(),
                        testDevice.getSerialNumber());
                if (testDevice.getMonitor().waitForDeviceShell(CHECK_WAIT_DEVICE_AVAIL_MS)) {
                    DeviceEventResponse r = mManagedDeviceList.handleDeviceEvent(testDevice,
                            DeviceEvent.AVAILABLE_CHECK_PASSED);
                    if (r.stateChanged && r.allocationState == DeviceAllocationState.Available) {
                        CLog.logAndDisplay(LogLevel.INFO, "Detected new device %s",
                                testDevice.getSerialNumber());
                    } else {
                        CLog.d("Device %s failed or ignored responsiveness check, ",
                                testDevice.getSerialNumber());
                    }
                } else {
                    DeviceEventResponse r = mManagedDeviceList.handleDeviceEvent(testDevice,
                            DeviceEvent.AVAILABLE_CHECK_FAILED);
                    if (r.stateChanged && r.allocationState == DeviceAllocationState.Unavailable) {
                        CLog.w("Device %s is unresponsive, will not be available for testing",
                                testDevice.getSerialNumber());
                    }
                }
            }
        };
        if (mSynchronousMode) {
            checkRunnable.run();
        } else {
            Thread checkThread = new Thread(checkRunnable, threadName);
            // Device checking threads shouldn't hold the JVM open
            checkThread.setName("DeviceManager-checkRunnable");
            checkThread.setDaemon(true);
            checkThread.start();
        }
    }

    /**
     * Add placeholder objects for the max number of 'no device required' concurrent allocations
     */
    private void addNullDevices() {
        for (int i = 0; i < mNumNullDevicesSupported; i++) {
            addAvailableDevice(new NullDevice(String.format("%s-%d", NULL_DEVICE_SERIAL_PREFIX, i)));
        }
    }

    /**
     * Add placeholder objects for the max number of emulators that can be allocated
     */
    private void addEmulators() {
        // TODO currently this means 'additional emulators not already running'
        int port = 5554;
        for (int i = 0; i < mNumEmulatorSupported; i++) {
            addAvailableDevice(new StubDevice(String.format("%s-%d", EMULATOR_SERIAL_PREFIX, port),
                    true));
            port += 2;
        }
    }

    /**
     * Add placeholder objects for the max number of tcp devices that can be connected
     */
    private void addTcpDevices() {
        for (int i = 0; i < mNumTcpDevicesSupported; i++) {
            addAvailableDevice(new TcpDevice(String.format("%s-%d", TCP_DEVICE_SERIAL_PREFIX, i)));
        }
    }

    public void addAvailableDevice(IDevice stubDevice) {
        IManagedTestDevice d = mManagedDeviceList.findOrCreate(stubDevice);
        if (d != null) {
            mManagedDeviceList.handleDeviceEvent(d, DeviceEvent.FORCE_AVAILABLE);
        } else {
            CLog.e("Could not create stub device");
        }
    }

    private void addFastbootDevices() {
        final FastbootHelper fastboot = new FastbootHelper(getRunUtil(), mFastbootPath);
        Set<String> serials = fastboot.getDevices();
        for (String serial : serials) {
            FastbootDevice d = new FastbootDevice(serial);
            if (mGlobalDeviceFilter != null && mGlobalDeviceFilter.matches(d)) {
                addAvailableDevice(d);
            }
        }
    }

    public static class FastbootDevice extends StubDevice {
        public FastbootDevice(String serial) {
            super(serial, false);
        }
    }

    /**
     * Creates a {@link IDeviceStateMonitor} to use.
     * <p/>
     * Exposed so unit tests can mock
     */
    IDeviceStateMonitor createStateMonitor(IDevice device) {
        return new DeviceStateMonitor(this, device, mFastbootEnabled);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice allocateDevice() {
        return allocateDevice(ANY_DEVICE_OPTIONS);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice allocateDevice(IDeviceSelection options) {
        checkInit();
        return mManagedDeviceList.allocate(options);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice forceAllocateDevice(String serial) {
        checkInit();
        IManagedTestDevice d = mManagedDeviceList.findOrCreate(new StubDevice(serial, false));
        if (d != null) {
            DeviceEventResponse r = d.handleAllocationEvent(DeviceEvent.FORCE_ALLOCATE_REQUEST);
            if (r.stateChanged && r.allocationState == DeviceAllocationState.Allocated) {
                return d;
            }
        }
        return null;
    }

    /**
     * Creates the {@link IAndroidDebugBridge} to use.
     * <p/>
     * Exposed so tests can mock this.
     * @return the {@link IAndroidDebugBridge}
     */
    synchronized IAndroidDebugBridge createAdbBridge() {
        return new AndroidDebugBridgeWrapper();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void freeDevice(ITestDevice device, FreeDeviceState deviceState) {
        checkInit();
        IManagedTestDevice managedDevice = (IManagedTestDevice)device;
        // force stop capturing logcat just to be sure
        managedDevice.stopLogcat();
        IDevice ideviceToReturn = device.getIDevice();
        // don't kill emulator if it wasn't launched by launchEmulator (ie emulatorProcess is null).
        if (ideviceToReturn.isEmulator() && managedDevice.getEmulatorProcess() != null) {
            try {
                killEmulator(device);
                // stop emulator output log
                device.stopEmulatorOutput();
                // emulator killed - return a stub device
                // TODO: this is a bit of a hack. Consider having DeviceManager inject a StubDevice
                // when deviceDisconnected event is received
                ideviceToReturn = new StubDevice(ideviceToReturn.getSerialNumber(), true);
                deviceState = FreeDeviceState.AVAILABLE;
                managedDevice.setIDevice(ideviceToReturn);
            } catch (DeviceNotAvailableException e) {
                CLog.e(e);
                deviceState = FreeDeviceState.UNAVAILABLE;
            }
        }
        if (ideviceToReturn instanceof TcpDevice) {
            // Make sure the device goes back to the original state.
            managedDevice.setDeviceState(TestDeviceState.NOT_AVAILABLE);
        }
        DeviceEventResponse r = mManagedDeviceList.handleDeviceEvent(managedDevice,
                getEventFromFree(managedDevice, deviceState));
        if (r != null && !r.stateChanged) {
            CLog.e("Device %s was in unexpected state %s when freeing", device.getSerialNumber(),
                    r.allocationState.toString());
        }
    }

    /**
     * Helper method to convert from a {@link com.android.tradefed.device.FreeDeviceState} to a
     * {@link com.android.tradefed.device.DeviceEvent}
     *
     * @param managedDevice
     */
    private DeviceEvent getEventFromFree(
            IManagedTestDevice managedDevice, FreeDeviceState deviceState) {
        switch (deviceState) {
            case UNRESPONSIVE:
                return DeviceEvent.FREE_UNRESPONSIVE;
            case AVAILABLE:
                return DeviceEvent.FREE_AVAILABLE;
            case UNAVAILABLE:
                // We double check if device is still showing in adb or not to confirm the
                // connection is gone.
                if (TestDeviceState.NOT_AVAILABLE.equals(managedDevice.getDeviceState())) {
                    String devices = executeGlobalAdbCommand("devices");
                    Pattern p =
                            Pattern.compile(
                                    String.format(
                                            DEVICE_LIST_PATTERN, managedDevice.getSerialNumber()));
                    if (devices == null || !p.matcher(devices).find()) {
                        return DeviceEvent.FREE_UNKNOWN;
                    }
                }
                return DeviceEvent.FREE_UNAVAILABLE;
            case IGNORE:
                return DeviceEvent.FREE_UNKNOWN;
        }
        throw new IllegalStateException("unknown FreeDeviceState");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void launchEmulator(ITestDevice device, long bootTimeout, IRunUtil runUtil,
            List<String> emulatorArgs)
            throws DeviceNotAvailableException {
        if (!device.getIDevice().isEmulator()) {
            throw new IllegalStateException(String.format("Device %s is not an emulator",
                    device.getSerialNumber()));
        }
        if (!device.getDeviceState().equals(TestDeviceState.NOT_AVAILABLE)) {
            throw new IllegalStateException(String.format(
                    "Emulator device %s is in state %s. Expected: %s", device.getSerialNumber(),
                    device.getDeviceState(), TestDeviceState.NOT_AVAILABLE));
        }
        List<String> fullArgs = new ArrayList<String>(emulatorArgs);

        try {
            CLog.i("launching emulator with %s", fullArgs.toString());
            SizeLimitedOutputStream emulatorOutput = new SizeLimitedOutputStream(
                    MAX_EMULATOR_OUTPUT, EMULATOR_OUTPUT, ".txt");
            Process p = runUtil.runCmdInBackground(fullArgs, emulatorOutput);
            // sleep a small amount to wait for process to start successfully
            getRunUtil().sleep(500);
            assertEmulatorProcessAlive(p, device);
            TestDevice testDevice = (TestDevice) device;
            testDevice.setEmulatorProcess(p);
            testDevice.setEmulatorOutputStream(emulatorOutput);
        } catch (IOException e) {
            // TODO: is this the most appropriate exception to throw?
            throw new DeviceNotAvailableException("Failed to start emulator process", e,
                    device.getSerialNumber());
        }

        device.waitForDeviceAvailable(bootTimeout);
    }

    private void assertEmulatorProcessAlive(Process p, ITestDevice device)
            throws DeviceNotAvailableException {
        if (!isProcessRunning(p)) {
            try {
                CLog.e("Emulator process has died . stdout: '%s', stderr: '%s'",
                        StreamUtil.getStringFromStream(p.getInputStream()),
                        StreamUtil.getStringFromStream(p.getErrorStream()));
            } catch (IOException e) {
                // ignore
            }
            throw new DeviceNotAvailableException("emulator died after launch",
                    device.getSerialNumber());
        }
    }

    /**
     * Check if emulator process has died
     *
     * @param p the {@link Process} to check
     * @return true if process is running, false otherwise
     */
    private boolean isProcessRunning(Process p) {
        try {
            p.exitValue();
        } catch (IllegalThreadStateException e) {
            // expected if process is still alive
            return true;
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void killEmulator(ITestDevice device) throws DeviceNotAvailableException {
        EmulatorConsole console = EmulatorConsole.getConsole(device.getIDevice());
        if (console != null) {
            console.kill();
            // check and wait for device to become not avail
            device.waitForDeviceNotAvailable(5 * 1000);
            // lets ensure process is killed too - fall through
        } else {
            CLog.w("Could not get emulator console for %s", device.getSerialNumber());
        }
        // lets try killing the process
        Process emulatorProcess = ((IManagedTestDevice) device).getEmulatorProcess();
        if (emulatorProcess != null) {
            emulatorProcess.destroy();
            if (isProcessRunning(emulatorProcess)) {
                CLog.w("Emulator process still running after destroy for %s",
                        device.getSerialNumber());
                forceKillProcess(emulatorProcess, device.getSerialNumber());
            }
        }
        if (!device.waitForDeviceNotAvailable(20 * 1000)) {
            throw new DeviceNotAvailableException(String.format("Failed to kill emulator %s",
                    device.getSerialNumber()), device.getSerialNumber());
        }
    }

    /**
     * Disgusting hack alert! Attempt to force kill given process.
     * Relies on implementation details. Only works on linux
     *
     * @param emulatorProcess the {@link Process} to kill
     * @param emulatorSerial the serial number of emulator. Only used for logging
     */
    private void forceKillProcess(Process emulatorProcess, String emulatorSerial) {
        if (emulatorProcess.getClass().getName().equals("java.lang.UNIXProcess")) {
            try {
                CLog.i("Attempting to force kill emulator process for %s", emulatorSerial);
                Field f = emulatorProcess.getClass().getDeclaredField("pid");
                f.setAccessible(true);
                Integer pid = (Integer)f.get(emulatorProcess);
                if (pid != null) {
                    RunUtil.getDefault().runTimedCmd(5 * 1000, "kill", "-9", pid.toString());
                }
            } catch (NoSuchFieldException e) {
                CLog.d("got NoSuchFieldException when attempting to read process pid");
            } catch (IllegalAccessException e) {
                CLog.d("got IllegalAccessException when attempting to read process pid");
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice connectToTcpDevice(String ipAndPort) {
        ITestDevice tcpDevice = forceAllocateDevice(ipAndPort);
        if (tcpDevice == null) {
            return null;
        }
        if (doAdbConnect(ipAndPort)) {
            try {
                tcpDevice.setRecovery(new WaitDeviceRecovery());
                tcpDevice.waitForDeviceOnline();
                return tcpDevice;
            } catch (DeviceNotAvailableException e) {
                CLog.w("Device with tcp serial %s did not come online", ipAndPort);
            }
        }
        freeDevice(tcpDevice, FreeDeviceState.IGNORE);
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice reconnectDeviceToTcp(ITestDevice usbDevice)
            throws DeviceNotAvailableException {
        CLog.i("Reconnecting device %s to adb over tcpip", usbDevice.getSerialNumber());
        ITestDevice tcpDevice = null;
        if (usbDevice instanceof IManagedTestDevice) {
            IManagedTestDevice managedUsbDevice = (IManagedTestDevice) usbDevice;
            String ipAndPort = managedUsbDevice.switchToAdbTcp();
            if (ipAndPort != null) {
                CLog.d("Device %s was switched to adb tcp on %s", usbDevice.getSerialNumber(),
                        ipAndPort);
                tcpDevice = connectToTcpDevice(ipAndPort);
                if (tcpDevice == null) {
                    // ruh roh, could not connect to device
                    // Try to re-establish connection back to usb device
                    managedUsbDevice.recoverDevice();
                }
            }
        } else {
            CLog.e("reconnectDeviceToTcp: unrecognized device type.");
        }
        return tcpDevice;
    }

    @Override
    public boolean disconnectFromTcpDevice(ITestDevice tcpDevice) {
        CLog.i("Disconnecting and freeing tcp device %s", tcpDevice.getSerialNumber());
        boolean result = false;
        try {
            result = tcpDevice.switchToAdbUsb();
        } catch (DeviceNotAvailableException e) {
            CLog.w("Failed to switch device %s to usb mode: %s", tcpDevice.getSerialNumber(),
                    e.getMessage());
        }
        freeDevice(tcpDevice, FreeDeviceState.IGNORE);
        return result;
    }

    private boolean doAdbConnect(String ipAndPort) {
        final String resultSuccess = String.format("connected to %s", ipAndPort);
        for (int i = 1; i <= 3; i++) {
            String adbConnectResult = executeGlobalAdbCommand("connect", ipAndPort);
            // runcommand "adb connect ipAndPort"
            if (adbConnectResult != null && adbConnectResult.startsWith(resultSuccess)) {
                return true;
            }
            CLog.w("Failed to connect to device on %s, attempt %d of 3. Response: %s.",
                    ipAndPort, i, adbConnectResult);
            getRunUtil().sleep(5 * 1000);
        }
        return false;
    }

    /**
     * Execute a adb command not targeted to a particular device eg. 'adb connect'
     *
     * @param cmdArgs
     * @return std output if the command succeedm null otherwise.
     */
    public String executeGlobalAdbCommand(String... cmdArgs) {
        String[] fullCmd = ArrayUtil.buildArray(new String[] {"adb"}, cmdArgs);
        CommandResult result = getRunUtil().runTimedCmd(FASTBOOT_CMD_TIMEOUT, fullCmd);
        if (CommandStatus.SUCCESS.equals(result.getStatus())) {
            return result.getStdout();
        }
        CLog.w("adb %s failed", cmdArgs[0]);
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void terminate() {
        checkInit();
        if (!mIsTerminated) {
            mIsTerminated = true;
            stopAdbBridgeAndDependentServices();
            // We are not terminating mFastbootMonitor here since it is a daemon thread.
            // Early terminating it can cause other threads to be blocked if they check
            // fastboot state of a device.
            if (mGlobalHostMonitors != null ) {
                for (IHostMonitor hm : mGlobalHostMonitors) {
                    hm.terminate();
                }
            }
        }
    }

    /** Stop adb bridge and services depending on adb connection. */
    private synchronized void stopAdbBridgeAndDependentServices() {
        terminateDeviceRecovery();
        mAdbBridge.removeDeviceChangeListener(mManagedDeviceListener);
        mAdbBridge.terminate();
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void stopAdbBridge() {
        stopAdbBridgeAndDependentServices();
        mAdbBridgeNeedRestart = true;
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void terminateDeviceRecovery() {
        if (mDeviceRecoverer != null) {
            mDeviceRecoverer.terminate();
        }
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void terminateDeviceMonitor() {
        mDvcMon.stop();
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void terminateHard() {
        checkInit();
        if (!mIsTerminated ) {
            for (IManagedTestDevice device : mManagedDeviceList) {
                device.setRecovery(new AbortRecovery());
            }
            mAdbBridge.disconnectBridge();
            terminate();
        }
    }

    private static class AbortRecovery implements IDeviceRecovery {

        /**
         * {@inheritDoc}
         */
        @Override
        public void recoverDevice(IDeviceStateMonitor monitor, boolean recoverUntilOnline)
                throws DeviceNotAvailableException {
            throw new DeviceNotAvailableException("aborted test session",
                    monitor.getSerialNumber());
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void recoverDeviceBootloader(IDeviceStateMonitor monitor)
                throws DeviceNotAvailableException {
            throw new DeviceNotAvailableException("aborted test session",
                    monitor.getSerialNumber());
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void recoverDeviceRecovery(IDeviceStateMonitor monitor)
                throws DeviceNotAvailableException {
            throw new DeviceNotAvailableException("aborted test session",
                    monitor.getSerialNumber());
        }
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public List<DeviceDescriptor> listAllDevices() {
        final List<DeviceDescriptor> serialStates = new ArrayList<DeviceDescriptor>();
        IDeviceSelection selector = getDeviceSelectionOptions();
        for (IManagedTestDevice d : mManagedDeviceList) {
            IDevice idevice = d.getIDevice();
            serialStates.add(
                    new DeviceDescriptor(
                            idevice.getSerialNumber(),
                            idevice instanceof StubDevice,
                            idevice.getState(),
                            d.getAllocationState(),
                            getDisplay(selector.getDeviceProductType(idevice)),
                            getDisplay(selector.getDeviceProductVariant(idevice)),
                            getDisplay(idevice.getProperty("ro.build.version.sdk")),
                            getDisplay(idevice.getProperty("ro.build.id")),
                            getDisplay(selector.getBatteryLevel(idevice)),
                            d.getDeviceClass(),
                            getDisplay(d.getMacAddress()),
                            getDisplay(d.getSimState()),
                            getDisplay(d.getSimOperator()),
                            idevice));
        }
        return serialStates;
    }

    @Override
    public void displayDevicesInfo(PrintWriter stream) {
        ArrayList<List<String>> displayRows = new ArrayList<List<String>>();
        displayRows.add(Arrays.asList("Serial", "State", "Allocation", "Product", "Variant",
                "Build", "Battery"));
        List<DeviceDescriptor> deviceList = listAllDevices();
        sortDeviceList(deviceList);
        addDevicesInfo(displayRows, deviceList);
        new TableFormatter().displayTable(displayRows, stream);
    }

    /**
     * Sorts list by state, then by serial.
     */
    @VisibleForTesting
    static List<DeviceDescriptor> sortDeviceList(List<DeviceDescriptor> deviceList) {

        Comparator<DeviceDescriptor> c = new Comparator<DeviceDescriptor>() {

            @Override
            public int compare(DeviceDescriptor o1, DeviceDescriptor o2) {
                if (o1.getState() != o2.getState()) {
                    // sort by state
                    return o1.getState().toString()
                            .compareTo(o2.getState().toString());
                }
                // states are equal, sort by serial
                return o1.getSerial().compareTo(o2.getSerial());
            }

        };
        Collections.sort(deviceList, c);
        return deviceList;
    }

    /**
     * Get the {@link IDeviceSelection} to use to display device info
     * <p/>
     * Exposed for unit testing.
     */
    IDeviceSelection getDeviceSelectionOptions() {
        return new DeviceSelectionOptions();
    }

    private void addDevicesInfo(List<List<String>> displayRows,
            List<DeviceDescriptor> sortedDeviceList) {
        for (DeviceDescriptor desc : sortedDeviceList) {
            if (desc.isStubDevice() &&
                    desc.getState() != DeviceAllocationState.Allocated) {
                // don't add placeholder devices
                continue;
            }
            displayRows.add(Arrays.asList(
                    desc.getSerial(),
                    desc.getDeviceState().toString(),
                    desc.getState().toString(),
                    desc.getProduct(),
                    desc.getProductVariant(),
                    desc.getBuildId(),
                    desc.getBatteryLevel())
                    );
        }
    }

    /**
     * Return the displayable string for given object
     */
    private String getDisplay(Object o) {
        return o == null ? UNKNOWN_DISPLAY_STRING : o.toString();
    }

    /**
     * A class to listen for and act on device presence updates from ddmlib
     */
    private class ManagedDeviceListener implements IDeviceChangeListener {

        /**
         * {@inheritDoc}
         */
        @Override
        public void deviceChanged(IDevice idevice, int changeMask) {
            if ((changeMask & IDevice.CHANGE_STATE) != 0) {
                IManagedTestDevice testDevice = mManagedDeviceList.findOrCreate(idevice);
                if (testDevice == null) {
                    return;
                }
                TestDeviceState newState = TestDeviceState.getStateByDdms(idevice.getState());
                testDevice.setDeviceState(newState);
                if (newState == TestDeviceState.ONLINE) {
                    DeviceEventResponse r = mManagedDeviceList.handleDeviceEvent(testDevice,
                            DeviceEvent.STATE_CHANGE_ONLINE);
                    if (r.stateChanged && r.allocationState ==
                            DeviceAllocationState.Checking_Availability) {
                        checkAndAddAvailableDevice(testDevice);
                    }
                } else if (DeviceState.OFFLINE.equals(idevice.getState()) ||
                        DeviceState.UNAUTHORIZED.equals(idevice.getState())) {
                    // handle device changing to offline or unauthorized.
                    mManagedDeviceList.handleDeviceEvent(testDevice,
                            DeviceEvent.STATE_CHANGE_OFFLINE);
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void deviceConnected(IDevice idevice) {
            CLog.d("Detected device connect %s, id %d", idevice.getSerialNumber(),
                    idevice.hashCode());
            String threadName = String.format("Connected device %s", idevice.getSerialNumber());
            Runnable connectedRunnable =
                    new Runnable() {
                        @Override
                        public void run() {
                            IManagedTestDevice testDevice =
                                    mManagedDeviceList.findOrCreate(idevice);
                            if (testDevice == null) {
                                return;
                            }
                            // DDMS will allocate a new IDevice, so need
                            // to update the TestDevice record with the new device
                            CLog.d("Updating IDevice for device %s", idevice.getSerialNumber());
                            testDevice.setIDevice(idevice);
                            TestDeviceState newState =
                                    TestDeviceState.getStateByDdms(idevice.getState());
                            testDevice.setDeviceState(newState);
                            if (newState == TestDeviceState.ONLINE) {
                                DeviceEventResponse r =
                                        mManagedDeviceList.handleDeviceEvent(
                                                testDevice, DeviceEvent.CONNECTED_ONLINE);
                                if (r.stateChanged
                                        && r.allocationState
                                                == DeviceAllocationState.Checking_Availability) {
                                    checkAndAddAvailableDevice(testDevice);
                                }
                                logDeviceEvent(
                                        EventType.DEVICE_CONNECTED, testDevice.getSerialNumber());
                            } else if (DeviceState.OFFLINE.equals(idevice.getState())
                                    || DeviceState.UNAUTHORIZED.equals(idevice.getState())) {
                                mManagedDeviceList.handleDeviceEvent(
                                        testDevice, DeviceEvent.CONNECTED_OFFLINE);
                                logDeviceEvent(
                                        EventType.DEVICE_CONNECTED_OFFLINE,
                                        testDevice.getSerialNumber());
                            }
                            mFirstDeviceAdded.countDown();
                        }
                    };

            if (mSynchronousMode) {
                connectedRunnable.run();
            } else {
                // Device creation step can take a little bit of time, so do it in a thread to
                // avoid blocking following events of new devices
                Thread checkThread = new Thread(connectedRunnable, threadName);
                // Device checking threads shouldn't hold the JVM open
                checkThread.setDaemon(true);
                checkThread.start();
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void deviceDisconnected(IDevice disconnectedDevice) {
            IManagedTestDevice d = mManagedDeviceList.find(disconnectedDevice.getSerialNumber());
            if (d != null) {
                mManagedDeviceList.handleDeviceEvent(d, DeviceEvent.DISCONNECTED);
                d.setDeviceState(TestDeviceState.NOT_AVAILABLE);
                logDeviceEvent(EventType.DEVICE_DISCONNECTED, disconnectedDevice.getSerialNumber());
            }
        }
    }

    /** Helper to log the device events. */
    @VisibleForTesting
    void logDeviceEvent(EventType event, String serial) {
        Map<String, String> args = new HashMap<>();
        args.put("serial", serial);
        LogRegistry.getLogRegistry().logEvent(LogLevel.DEBUG, event, args);
    }

    /** {@inheritDoc} */
    @Override
    public boolean waitForFirstDeviceAdded(long timeout) {
        try {
            return mFirstDeviceAdded.await(timeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addFastbootListener(IFastbootListener listener) {
        checkInit();
        if (mFastbootEnabled) {
            mFastbootListeners.add(listener);
        } else {
            throw new UnsupportedOperationException("fastboot is not enabled");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void removeFastbootListener(IFastbootListener listener) {
        checkInit();
        if (mFastbootEnabled) {
            mFastbootListeners.remove(listener);
        }
    }

    /**
     * A class to monitor and update fastboot state of devices.
     */
    private class FastbootMonitor extends Thread {

        private boolean mQuit = false;

        FastbootMonitor() {
            super("FastbootMonitor");
            setDaemon(true);
        }

        @Override
        public void interrupt() {
            mQuit = true;
            super.interrupt();
        }

        @Override
        public void run() {
            final FastbootHelper fastboot = new FastbootHelper(getRunUtil(), mFastbootPath);
            while (!mQuit) {
                Set<String> serials = fastboot.getDevices();
                if (serials != null) {
                    // Update known fastboot devices state
                    mManagedDeviceList.updateFastbootStates(serials);
                    // Add new fastboot devices.
                    for (String serial : serials) {
                        FastbootDevice d = new FastbootDevice(serial);
                        if (mGlobalDeviceFilter != null && mGlobalDeviceFilter.matches(d)) {
                            addAvailableDevice(d);
                        }
                    }
                }
                if (!mFastbootListeners.isEmpty()) {
                    // create a copy of listeners for notification to prevent deadlocks
                    Collection<IFastbootListener> listenersCopy =
                            new ArrayList<IFastbootListener>(mFastbootListeners.size());
                    listenersCopy.addAll(mFastbootListeners);
                    for (IFastbootListener listener : listenersCopy) {
                        listener.stateUpdated();
                    }
                }
                getRunUtil().sleep(FASTBOOT_POLL_WAIT_TIME);
            }
        }
    }

    /**
     * A class for a thread which performs periodic device recovery operations.
     */
    private class DeviceRecoverer extends Thread {

        private boolean mQuit = false;
        private List<IMultiDeviceRecovery> mMultiDeviceRecoverers;

        public DeviceRecoverer(List<IMultiDeviceRecovery> multiDeviceRecoverers) {
            super("DeviceRecoverer");
            mMultiDeviceRecoverers = multiDeviceRecoverers;
            // Ensure that this thread doesn't prevent TF from terminating
            setDaemon(true);
        }

        @Override
        public void run() {
            while (!mQuit) {
                getRunUtil().sleep(mDeviceRecoveryInterval);
                if (mQuit) {
                    // After the sleep time, we check if we should run or not.
                    return;
                }
                if (mMultiDeviceRecoverers != null && !mMultiDeviceRecoverers.isEmpty()) {
                    for (IMultiDeviceRecovery m : mMultiDeviceRecoverers) {
                        m.recoverDevices(getDeviceList());
                    }
                }
            }
        }

        public void terminate() {
            mQuit = true;
            interrupt();
        }
    }

    @VisibleForTesting
    List<IManagedTestDevice> getDeviceList() {
        return mManagedDeviceList.getCopy();
    }

    @VisibleForTesting
    void setMaxEmulators(int numEmulators) {
        mNumEmulatorSupported = numEmulators;
    }

    @VisibleForTesting
    void setMaxNullDevices(int nullDevices) {
        mNumNullDevicesSupported = nullDevices;
    }

    @VisibleForTesting
    void setMaxTcpDevices(int tcpDevices) {
        mNumTcpDevicesSupported = tcpDevices;
    }

    @Override
    public boolean isNullDevice(String serial) {
        return serial.startsWith(NULL_DEVICE_SERIAL_PREFIX);
    }

    @Override
    public boolean isEmulator(String serial) {
        return serial.startsWith(EMULATOR_SERIAL_PREFIX);
    }

    @Override
    public void addDeviceMonitor(IDeviceMonitor mon) {
        mDvcMon.addMonitor(mon);
    }

    @Override
    public void removeDeviceMonitor(IDeviceMonitor mon) {
        mDvcMon.removeMonitor(mon);
    }

    @Override
    public String getFastbootPath() {
        return mFastbootPath;
    }

    /**
     * Set the state of the concurrent flash limit implementation
     *
     * Exposed for unit testing
     */
    void setConcurrentFlashSettings(Semaphore flashLock, boolean shouldCheck) {
        synchronized (mShouldCheckFlashLock) {
            mConcurrentFlashLock = flashLock;
            mShouldCheckFlashLock = shouldCheck;
        }
    }

    Semaphore getConcurrentFlashLock() {
        return mConcurrentFlashLock;
    }

    /** Initialize the concurrent flash lock semaphore **/
    private void initConcurrentFlashLock() {
        if (!mShouldCheckFlashLock) return;
        // The logic below is to avoid multi-thread race conditions while initializing
        // mConcurrentFlashLock when we hit this condition.
        if (mConcurrentFlashLock == null) {
            // null with mShouldCheckFlashLock == true means initialization hasn't been done yet
            synchronized(mShouldCheckFlashLock) {
                // Check all state again, since another thread might have gotten here first
                if (!mShouldCheckFlashLock) return;

                IHostOptions hostOptions = getHostOptions();
                Integer concurrentFlashingLimit = hostOptions.getConcurrentFlasherLimit();

                if (concurrentFlashingLimit == null) {
                    mShouldCheckFlashLock = false;
                    return;
                }

                if (mConcurrentFlashLock == null) {
                    mConcurrentFlashLock = new Semaphore(concurrentFlashingLimit, true /* fair */);
                }
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public int getAvailableFlashingPermits() {
        initConcurrentFlashLock();
        if (mConcurrentFlashLock != null) {
            return mConcurrentFlashLock.availablePermits();
        }
        IHostOptions hostOptions = getHostOptions();
        if (hostOptions.getConcurrentFlasherLimit() != null) {
            return hostOptions.getConcurrentFlasherLimit();
        }
        return Integer.MAX_VALUE;
    }

    /** {@inheritDoc} */
    @Override
    public void takeFlashingPermit() {
        initConcurrentFlashLock();
        if (!mShouldCheckFlashLock) return;

        IHostOptions hostOptions = getHostOptions();
        Integer concurrentFlashingLimit = hostOptions.getConcurrentFlasherLimit();
        CLog.i(
                "Requesting a flashing permit out of the max limit of %s. Current queue "
                        + "length: %s",
                concurrentFlashingLimit,
                mConcurrentFlashLock.getQueueLength());
        mConcurrentFlashLock.acquireUninterruptibly();
    }

    /** {@inheritDoc} */
    @Override
    public void returnFlashingPermit() {
        if (mConcurrentFlashLock != null) {
            mConcurrentFlashLock.release();
        }
    }

    /** {@inheritDoc} */
    @Override
    public String getAdbVersion() {
        return mAdbBridge.getAdbVersion(mAdbPath);
    }
}
