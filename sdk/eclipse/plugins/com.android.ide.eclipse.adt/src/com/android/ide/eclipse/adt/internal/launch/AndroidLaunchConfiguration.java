/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.eclipse.adt.internal.launch;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.debug.core.ILaunchConfiguration;

/**
 * Launch configuration data. This stores the result of querying the
 * {@link ILaunchConfiguration} so that it's only done once.
 */
public class AndroidLaunchConfiguration {

    /**
     * Launch action. See {@link LaunchConfigDelegate#ACTION_DEFAULT},
     * {@link LaunchConfigDelegate#ACTION_ACTIVITY},
     * {@link LaunchConfigDelegate#ACTION_DO_NOTHING}
     */
    public int mLaunchAction = LaunchConfigDelegate.DEFAULT_LAUNCH_ACTION;

    /** Target selection mode for the configuration. */
    public enum TargetMode {
        /** Automatic target selection mode. */
        AUTO,
        /** Manual target selection mode. */
        MANUAL,
        /** All active devices */
        ALL_DEVICES,
        /** All active emulators */
        ALL_EMULATORS,
        /** All active devices and emulators */
        ALL_DEVICES_AND_EMULATORS;

        public static TargetMode getMode(String s) {
            for (TargetMode m: values()) {
                if (m.toString().equals(s)) {
                    return m;
                }
            }

            throw new IllegalArgumentException(String.format(
                    "Invalid representation (%s) for TargetMode", s));
        }

        public boolean isMultiDevice() {
            return this == ALL_DEVICES
                    || this == ALL_EMULATORS
                    || this == ALL_DEVICES_AND_EMULATORS;
        }
    }

    /**
     * Target selection mode.
     * @see TargetMode
     */
    public TargetMode mTargetMode = LaunchConfigDelegate.DEFAULT_TARGET_MODE;

    /**
     * Indicates whether the emulator should be called with -wipe-data
     */
    public boolean mWipeData = LaunchConfigDelegate.DEFAULT_WIPE_DATA;

    /**
     * Indicates whether the emulator should be called with -no-boot-anim
     */
    public boolean mNoBootAnim = LaunchConfigDelegate.DEFAULT_NO_BOOT_ANIM;

    /**
     * AVD Name.
     */
    public String mAvdName = null;

    public String mNetworkSpeed = EmulatorConfigTab.getSpeed(
            LaunchConfigDelegate.DEFAULT_SPEED);
    public String mNetworkDelay = EmulatorConfigTab.getDelay(
            LaunchConfigDelegate.DEFAULT_DELAY);

    /**
     * Optional custom command line parameter to launch the emulator
     */
    public String mEmulatorCommandLine;

    /** Flag indicating whether the same device should be used for future launches. */
    public boolean mReuseLastUsedDevice = false;

    /** Serial number of the device used in the last launch of this config. */
    public String mLastUsedDevice = null;

    /**
     * Initialized the structure from an ILaunchConfiguration object.
     * @param config
     */
    public void set(ILaunchConfiguration config) {
        try {
            mLaunchAction = config.getAttribute(LaunchConfigDelegate.ATTR_LAUNCH_ACTION,
                    mLaunchAction);
        } catch (CoreException e1) {
            // nothing to be done here, we'll use the default value
        }

        mTargetMode = parseTargetMode(config, mTargetMode);

        try {
            mReuseLastUsedDevice = config.getAttribute(
                    LaunchConfigDelegate.ATTR_REUSE_LAST_USED_DEVICE, false);
            mLastUsedDevice = config.getAttribute(
                    LaunchConfigDelegate.ATTR_LAST_USED_DEVICE, (String)null);
        } catch (CoreException e) {
         // nothing to be done here, we'll use the default value
        }

        try {
            mAvdName = config.getAttribute(LaunchConfigDelegate.ATTR_AVD_NAME, mAvdName);
        } catch (CoreException e) {
            // ignore
        }

        int index = LaunchConfigDelegate.DEFAULT_SPEED;
        try {
            index = config.getAttribute(LaunchConfigDelegate.ATTR_SPEED, index);
        } catch (CoreException e) {
            // nothing to be done here, we'll use the default value
        }
        mNetworkSpeed = EmulatorConfigTab.getSpeed(index);

        index = LaunchConfigDelegate.DEFAULT_DELAY;
        try {
            index = config.getAttribute(LaunchConfigDelegate.ATTR_DELAY, index);
        } catch (CoreException e) {
            // nothing to be done here, we'll use the default value
        }
        mNetworkDelay = EmulatorConfigTab.getDelay(index);

        try {
            mEmulatorCommandLine = config.getAttribute(
                    LaunchConfigDelegate.ATTR_COMMANDLINE, ""); //$NON-NLS-1$
        } catch (CoreException e) {
            // lets not do anything here, we'll use the default value
        }

        try {
            mWipeData = config.getAttribute(LaunchConfigDelegate.ATTR_WIPE_DATA, mWipeData);
        } catch (CoreException e) {
            // nothing to be done here, we'll use the default value
        }

        try {
            mNoBootAnim = config.getAttribute(LaunchConfigDelegate.ATTR_NO_BOOT_ANIM,
                                              mNoBootAnim);
        } catch (CoreException e) {
            // nothing to be done here, we'll use the default value
        }
    }

    /**
     * Retrieve the {@link TargetMode} saved in the provided launch configuration.
     * Returns defaultMode if there are any errors while retrieving or parsing the saved setting.
     */
    public static TargetMode parseTargetMode(ILaunchConfiguration config, TargetMode defaultMode) {
        try {
            String value = config.getAttribute(LaunchConfigDelegate.ATTR_TARGET_MODE,
                    defaultMode.toString());
            return TargetMode.getMode(value);
        } catch (CoreException e) {
            // ADT R20 changes the attribute type of ATTR_TARGET_MODE to be a string from a bool.
            // So if parsing as a string fails, attempt parsing as a boolean.
            try {
                boolean value = config.getAttribute(LaunchConfigDelegate.ATTR_TARGET_MODE, true);
                return value ? TargetMode.AUTO : TargetMode.MANUAL;
            } catch (CoreException e1) {
                return defaultMode;
            }
        } catch (IllegalArgumentException e) {
            return defaultMode;
        }
    }
}

