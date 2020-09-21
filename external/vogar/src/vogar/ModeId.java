/*
 * Copyright (C) 2009 The Android Open Source Project
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

package vogar;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public enum ModeId {
    /** (Target) dalvikvm */
    DEVICE,
    /** (Host) dalvikvm */
    HOST,
    /** (Host) java */
    JVM,
    /** (Target), execution as an Android app with Zygote */
    ACTIVITY,
    /** (Target) app_process */
    APP_PROCESS;

    // $BOOTCLASSPATH defined by system/core/rootdir/init.rc
    // - DEVICE_JARS are appended automatically.
    // (Intended for use with app_process and activities.)
    // See PRODUCT_BOOT_JARS in build/make/target/product/core_tiny.mk
    private static final String[] APP_JARS = new String[] {
            "legacy-test",
            "framework",
            "telephony-common",
            "voip-common",
            "ims-common",
            "org.apache.http.legacy.boot",
            "android.hidl.base-V1.0-java",
            "android.hidl.manager-V1.0-java"
            // TODO: get this list programatically
    };

    // $BOOTCLASSPATH for art+libcore only.
    // (Intended for use with dalvikvm only.)
    // See TARGET_CORE_JARS in android/build/make/core/envsetup.mk
    private static final String[] DEVICE_JARS = new String[] {
            "core-oj",
            "core-libart",
            "conscrypt",
            "okhttp",
            "bouncycastle",
            "apache-xml"
    };

    // $BOOTCLASSPATH for art+libcore only (host version).
    // - Must be same as DEVICE_JARS + "hostdex" suffix.
    // (Intended for use with dalvikvm only.)
    // See HOST_CORE_JARS in android/build/make/core/envsetup.mk
    private static final String[] HOST_JARS = new String[] {
            "core-oj-hostdex",
            "core-libart-hostdex",
            "conscrypt-hostdex",
            "okhttp-hostdex",
            "bouncycastle-hostdex",
            "apache-xml-hostdex"
    };

    public boolean acceptsVmArgs() {
        return this != ACTIVITY;
    }

    /**
     * Returns {@code true} if execution happens on the local machine. e.g. host-mode android or a
     * JVM.
     */
    public boolean isLocal() {
        return isHost() || this == ModeId.JVM;
    }

    /** Returns {@code true} if execution takes place with a host-mode Android runtime */
    public boolean isHost() {
        return this == HOST;
    }

    /** Returns {@code true} if execution takes place with a device-mode Android runtime */
    public boolean isDevice() {
        return this == ModeId.DEVICE || this == ModeId.APP_PROCESS;
    }

    public boolean requiresAndroidSdk() {
        return this != JVM;
    }

    public boolean supportsVariant(Variant variant) {
        return (variant == Variant.X32)
                || ((this == HOST || this == DEVICE) && (variant == Variant.X64));
    }

    public boolean supportsToolchain(Toolchain toolchain) {
        return (this == JVM && toolchain == Toolchain.JAVAC)
                || (this != JVM && toolchain != Toolchain.JAVAC);
    }

    /** The default command to use for the mode unless overridden by --vm-command */
    public String defaultVmCommand(Variant variant) {
        if (!supportsVariant(variant)) {
            throw new AssertionError("Unsupported variant: " + variant + " for " + this);
        }
        switch (this) {
            case DEVICE:
            case HOST:
                if (variant == Variant.X32) {
                    return "dalvikvm32";
                } else {
                    return "dalvikvm64";
                }

            case JVM:
                return "java";
            case APP_PROCESS:
                return "app_process";
            case ACTIVITY:
                return null;
            default:
                throw new IllegalArgumentException("Unknown mode: " + this);
        }
    }

    /**
     * Return the names of jars required to compile in this mode when android.jar is not being used.
     * Also used to generated the bootclasspath in HOST* and DEVICE* modes.
     */
    public String[] getJarNames() {
        List<String> jarNames = new ArrayList<String>();
        switch (this) {
            case ACTIVITY:
            case APP_PROCESS:
                // Order matters. Add device-jars before app-jars.
                jarNames.addAll(Arrays.asList(DEVICE_JARS));
                jarNames.addAll(Arrays.asList(APP_JARS));
                break;
            case DEVICE:
                jarNames.addAll(Arrays.asList(DEVICE_JARS));
                break;
            case HOST:
                jarNames.addAll(Arrays.asList(HOST_JARS));
                break;
            default:
                throw new IllegalArgumentException("Unsupported mode: " + this);
        }
        return jarNames.toArray(new String[jarNames.size()]);
    }

    /** Returns the default toolchain to use with the mode if not overriden. */
    public Toolchain defaultToolchain() {
        switch (this) {
            case JVM:
                return Toolchain.JAVAC;
            default:
                return Toolchain.D8;
        }
    }
}
