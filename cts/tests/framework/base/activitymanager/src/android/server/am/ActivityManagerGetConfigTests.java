/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.server.am;

import android.app.ActivityManager;
import android.app.KeyguardManager;
import android.content.Context;
import android.content.nano.DeviceConfigurationProto;
import android.content.nano.GlobalConfigurationProto;
import android.content.nano.LocaleProto;
import android.content.nano.ResourcesConfigurationProto;
import android.content.pm.ConfigurationInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.PackageManager;
import android.content.pm.SharedLibraryInfo;
import android.content.res.Configuration;
import android.graphics.Point;
import android.hardware.display.DisplayManager;
import android.opengl.GLES10;
import android.os.Build;
import android.os.LocaleList;
import android.os.ParcelFileDescriptor;
import android.support.test.InstrumentationRegistry;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.Display;

import com.google.protobuf.nano.InvalidProtocolBufferNanoException;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class ActivityManagerGetConfigTests {
    Context mContext;
    ActivityManager mAm;
    PackageManager mPm;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mAm = mContext.getSystemService(ActivityManager.class);
        mPm = mContext.getPackageManager();
    }

    private byte[] executeShellCommand(String cmd) {
        try {
            ParcelFileDescriptor pfd =
                    InstrumentationRegistry.getInstrumentation().getUiAutomation()
                            .executeShellCommand(cmd);
            byte[] buf = new byte[512];
            int bytesRead;
            FileInputStream fis = new ParcelFileDescriptor.AutoCloseInputStream(pfd);
            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            while ((bytesRead = fis.read(buf)) != -1) {
                stdout.write(buf, 0, bytesRead);
            }
            fis.close();
            return stdout.toByteArray();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Adds all supported GL extensions for a provided EGLConfig to a set by creating an EGLContext
     * and EGLSurface and querying extensions.
     *
     * @param egl An EGL API object
     * @param display An EGLDisplay to create a context and surface with
     * @param config The EGLConfig to get the extensions for
     * @param surfaceSize eglCreatePbufferSurface generic parameters
     * @param contextAttribs eglCreateContext generic parameters
     * @param glExtensions A Set<String> to add GL extensions to
     */
    private static void addExtensionsForConfig(
            EGL10 egl,
            EGLDisplay display,
            EGLConfig config,
            int[] surfaceSize,
            int[] contextAttribs,
            Set<String> glExtensions) {
        // Create a context.
        EGLContext context =
                egl.eglCreateContext(display, config, EGL10.EGL_NO_CONTEXT, contextAttribs);
        // No-op if we can't create a context.
        if (context == EGL10.EGL_NO_CONTEXT) {
            return;
        }

        // Create a surface.
        EGLSurface surface = egl.eglCreatePbufferSurface(display, config, surfaceSize);
        if (surface == EGL10.EGL_NO_SURFACE) {
            egl.eglDestroyContext(display, context);
            return;
        }

        // Update the current surface and context.
        egl.eglMakeCurrent(display, surface, surface, context);

        // Get the list of extensions.
        String extensionList = GLES10.glGetString(GLES10.GL_EXTENSIONS);
        if (!TextUtils.isEmpty(extensionList)) {
            // The list of extensions comes from the driver separated by spaces.
            // Split them apart and add them into a Set for deduping purposes.
            for (String extension : extensionList.split(" ")) {
                glExtensions.add(extension);
            }
        }

        // Tear down the context and surface for this config.
        egl.eglMakeCurrent(display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
        egl.eglDestroySurface(display, surface);
        egl.eglDestroyContext(display, context);
    }


    Set<String> getGlExtensionsFromDriver() {
        Set<String> glExtensions = new HashSet<>();

        // Get the EGL implementation.
        EGL10 egl = (EGL10) EGLContext.getEGL();
        if (egl == null) {
            throw new RuntimeException("Warning: couldn't get EGL");
        }

        // Get the default display and initialize it.
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        int[] version = new int[2];
        egl.eglInitialize(display, version);

        // Call getConfigs() in order to find out how many there are.
        int[] numConfigs = new int[1];
        if (!egl.eglGetConfigs(display, null, 0, numConfigs)) {
            throw new RuntimeException("Warning: couldn't get EGL config count");
        }

        // Allocate space for all configs and ask again.
        EGLConfig[] configs = new EGLConfig[numConfigs[0]];
        if (!egl.eglGetConfigs(display, configs, numConfigs[0], numConfigs)) {
            throw new RuntimeException("Warning: couldn't get EGL configs");
        }

        // Allocate surface size parameters outside of the main loop to cut down
        // on GC thrashing.  1x1 is enough since we are only using it to get at
        // the list of extensions.
        int[] surfaceSize =
                new int[] {
                        EGL10.EGL_WIDTH, 1,
                        EGL10.EGL_HEIGHT, 1,
                        EGL10.EGL_NONE
                };

        // For when we need to create a GLES2.0 context.
        final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        int[] gles2 = new int[] {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE};

        // For getting return values from eglGetConfigAttrib
        int[] attrib = new int[1];

        for (int i = 0; i < numConfigs[0]; i++) {
            // Get caveat for this config in order to skip slow (i.e. software) configs.
            egl.eglGetConfigAttrib(display, configs[i], EGL10.EGL_CONFIG_CAVEAT, attrib);
            if (attrib[0] == EGL10.EGL_SLOW_CONFIG) {
                continue;
            }

            // If the config does not support pbuffers we cannot do an eglMakeCurrent
            // on it in addExtensionsForConfig(), so skip it here. Attempting to make
            // it current with a pbuffer will result in an EGL_BAD_MATCH error
            egl.eglGetConfigAttrib(display, configs[i], EGL10.EGL_SURFACE_TYPE, attrib);
            if ((attrib[0] & EGL10.EGL_PBUFFER_BIT) == 0) {
                continue;
            }

            final int EGL_OPENGL_ES_BIT = 0x0001;
            final int EGL_OPENGL_ES2_BIT = 0x0004;
            egl.eglGetConfigAttrib(display, configs[i], EGL10.EGL_RENDERABLE_TYPE, attrib);
            if ((attrib[0] & EGL_OPENGL_ES_BIT) != 0) {
                addExtensionsForConfig(egl, display, configs[i], surfaceSize, null, glExtensions);
            }
            if ((attrib[0] & EGL_OPENGL_ES2_BIT) != 0) {
                addExtensionsForConfig(egl, display, configs[i], surfaceSize, gles2, glExtensions);
            }
        }

        // Release all EGL resources.
        egl.eglTerminate(display);

        return glExtensions;
    }

    private void checkResourceConfig(Configuration config, DisplayMetrics metrics,
            ResourcesConfigurationProto resConfig) {
        final int width, height;
        if (metrics.widthPixels >= metrics.heightPixels) {
            width = metrics.widthPixels;
            height = metrics.heightPixels;
        } else {
            //noinspection SuspiciousNameCombination
            width = metrics.heightPixels;
            //noinspection SuspiciousNameCombination
            height = metrics.widthPixels;
        }

        assertEquals("Expected SDK version does not match",
                Build.VERSION.RESOURCES_SDK_INT, resConfig.sdkVersion);
        assertEquals("Expected screen width px does not match",
                width, resConfig.screenWidthPx);
        assertEquals("Expected screen width px does not match",
                height, resConfig.screenHeightPx);

        assertEquals("Expected font scale does not match",
                config.fontScale, resConfig.configuration.fontScale, Float.MIN_VALUE*5);
        assertEquals("Expected mcc does not match",
                config.mcc, resConfig.configuration.mcc);
        assertEquals("Expected mnc does not match",
                config.mnc, resConfig.configuration.mnc);
        LocaleList llist = config.getLocales();
        LocaleProto[] lprotos = resConfig.configuration.locales;
        assertEquals("Expected number of locales does not match",
                llist.size(), lprotos.length);
        for (int i = 0; i < llist.size(); i++) {
            assertEquals("Expected locale #" + i + " language does not match",
                    llist.get(i).getLanguage(), lprotos[i].language);
            assertEquals("Expected locale #" + i + " country does not match",
                    llist.get(i).getCountry(), lprotos[i].country);
            assertEquals("Expected locale #" + i + " variant does not match",
                    llist.get(i).getVariant(), lprotos[i].variant);
        }
        assertEquals("Expected screen layout does not match",
                config.screenLayout, resConfig.configuration.screenLayout);
        assertEquals("Expected color mode does not match",
                config.colorMode, resConfig.configuration.colorMode);
        assertEquals("Expected touchscreen does not match",
                config.touchscreen, resConfig.configuration.touchscreen);
        assertEquals("Expected keyboard does not match",
                config.keyboard, resConfig.configuration.keyboard);
        assertEquals("Expected keyboard hidden does not match",
                config.keyboardHidden, resConfig.configuration.keyboardHidden);
        assertEquals("Expected hard keyboard hidden does not match",
                config.hardKeyboardHidden, resConfig.configuration.hardKeyboardHidden);
        assertEquals("Expected navigation does not match",
                config.navigation, resConfig.configuration.navigation);
        assertEquals("Expected navigation hidden does not match",
                config.navigationHidden, resConfig.configuration.navigationHidden);
        assertEquals("Expected orientation does not match",
                config.orientation, resConfig.configuration.orientation);
        assertEquals("Expected UI mode does not match",
                config.uiMode, resConfig.configuration.uiMode);
        assertEquals("Expected screen width dp does not match",
                config.screenWidthDp, resConfig.configuration.screenWidthDp);
        assertEquals("Expected screen hight dp does not match",
                config.screenHeightDp, resConfig.configuration.screenHeightDp);
        assertEquals("Expected smallest screen width dp does not match",
                config.smallestScreenWidthDp, resConfig.configuration.smallestScreenWidthDp);
        assertEquals("Expected density dpi does not match",
                config.densityDpi, resConfig.configuration.densityDpi);
        // XXX not comparing windowConfiguration, since by definition this is contextual.
    }

    private void checkDeviceConfig(DisplayManager dm, DeviceConfigurationProto deviceConfig) {
        Point stableSize = dm.getStableDisplaySize();
        assertEquals("Expected stable screen width does not match",
                stableSize.x, deviceConfig.stableScreenWidthPx);
        assertEquals("Expected stable screen height does not match",
                stableSize.y, deviceConfig.stableScreenHeightPx);
        assertEquals("Expected stable screen density does not match",
                DisplayMetrics.DENSITY_DEVICE_STABLE, deviceConfig.stableDensityDpi);

        assertEquals("Expected total RAM does not match",
                mAm.getTotalRam(), deviceConfig.totalRam);
        assertEquals("Expected low RAM does not match",
                mAm.isLowRamDevice(), deviceConfig.lowRam);
        assertEquals("Expected max cores does not match",
                Runtime.getRuntime().availableProcessors(), deviceConfig.maxCores);
        KeyguardManager kgm = mContext.getSystemService(KeyguardManager.class);
        assertEquals("Expected has secure screen lock does not match",
                kgm.isDeviceSecure(), deviceConfig.hasSecureScreenLock);

        ConfigurationInfo configInfo = mAm.getDeviceConfigurationInfo();
        if (configInfo.reqGlEsVersion != ConfigurationInfo.GL_ES_VERSION_UNDEFINED) {
            assertEquals("Expected opengl version does not match",
                    configInfo.reqGlEsVersion, deviceConfig.openglVersion);
        }

        Set<String> glExtensionsSet = getGlExtensionsFromDriver();
        String[] glExtensions = new String[glExtensionsSet.size()];
        glExtensions = glExtensionsSet.toArray(glExtensions);
        Arrays.sort(glExtensions);
        assertArrayEquals("Expected opengl extensions does not match",
                glExtensions, deviceConfig.openglExtensions);

        List<SharedLibraryInfo> slibs = mPm.getSharedLibraries(0);
        Collections.sort(slibs, Comparator.comparing(SharedLibraryInfo::getName));
        String[] slibNames = new String[slibs.size()];
        for (int i = 0; i < slibs.size(); i++) {
            slibNames[i] = slibs.get(i).getName();
        }
        assertArrayEquals("Expected shared libraries does not match",
                slibNames, deviceConfig.sharedLibraries);

        FeatureInfo[] features = mPm.getSystemAvailableFeatures();
        Arrays.sort(features, (o1, o2) ->
                (o1.name == o2.name ? 0 : (o1.name == null ? -1 : o1.name.compareTo(o2.name))));
        int size = 0;
        for (int i = 0; i < features.length; i++) {
            if (features[i].name != null) {
                size++;
            }
        }
        String[] featureNames = new String[size];
        for (int i = 0, j = 0; i < features.length; i++) {
            if (features[i].name != null) {
                featureNames[j] = features[i].name;
                j++;
            }
        }
        assertArrayEquals("Expected features does not match",
                featureNames, deviceConfig.features);
    }

    @Test
    public void testDeviceConfig() {
        byte[] dump = executeShellCommand("cmd activity get-config --proto --device");
        GlobalConfigurationProto globalConfig;
        try {
            globalConfig = GlobalConfigurationProto.parseFrom(dump);
        } catch (InvalidProtocolBufferNanoException ex) {
            throw new RuntimeException("Failed to parse get-config:\n"
                    + new String(dump, StandardCharsets.UTF_8), ex);
        }

        Configuration config = mContext.getResources().getConfiguration();
        DisplayManager dm = mContext.getSystemService(DisplayManager.class);
        Display display = dm.getDisplay(Display.DEFAULT_DISPLAY);
        DisplayMetrics metrics = new DisplayMetrics();
        display.getMetrics(metrics);

        checkResourceConfig(config, metrics, globalConfig.resources);
        checkDeviceConfig(dm, globalConfig.device);
    }
}
