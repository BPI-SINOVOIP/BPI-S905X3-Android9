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

import com.android.ddmlib.IDevice;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceManager.FastbootDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/**
 * Container for for device selection criteria.
 */
public class DeviceSelectionOptions implements IDeviceSelection {

    @Option(name = "serial", shortName = 's', description =
        "run this test on a specific device with given serial number(s).")
    private Collection<String> mSerials = new ArrayList<String>();

    @Option(name = "exclude-serial", description =
        "run this test on any device except those with this serial number(s).")
    private Collection<String> mExcludeSerials = new ArrayList<String>();

    @Option(name = "product-type", description =
            "run this test on device with this product type(s).  May also filter by variant " +
            "using product:variant.")
    private Collection<String> mProductTypes = new ArrayList<String>();

    @Option(name = "property", description =
        "run this test on device with this property value. " +
        "Expected format --property <propertyname> <propertyvalue>.")
    private Map<String, String> mPropertyMap = new HashMap<>();

    @Option(name = "emulator", shortName = 'e', description =
        "force this test to run on emulator.")
    private boolean mEmulatorRequested = false;

    @Option(name = "device", shortName = 'd', description =
        "force this test to run on a physical device, not an emulator.")
    private boolean mDeviceRequested = false;

    @Option(name = "new-emulator", description =
        "allocate a placeholder emulator. Should be used when config intends to launch an emulator")
    private boolean mStubEmulatorRequested = false;

    @Option(name = "null-device", shortName = 'n', description =
        "do not allocate a device for this test.")
    private boolean mNullDeviceRequested = false;

    @Option(name = "tcp-device", description =
            "start a placeholder for a tcp device that will be connected later.")
    private boolean mTcpDeviceRequested = false;

    @Option(name = "min-battery", description =
        "only run this test on a device whose battery level is at least the given amount. " +
        "Scale: 0-100")
    private Integer mMinBattery = null;

    @Option(name = "max-battery", description =
        "only run this test on a device whose battery level is strictly less than the given " +
        "amount. Scale: 0-100")
    private Integer mMaxBattery = null;

    @Option(
        name = "max-battery-temperature",
        description =
                "only run this test on a device whose battery temperature is strictly "
                        + "less than the given amount. Scale: Degrees celsius"
    )
    private Integer mMaxBatteryTemperature = null;

    @Option(
        name = "require-battery-check",
        description =
                "_If_ --min-battery and/or "
                        + "--max-battery is specified, enforce the check. If "
                        + "require-battery-check=false, then no battery check will occur."
    )
    private boolean mRequireBatteryCheck = true;

    @Option(
        name = "require-battery-temp-check",
        description =
                "_If_ --max-battery-temperature is specified, enforce the battery checking. If "
                        + "require-battery-temp-check=false, then no temperature check will occur."
    )
    private boolean mRequireBatteryTemperatureCheck = true;

    @Option(name = "min-sdk-level", description = "Only run this test on devices that support " +
            "this Android SDK/API level")
    private Integer mMinSdk = null;

    @Option(name = "max-sdk-level", description = "Only run this test on devices that are running " +
        "this or lower Android SDK/API level")
    private Integer mMaxSdk = null;

    // If we have tried to fetch the environment variable ANDROID_SERIAL before.
    private boolean mFetchedEnvVariable = false;

    private static final String VARIANT_SEPARATOR = ":";

