/*
 * Copyright (C) 2017 The Android Open Source Project
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
package android.gputools.cts;

import android.platform.test.annotations.Presubmit;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.IDeviceTest;

import com.android.ddmlib.Log;

import java.util.Scanner;

import org.junit.After;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests that exercise Rootless GPU Debug functionality supported by the loader.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class CtsRootlessGpuDebugHostTest implements IDeviceTest {

    public static final String TAG = CtsRootlessGpuDebugHostTest.class.getSimpleName();

    /**
     * A reference to the device under test.
     */
    private ITestDevice mDevice;

    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    // This test ensures that the Vulkan loader can use Settings to load layer
    // from the base directory of debuggable applications.  Is also tests several
    // positive and negative scenarios we want to cover (listed below).
    //
    // There are three APKs; DEBUG and RELEASE are practically identical with one
    // being flagged as debuggable.  The LAYERS APK is mainly a conduit for getting
    // layers onto the device without affecting the other APKs.
    //
    // The RELEASE APK does contain one layer to ensure using Settings to enable
    // layers does not interfere with legacy methods using system properties.
    //
    // The layers themselves are practically null, only enough functionality to
    // satisfy loader enumerating and loading.  They don't actually chain together.
    //
    // Positive tests
    // - Ensure we can toggle the Enable Setting on and off (testDebugLayerLoadVulkan)
    // - Ensure we can set the debuggable app (testDebugLayerLoadVulkan)
    // - Ensure we can set the layer list (testDebugLayerLoadVulkan)
    // - Ensure we can push a layer to debuggable app (testDebugLayerLoadVulkan)
    // - Ensure we can specify the app to load layers (testDebugLayerLoadVulkan)
    // - Ensure we can load a layer from app's data directory (testDebugLayerLoadVulkan)
    // - Ensure we can load multiple layers, in order, from app's data directory (testDebugLayerLoadVulkan)
    // - Ensure we can still use system properties if no layers loaded via Settings (testSystemPropertyEnableVulkan)
    // Negative tests
    // - Ensure we cannot push a layer to non-debuggable app (testReleaseLayerLoad)
    // - Ensure non-debuggable app ignores the new Settings (testReleaseLayerLoad)
    // - Ensure we cannot enumerate layers from debuggable app's data directory if Setting not specified (testDebugNoEnumerateVulkan)
    // - Ensure we cannot enumerate layers without specifying the debuggable app (testDebugNoEnumerateVulkan)
    // - Ensure we cannot use system properties when layer is found via Settings with debuggable app (testSystemPropertyIgnoreVulkan)

    private static final String CLASS = "RootlessGpuDebugDeviceActivity";
    private static final String ACTIVITY = "android.rootlessgpudebug.app.RootlessGpuDebugDeviceActivity";
    private static final String LAYER_A = "nullLayerA";
    private static final String LAYER_B = "nullLayerB";
    private static final String LAYER_C = "nullLayerC";
    private static final String LAYER_A_LIB = "libVkLayer_" + LAYER_A + ".so";
    private static final String LAYER_B_LIB = "libVkLayer_" + LAYER_B + ".so";
    private static final String LAYER_C_LIB = "libVkLayer_" + LAYER_C + ".so";
    private static final String LAYER_A_NAME = "VK_LAYER_ANDROID_" + LAYER_A;
    private static final String LAYER_B_NAME = "VK_LAYER_ANDROID_" + LAYER_B;
    private static final String LAYER_C_NAME = "VK_LAYER_ANDROID_" + LAYER_C;
    private static final String DEBUG_APP = "android.rootlessgpudebug.DEBUG.app";
    private static final String RELEASE_APP = "android.rootlessgpudebug.RELEASE.app";
    private static final String LAYERS_APP = "android.rootlessgpudebug.LAYERS.app";

    // This is how long we'll scan the log for a result before giving up. This limit will only
    // be reached if something has gone wrong
    private static final long LOG_SEARCH_TIMEOUT_MS = 5000;

    private String removeWhitespace(String input) {
        return input.replaceAll(System.getProperty("line.separator"), "").trim();
    }

    /**
     * Grab and format the process ID of requested app.
     */
    private String getPID(String app) throws Exception {
        String pid = mDevice.executeAdbCommand("shell", "pidof", app);
        return removeWhitespace(pid);
    }

    /**
     * Extract the requested layer from APK and copy to tmp
     */
    private void setupLayer(String layer) throws Exception {

        // We use the LAYERS apk to facilitate getting layers onto the device for mixing and matching
        String libPath = mDevice.executeAdbCommand("shell", "pm", "path", LAYERS_APP);
        libPath = libPath.replaceAll("package:", "");
        libPath = libPath.replaceAll("base.apk", "");
        libPath = removeWhitespace(libPath);
        libPath += "lib/";

        // Use find to get the .so so we can ignore ABI
        String layerPath = mDevice.executeAdbCommand("shell", "find", libPath + " -name " + layer);
        layerPath = removeWhitespace(layerPath);
        mDevice.executeAdbCommand("shell", "cp", layerPath + " /data/local/tmp");
    }

    /**
     * Simple helper class for returning multiple results
     */
    public class LogScanResult {
        public boolean found;
        public int lineNumber;
    }

    private LogScanResult scanLog(String pid, String searchString) throws Exception {
        return scanLog(pid, searchString, "");
    }

    /**
     * Scan the logcat for requested process ID, returning if found and which line
     */
    private LogScanResult scanLog(String pid, String searchString, String endString) throws Exception {

        LogScanResult result = new LogScanResult();
        result.found = false;
        result.lineNumber = -1;

        // Scan until output from app is found
        boolean scanComplete= false;

        // Let the test run a reasonable amount of time before moving on
        long hostStartTime = System.currentTimeMillis();

        while (!scanComplete && ((System.currentTimeMillis() - hostStartTime) < LOG_SEARCH_TIMEOUT_MS)) {

            // Give our activity a chance to run and fill the log
            Thread.sleep(1000);

            // Pull the logcat for a single process
            String logcat = mDevice.executeAdbCommand("logcat", "-v", "brief", "-d", "--pid=" + pid, "*:V");
            int lineNumber = 0;
            Scanner apkIn = new Scanner(logcat);
            while (apkIn.hasNextLine()) {
                lineNumber++;
                String line = apkIn.nextLine();
                if (line.contains(searchString) && line.endsWith(endString)) {
                    result.found = true;
                    result.lineNumber = lineNumber;
                }
                if (line.contains("vkCreateInstance succeeded")) {
                    // Once we've got output from the app, we've collected what we need
                    scanComplete= true;
                }
            }
            apkIn.close();
        }

        return result;
    }

    /**
     * Remove any temporary files on the device, clear any settings, kill the apps after each test
     */
    @After
    public void cleanup() throws Exception {
        mDevice.executeAdbCommand("shell", "am", "force-stop", DEBUG_APP);
        mDevice.executeAdbCommand("shell", "am", "force-stop", RELEASE_APP);
        mDevice.executeAdbCommand("shell", "rm", "-f", "/data/local/tmp/" + LAYER_A_LIB);
        mDevice.executeAdbCommand("shell", "rm", "-f", "/data/local/tmp/" + LAYER_B_LIB);
        mDevice.executeAdbCommand("shell", "rm", "-f", "/data/local/tmp/" + LAYER_C_LIB);
        mDevice.executeAdbCommand("shell", "settings", "delete", "global", "enable_gpu_debug_layers");
        mDevice.executeAdbCommand("shell", "settings", "delete", "global", "gpu_debug_app");
        mDevice.executeAdbCommand("shell", "settings", "delete", "global", "gpu_debug_layers");
        mDevice.executeAdbCommand("shell", "setprop", "debug.vulkan.layers", "\'\"\"\'");
    }

    /**
     * This is the primary test of the feature. It pushes layers to our debuggable app and ensures they are
     * loaded in the correct order.
     */
    @Test
    public void testDebugLayerLoadVulkan() throws Exception {

        // Set up layers to be loaded
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", DEBUG_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME + ":" + LAYER_B_NAME);

        // Copy the layers from our LAYERS APK to tmp
        setupLayer(LAYER_A_LIB);
        setupLayer(LAYER_B_LIB);

        // Copy them over to our DEBUG app
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", DEBUG_APP,
                                  "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'");
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_B_LIB, "|", "run-as", DEBUG_APP,
                                  "sh", "-c", "\'cat", ">", LAYER_B_LIB, ";", "chmod", "700", LAYER_B_LIB + "\'");

        // Kick off our DEBUG app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", DEBUG_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(DEBUG_APP);

        // Check that both layers were loaded, in the correct order
        String searchStringA = "nullCreateInstance called in " + LAYER_A;
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertTrue("LayerA was not loaded", resultA.found);

        String searchStringB = "nullCreateInstance called in " + LAYER_B;
        LogScanResult resultB = scanLog(pid, searchStringB);
        Assert.assertTrue("LayerB was not loaded", resultB.found);

        Assert.assertTrue("LayerA should be loaded before LayerB", resultA.lineNumber < resultB.lineNumber);
    }

    /**
     * This test ensures that we cannot push a layer to a non-debuggable app
     * It also ensures non-debuggable apps ignore Settings and don't enumerate layers in the base directory.
     */
    @Test
    public void testReleaseLayerLoadVulkan() throws Exception {

        // Set up a layers to be loaded for RELEASE app
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", RELEASE_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME + ":" + LAYER_B_NAME);

        // Copy a layer from our LAYERS APK to tmp
        setupLayer(LAYER_A_LIB);

        // Attempt to copy them over to our RELEASE app (this should fail)
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", RELEASE_APP,
                                   "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'", "||", "echo", "run-as", "failed");

        // Kick off our RELEASE app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", RELEASE_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(RELEASE_APP);

        // Ensure we don't load the layer in base dir
        String searchStringA = LAYER_A_NAME + "loaded";
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertFalse("LayerA was enumerated", resultA.found);
    }

    /**
     * This test ensures debuggable apps do not enumerate layers in base
     * directory if enable_gpu_debug_layers is not enabled.
     */
    @Test
    public void testDebugNotEnabledVulkan() throws Exception {

        // Ensure the global layer enable settings is NOT enabled
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "0");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", DEBUG_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME);

        // Copy a layer from our LAYERS APK to tmp
        setupLayer(LAYER_A_LIB);

        // Copy it over to our DEBUG app
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", DEBUG_APP,
                                  "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'");

        // Kick off our DEBUG app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", DEBUG_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(DEBUG_APP);

        // Ensure we don't load the layer in base dir
        String searchStringA = LAYER_A_NAME + "loaded";
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertFalse("LayerA was enumerated", resultA.found);
    }

    /**
     * This test ensures debuggable apps do not enumerate layers in base
     * directory if gpu_debug_app does not match.
     */
    @Test
    public void testDebugWrongAppVulkan() throws Exception {

        // Ensure the gpu_debug_app does not match what we launch
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", RELEASE_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME);

        // Copy a layer from our LAYERS APK to tmp
        setupLayer(LAYER_A_LIB);

        // Copy it over to our DEBUG app
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", DEBUG_APP,
                                  "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'");

        // Kick off our DEBUG app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", DEBUG_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(DEBUG_APP);

        // Ensure we don't load the layer in base dir
        String searchStringA = LAYER_A_NAME + "loaded";
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertFalse("LayerA was enumerated", resultA.found);
    }

    /**
     * This test ensures debuggable apps do not enumerate layers in base
     * directory if gpu_debug_layers are not set.
     */
    @Test
    public void testDebugNoLayersEnabledVulkan() throws Exception {

        // Ensure the global layer enable settings is NOT enabled
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", DEBUG_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", "foo");

        // Copy a layer from our LAYERS APK to tmp
        setupLayer(LAYER_A_LIB);

        // Copy it over to our DEBUG app
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", DEBUG_APP,
                                  "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'");

        // Kick off our DEBUG app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", DEBUG_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(DEBUG_APP);

        // Ensure layerA is not loaded
        String searchStringA = "nullCreateInstance called in " + LAYER_A;
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertFalse("LayerA was loaded", resultA.found);
    }

    /**
     * This test ensures we can still use properties if no layer found via Settings
     */
    @Test
    public void testSystemPropertyEnableVulkan() throws Exception {

        // Set up layerA to be loaded, but not layerB or layerC
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", RELEASE_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME);

        // Enable layerC (which is packaged with the RELEASE app) with system properties
        mDevice.executeAdbCommand("shell", "setprop", "debug.vulkan.layers " + LAYER_C_NAME);

        // Kick off our RELEASE app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", RELEASE_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(RELEASE_APP);

        // Check that both layers were loaded, in the correct order
        String searchStringA = LAYER_A_NAME + "loaded";
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertFalse("LayerA was enumerated", resultA.found);

        String searchStringC = "nullCreateInstance called in " + LAYER_C;
        LogScanResult resultC = scanLog(pid, searchStringC);
        Assert.assertTrue("LayerC was not loaded", resultC.found);
    }

    /**
     * This test ensures system properties are ignored if Settings load a layer
     */
    @Test
    public void testSystemPropertyIgnoreVulkan() throws Exception {

        // Set up layerA to be loaded, but not layerB
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "enable_gpu_debug_layers", "1");
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_app", DEBUG_APP);
        mDevice.executeAdbCommand("shell", "settings", "put", "global", "gpu_debug_layers", LAYER_A_NAME);

        // Copy the layers from our LAYERS APK
        setupLayer(LAYER_A_LIB);
        setupLayer(LAYER_B_LIB);

        // Copy them over to our DEBUG app
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_A_LIB, "|", "run-as", DEBUG_APP,
                                 "sh", "-c", "\'cat", ">", LAYER_A_LIB, ";", "chmod", "700", LAYER_A_LIB + "\'");
        mDevice.executeAdbCommand("shell", "cat", "/data/local/tmp/" + LAYER_B_LIB, "|", "run-as", DEBUG_APP,
                                 "sh", "-c", "\'cat", ">", LAYER_B_LIB, ";", "chmod", "700", LAYER_B_LIB + "\'");

        // Enable layerB with system properties
        mDevice.executeAdbCommand("shell", "setprop", "debug.vulkan.layers " + LAYER_B_NAME);

        // Kick off our DEBUG app
        mDevice.executeAdbCommand("shell", "am", "start", "-n", DEBUG_APP + "/" + ACTIVITY);

        // Give it a chance to start, then grab process ID
        Thread.sleep(1000);
        String pid = getPID(DEBUG_APP);

        // Ensure only layerA is loaded
        String searchStringA = "nullCreateInstance called in " + LAYER_A;
        LogScanResult resultA = scanLog(pid, searchStringA);
        Assert.assertTrue("LayerA was not loaded", resultA.found);

        String searchStringB = "nullCreateInstance called in " + LAYER_B;
        LogScanResult resultB = scanLog(pid, searchStringB);
        Assert.assertFalse("LayerB was loaded", resultB.found);
    }
}