    /**
     * Add a serial number to the device selection options.
     *
     * @param serialNumber
     */
    public void addSerial(String serialNumber) {
        mSerials.add(serialNumber);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSerial(String... serialNumber) {
        mSerials.clear();
        mSerials.addAll(Arrays.asList(serialNumber));
    }

    /**
     * Add a serial number to exclusion list.
     *
     * @param serialNumber
     */
    public void addExcludeSerial(String serialNumber) {
        mExcludeSerials.add(serialNumber);
    }

    /**
     * Add a product type to the device selection options.
     *
     * @param productType
     */
    public void addProductType(String productType) {
        mProductTypes.add(productType);
    }

    /**
     * Add a property criteria to the device selection options
     */
    public void addProperty(String propertyKey, String propValue) {
        mPropertyMap.put(propertyKey, propValue);
    }

    /** {@inheritDoc} */
    @Override
    public Collection<String> getSerials(IDevice device) {
        // If no serial was explicitly set, use the environment variable ANDROID_SERIAL.
        if (mSerials.isEmpty() && !mFetchedEnvVariable) {
            String env_serial = fetchEnvironmentVariable("ANDROID_SERIAL");
            if (env_serial != null && !(device instanceof StubDevice)) {
                mSerials.add(env_serial);
            }
            mFetchedEnvVariable = true;
        }
        return copyCollection(mSerials);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<String> getExcludeSerials() {
        return copyCollection(mExcludeSerials);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<String> getProductTypes() {
        return copyCollection(mProductTypes);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean deviceRequested() {
        return mDeviceRequested;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean emulatorRequested() {
        return mEmulatorRequested;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean stubEmulatorRequested() {
        return mStubEmulatorRequested;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean nullDeviceRequested() {
        return mNullDeviceRequested;
    }

    public boolean tcpDeviceRequested() {
        return mTcpDeviceRequested;
    }

    /**
     * Sets the emulator requested flag
     */
    public void setEmulatorRequested(boolean emulatorRequested) {
        mEmulatorRequested = emulatorRequested;
    }

    /**
     * Sets the stub emulator requested flag
     */
    public void setStubEmulatorRequested(boolean stubEmulatorRequested) {
        mStubEmulatorRequested = stubEmulatorRequested;
    }

    /**
     * Sets the emulator requested flag
     */
    public void setDeviceRequested(boolean deviceRequested) {
        mDeviceRequested = deviceRequested;
    }

    /**
     * Sets the null device requested flag
     */
    public void setNullDeviceRequested(boolean nullDeviceRequested) {
        mNullDeviceRequested = nullDeviceRequested;
    }

    /**
     * Sets the tcp device requested flag
     */
    public void setTcpDeviceRequested(boolean tcpDeviceRequested) {
        mTcpDeviceRequested = tcpDeviceRequested;
    }

    /**
     * Sets the minimum battery level
     */
    public void setMinBatteryLevel(Integer minBattery) {
        mMinBattery = minBattery;
    }

    /**
     * Gets the requested minimum battery level
     */
    public Integer getMinBatteryLevel() {
        return mMinBattery;
    }

    /**
     * Sets the maximum battery level
     */
    public void setMaxBatteryLevel(Integer maxBattery) {
        mMaxBattery = maxBattery;
    }

    /**
     * Gets the requested maximum battery level
     */
    public Integer getMaxBatteryLevel() {
        return mMaxBattery;
    }

    /** Sets the maximum battery level */
    public void setMaxBatteryTemperature(Integer maxBatteryTemperature) {
        mMaxBatteryTemperature = maxBatteryTemperature;
    }

    /** Gets the requested maximum battery level */
    public Integer getMaxBatteryTemperature() {
        return mMaxBatteryTemperature;
    }

    /**
     * Sets whether battery check is required for devices with unknown battery level
     */
    public void setRequireBatteryCheck(boolean requireCheck) {
        mRequireBatteryCheck = requireCheck;
    }

    /**
     * Gets whether battery check is required for devices with unknown battery level
     */
    public boolean getRequireBatteryCheck() {
        return mRequireBatteryCheck;
    }

    /** Sets whether battery temp check is required for devices with unknown battery temperature */
    public void setRequireBatteryTemperatureCheck(boolean requireCheckTemprature) {
        mRequireBatteryTemperatureCheck = requireCheckTemprature;
    }

    /** Gets whether battery temp check is required for devices with unknown battery temperature */
    public boolean getRequireBatteryTemperatureCheck() {
        return mRequireBatteryTemperatureCheck;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<String, String> getProperties() {
        return mPropertyMap;
    }

    private Collection<String> copyCollection(Collection<String> original) {
        Collection<String> listCopy = new ArrayList<String>(original.size());
        listCopy.addAll(original);
        return listCopy;
    }

    /**
     * Helper function used to fetch environment variable. It is essentially a wrapper around
     * {@link System#getenv(String)} This is done for unit testing purposes.
     *
     * @param name the environment variable to fetch.
     * @return a {@link String} value of the environment variable or null if not available.
     */
    String fetchEnvironmentVariable(String name) {
        return System.getenv(name);
    }

    /**
     * @return <code>true</code> if the given {@link IDevice} is a match for the provided options.
     * <code>false</code> otherwise
     */
    @Override
    public boolean matches(IDevice device) {
        Collection<String> serials = getSerials(device);
        Collection<String> excludeSerials = getExcludeSerials();
        Map<String, Collection<String>> productVariants = splitOnVariant(getProductTypes());
        Collection<String> productTypes = productVariants.keySet();
        Map<String, String> properties = getProperties();

        if (!serials.isEmpty() &&
                !serials.contains(device.getSerialNumber())) {
            return false;
        }
        if (excludeSerials.contains(device.getSerialNumber())) {
            return false;
        }
        if (!productTypes.isEmpty()) {
            String productType = getDeviceProductType(device);
            if (productTypes.contains(productType)) {
                // check variant
                String productVariant = getDeviceProductVariant(device);
                Collection<String> variants = productVariants.get(productType);
                if (variants != null && !variants.contains(productVariant)) {
                    return false;
                }
            } else {
                // no product type matches; bye-bye
                return false;
            }
        }
        for (Map.Entry<String, String> propEntry : properties.entrySet()) {
            if (!propEntry.getValue().equals(device.getProperty(propEntry.getKey()))) {
                return false;
            }
        }
        if ((emulatorRequested() || stubEmulatorRequested()) && !device.isEmulator()) {
            return false;
        }
        if (deviceRequested() && device.isEmulator()) {
            return false;
        }
        if (device.isEmulator() && (device instanceof StubDevice) && !stubEmulatorRequested()) {
            // only allocate the stub emulator if requested
            return false;
        }
        if (nullDeviceRequested() != (device instanceof NullDevice)) {
            return false;
        }
        if (tcpDeviceRequested() != (TcpDevice.class.equals(device.getClass()))) {
            // We only match an exact TcpDevice here, no child class.
            return false;
        }
        if ((mMinSdk != null) || (mMaxSdk != null)) {
            int deviceSdkLevel = getDeviceSdkLevel(device);
            if (deviceSdkLevel < 0) {
                return false;
            }
            if (mMinSdk != null && deviceSdkLevel < mMinSdk) {
                return false;
            }
            if (mMaxSdk != null && mMaxSdk < deviceSdkLevel) {
                return false;
            }
        }
        // If battery check is required and we have a min/max battery requested
        if (mRequireBatteryCheck) {
            if (((mMinBattery != null) || (mMaxBattery != null))
                    && (!(device instanceof StubDevice) || (device instanceof FastbootDevice))) {
                // Only check battery on physical device. (FastbootDevice placeholder is always for
                // a physical device
                if (device instanceof FastbootDevice) {
                    // Ready battery of fastboot device does not work and could lead to weird log.
                    return false;
                }
                Integer deviceBattery = getBatteryLevel(device);
                if (deviceBattery == null) {
                    // Couldn't determine battery level when that check is required; reject device
                    return false;
                }
                if (isLessAndNotNull(deviceBattery, mMinBattery)) {
                    // deviceBattery < mMinBattery
                    return false;
                }
                if (isLessEqAndNotNull(mMaxBattery, deviceBattery)) {
                    // mMaxBattery <= deviceBattery
                    return false;
                }
            }
        }
        // If temperature check is required and we have a max temperature requested.
        if (mRequireBatteryTemperatureCheck) {
            if (mMaxBatteryTemperature != null
                    && (!(device instanceof StubDevice) || (device instanceof FastbootDevice))) {
                // Only check battery temp on physical device. (FastbootDevice placeholder is
                // always for a physical device

                if (device instanceof FastbootDevice) {
                    // Cannot get battery temperature
                    return false;
                }

                // Extract the temperature from the file
                IBatteryTemperature temp = new BatteryTemperature();
                Integer deviceBatteryTemp = temp.getBatteryTemperature(device);

                if (deviceBatteryTemp <= 0) {
                    // Couldn't determine battery temp when that check is required; reject device
                    return false;
                }

                if (isLessEqAndNotNull(mMaxBatteryTemperature, deviceBatteryTemp)) {
                    // mMaxBatteryTemperature <= deviceBatteryTemp
                    return false;
                }
            }
        }

        return extraMatching(device);
    }

    /** Extra validation step that maybe overridden if it does not make sense. */
    protected boolean extraMatching(IDevice device) {
        // Any device that extends TcpDevice and is not a TcpDevice will be rejected.
        if (device instanceof TcpDevice && !device.getClass().isAssignableFrom(TcpDevice.class)) {
            return false;
        }
        return true;
    }

    /** Determine if x is less-than y, given that both are non-Null */
    private static boolean isLessAndNotNull(Integer x, Integer y) {
        if ((x == null) || (y == null)) {
            return false;
        }
        return x < y;
    }

    /** Determine if x is less-than y, given that both are non-Null */
    private static boolean isLessEqAndNotNull(Integer x, Integer y) {
        if ((x == null) || (y == null)) {
            return false;
        }
        return x <= y;
    }

    private Map<String, Collection<String>> splitOnVariant(Collection<String> products) {
        // FIXME: we should validate all provided device selection options once, on the first
        // FIXME: call to #matches
        Map<String, Collection<String>> splitProducts =
                new HashMap<String, Collection<String>>(products.size());
        // FIXME: cache this
        for (String prod : products) {
            String[] parts = prod.split(VARIANT_SEPARATOR);
            if (parts.length == 1) {
                splitProducts.put(parts[0], null);
            } else if (parts.length == 2) {
                // A variant was specified as product:variant
                Collection<String> variants = splitProducts.get(parts[0]);
                if (variants == null) {
                    variants = new HashSet<String>();
                    splitProducts.put(parts[0], variants);
                }
                variants.add(parts[1]);
            } else {
                throw new IllegalArgumentException(String.format("The product type filter \"%s\" " +
                        "is invalid.  It must contain 0 or 1 '%s' characters, not %d.",
                        prod, VARIANT_SEPARATOR, parts.length));
            }
        }

        return splitProducts;
    }

    @Override
    public String getDeviceProductType(IDevice device) {
        String prop = getProperty(device, DeviceProperties.BOARD);
        if (prop != null) {
            prop = prop.toLowerCase();
        }
        return prop;
    }

    private String getProperty(IDevice device, String propName) {
        return device.getProperty(propName);
    }

    @Override
    public String getDeviceProductVariant(IDevice device) {
        String prop = getProperty(device, DeviceProperties.VARIANT);
        if (prop == null) {
            prop = getProperty(device, DeviceProperties.VARIANT_LEGACY);
        }
        if (prop != null) {
            prop = prop.toLowerCase();
        }
        return prop;
    }

    @Override
    public Integer getBatteryLevel(IDevice device) {
        try {
            // use default 5 minutes freshness
            Future<Integer> batteryFuture = device.getBattery();
            // get cached value or wait up to 500ms for battery level query
            return batteryFuture.get(500, TimeUnit.MILLISECONDS);
        } catch (InterruptedException | ExecutionException |
                java.util.concurrent.TimeoutException e) {
            CLog.w("Failed to query battery level for %s: %s", device.getSerialNumber(),
                    e.toString());
        }
        return null;
    }

    /**
     * Get the device's supported API level or -1 if it cannot be retrieved
     * @param device
     * @return the device's supported API level.
     */
    private int getDeviceSdkLevel(IDevice device) {
        int apiLevel = -1;
        String prop = getProperty(device, DeviceProperties.SDK_VERSION);
        try {
            apiLevel = Integer.parseInt(prop);
        } catch (NumberFormatException nfe) {
            CLog.w("Failed to parse sdk level %s for device %s", prop, device.getSerialNumber());
        }
        return apiLevel;
    }

    /**
     * Helper factory method to create a {@link IDeviceSelection} that will only match device
     * with given serial
     */
    public static IDeviceSelection createForSerial(String serial) {
        DeviceSelectionOptions o = new DeviceSelectionOptions();
        o.setSerial(serial);
        return o;
    }
}
