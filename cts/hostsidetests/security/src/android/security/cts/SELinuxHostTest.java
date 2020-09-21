/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.security.cts;

import android.platform.test.annotations.RestrictedBuildTest;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.util.PropertyUtil;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;

import com.android.compatibility.common.util.CddTest;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.String;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Scanner;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

/**
 * Host-side SELinux tests.
 *
 * These tests analyze the policy file in use on the subject device directly or
 * run as the shell user to evaluate aspects of the state of SELinux on the test
 * device which otherwise would not be available to a normal apk.
 */
public class SELinuxHostTest extends DeviceTestCase implements IBuildReceiver, IDeviceTest {

    private static final Map<ITestDevice, File> cachedDevicePolicyFiles = new HashMap<>(1);
    private static final Map<ITestDevice, File> cachedDevicePlatFcFiles = new HashMap<>(1);
    private static final Map<ITestDevice, File> cachedDeviceNonplatFcFiles = new HashMap<>(1);
    private static final Map<ITestDevice, File> cachedDeviceVendorManifest = new HashMap<>(1);
    private static final Map<ITestDevice, File> cachedDeviceSystemPolicy = new HashMap<>(1);

    private File sepolicyAnalyze;
    private File checkSeapp;
    private File checkFc;
    private File aospSeappFile;
    private File aospFcFile;
    private File aospPcFile;
    private File aospSvcFile;
    private File devicePolicyFile;
    private File devicePlatSeappFile;
    private File deviceNonplatSeappFile;
    private File devicePlatFcFile;
    private File deviceNonplatFcFile;
    private File devicePcFile;
    private File deviceSvcFile;
    private File seappNeverAllowFile;
    private File libsepolwrap;
    private File libcpp;
    private File sepolicyTests;

    private IBuildInfo mBuild;

    /**
     * A reference to the device under test.
     */
    private ITestDevice mDevice;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo build) {
        mBuild = build;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        super.setDevice(device);
        mDevice = device;
    }

    public static File copyResourceToTempFile(String resName) throws IOException {
        InputStream is = SELinuxHostTest.class.getResourceAsStream(resName);
        File tempFile = File.createTempFile("SELinuxHostTest", ".tmp");
        FileOutputStream os = new FileOutputStream(tempFile);
        byte[] buf = new byte[1024];
        int len;

        while ((len = is.read(buf)) != -1) {
            os.write(buf, 0, len);
        }
        os.flush();
        os.close();
        tempFile.deleteOnExit();
        return tempFile;
    }

    private static void appendTo(String dest, String src) throws IOException {
        try (FileInputStream is = new FileInputStream(new File(src));
             FileOutputStream os = new FileOutputStream(new File(dest))) {
            byte[] buf = new byte[1024];
            int len;

            while ((len = is.read(buf)) != -1) {
                os.write(buf, 0, len);
            }
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuild);
        sepolicyAnalyze = buildHelper.getTestFile("sepolicy-analyze");
        sepolicyAnalyze.setExecutable(true);

        devicePolicyFile = getDevicePolicyFile(mDevice);
        if (isSepolicySplit(mDevice)) {
            devicePlatFcFile = getDeviceFile(mDevice, cachedDevicePlatFcFiles,
                    "/system/etc/selinux/plat_file_contexts", "plat_file_contexts");
            if (mDevice.doesFileExist("/vendor/etc/selinux/nonplat_file_contexts")){
                // Old nonplat_* naming can be present if a framework-only OTA was done.
                deviceNonplatFcFile = getDeviceFile(mDevice, cachedDeviceNonplatFcFiles,
                        "/vendor/etc/selinux/nonplat_file_contexts", "nonplat_file_contexts");
            } else {
                deviceNonplatFcFile = getDeviceFile(mDevice, cachedDeviceNonplatFcFiles,
                        "/vendor/etc/selinux/vendor_file_contexts", "vendor_file_contexts");
            }
        } else {
            devicePlatFcFile = getDeviceFile(mDevice, cachedDevicePlatFcFiles,
                    "/plat_file_contexts", "plat_file_contexts");
            deviceNonplatFcFile = getDeviceFile(mDevice, cachedDeviceNonplatFcFiles,
                    "/vendor_file_contexts", "vendor_file_contexts");
        }
    }

    /*
     * IMPLEMENTATION DETAILS: We cache some host-side policy files on per-device basis (in case
     * CTS supports running against multiple devices at the same time). HashMap is used instead
     * of WeakHashMap because in the grand scheme of things, keeping ITestDevice and
     * corresponding File objects from being garbage-collected is not a big deal in CTS. If this
     * becomes a big deal, this can be switched to WeakHashMap.
     */
    private static File getDeviceFile(ITestDevice device,
            Map<ITestDevice, File> cache, String deviceFilePath,
            String tmpFileName) throws Exception {
        if (!device.doesFileExist(deviceFilePath)){
            throw new Exception();
        }
        File file;
        synchronized (cache) {
            file = cache.get(device);
        }
        if (file != null) {
            return file;
        }
        file = File.createTempFile(tmpFileName, ".tmp");
        file.deleteOnExit();
        device.pullFile(deviceFilePath, file);
        synchronized (cache) {
            cache.put(device, file);
        }
        return file;
    }

    private static File buildSystemPolicy(ITestDevice device, Map<ITestDevice, File> cache,
            String tmpFileName) throws Exception {
        File builtPolicyFile;
        synchronized (cache) {
            builtPolicyFile = cache.get(device);
        }
        if (builtPolicyFile != null) {
            return builtPolicyFile;
        }


        builtPolicyFile = File.createTempFile(tmpFileName, ".tmp");
        builtPolicyFile.deleteOnExit();

        File secilc = copyResourceToTempFile("/secilc");
        secilc.setExecutable(true);

        File systemSepolicyCilFile = File.createTempFile("plat_sepolicy", ".cil");
        systemSepolicyCilFile.deleteOnExit();

        assertTrue(device.pullFile("/system/etc/selinux/plat_sepolicy.cil", systemSepolicyCilFile));

        ProcessBuilder pb = new ProcessBuilder(
            secilc.getAbsolutePath(),
            "-m", "-M", "true", "-c", "30",
            "-o", builtPolicyFile.getAbsolutePath(),
            systemSepolicyCilFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue(errorString.toString(), errorString.length() == 0);

        synchronized (cache) {
            cache.put(device, builtPolicyFile);
        }
        return builtPolicyFile;
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns the host-side file containing the SELinux policy of the device under test.
     */
    public static File getDevicePolicyFile(ITestDevice device) throws Exception {
        return getDeviceFile(device, cachedDevicePolicyFiles, "/sys/fs/selinux/policy", "sepolicy");
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns the host-side file containing the system SELinux policy of the device under test.
     */
    public static File getDeviceSystemPolicyFile(ITestDevice device) throws Exception {
        return buildSystemPolicy(device, cachedDeviceSystemPolicy, "system_sepolicy");
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns the major number of sepolicy version of device's vendor implementation.
     * TODO(b/37999212): Use VINTF object API instead of parsing vendor manifest.
     */
    public static int getVendorSepolicyVersion(ITestDevice device) throws Exception {
        String deviceManifestPath =
                (device.doesFileExist("/vendor/etc/vintf/manifest.xml")) ?
                "/vendor/etc/vintf/manifest.xml" :
                "/vendor/manifest.xml";
        File vendorManifestFile = getDeviceFile(device, cachedDeviceVendorManifest,
                deviceManifestPath, "manifest.xml");

        DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
        DocumentBuilder db = dbf.newDocumentBuilder();
        Document doc = db.parse(vendorManifestFile);
        Element root = doc.getDocumentElement();
        Element sepolicy = (Element) root.getElementsByTagName("sepolicy").item(0);
        Element version = (Element) sepolicy.getElementsByTagName("version").item(0);
        String sepolicyVersion = version.getTextContent().split("\\.")[0];
        return Integer.parseInt(sepolicyVersion);
    }

    /**
     * Tests that the kernel is enforcing selinux policy globally.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testGlobalEnforcing() throws Exception {
        CollectingOutputReceiver out = new CollectingOutputReceiver();
        mDevice.executeShellCommand("cat /sys/fs/selinux/enforce", out);
        assertEquals("SELinux policy is not being enforced!", "1", out.getOutput());
    }

    /**
     * Tests that all domains in the running policy file are in enforcing mode
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    @RestrictedBuildTest
    public void testAllDomainsEnforcing() throws Exception {

        /* run sepolicy-analyze permissive check on policy file */
        ProcessBuilder pb = new ProcessBuilder(sepolicyAnalyze.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(), "permissive");
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The following SELinux domains were found to be in permissive mode:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Asserts that specified type is not associated with the specified
     * attribute.
     *
     * @param attribute
     *  The attribute name.
     * @param type
     *  The type name.
     */
    private void assertNotInAttribute(String attribute, String badtype) throws Exception {
        Set<String> actualTypes = sepolicyAnalyzeGetTypesAssociatedWithAttribute(attribute);
        if (actualTypes.contains(badtype)) {
            fail("Attribute " + attribute + " includes " + badtype);
        }
    }

    private static final byte[] readFully(InputStream in) throws IOException {
        ByteArrayOutputStream result = new ByteArrayOutputStream();
        byte[] buf = new byte[65536];
        int chunkSize;
        while ((chunkSize = in.read(buf)) != -1) {
            result.write(buf, 0, chunkSize);
        }
        return result.toByteArray();
    }

    /**
     * Runs sepolicy-analyze against the device's SELinux policy and returns the set of types
     * associated with the provided attribute.
     */
    private Set<String> sepolicyAnalyzeGetTypesAssociatedWithAttribute(
            String attribute) throws Exception {
        ProcessBuilder pb =
                new ProcessBuilder(
                        sepolicyAnalyze.getAbsolutePath(),
                        devicePolicyFile.getAbsolutePath(),
                        "attribute",
                        attribute);
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        int errorCode = p.waitFor();
        if (errorCode != 0) {
            fail("sepolicy-analyze attribute " + attribute + " failed with error code " + errorCode
                    + ": " + new String(readFully(p.getInputStream())));
        }
        try (BufferedReader in =
                new BufferedReader(new InputStreamReader(p.getInputStream()))) {
            Set<String> types = new HashSet<>();
            String type;
            while ((type = in.readLine()) != null) {
                types.add(type.trim());
            }
            return types;
        }
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns {@code true} if this device is required to be a full Treble device.
     */
    public static boolean isFullTrebleDevice(ITestDevice device)
            throws DeviceNotAvailableException {
        return PropertyUtil.getFirstApiLevel(device) > 26;
    }

    private boolean isFullTrebleDevice() throws DeviceNotAvailableException {
        return isFullTrebleDevice(mDevice);
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns {@code true} if this device is required to enforce compatible property.
     */
    public static boolean isCompatiblePropertyEnforcedDevice(ITestDevice device)
            throws DeviceNotAvailableException {
        return PropertyUtil.getFirstApiLevel(device) > 27;
    }

    // NOTE: cts/tools/selinux depends on this method. Rename/change with caution.
    /**
     * Returns {@code true} if this device has sepolicy split across different paritions.
     * This is possible even for devices launched at api level higher than 26.
     */
    public static boolean isSepolicySplit(ITestDevice device)
            throws DeviceNotAvailableException {
        return device.doesFileExist("/system/etc/selinux/plat_file_contexts");
    }

    /**
     * Asserts that no vendor domains are exempted from the prohibition on Binder use.
     *
     * <p>NOTE: binder_in_vendor_violators attribute is only there to help bring up Treble devices.
     * It offers a convenient way to temporarily bypass the prohibition on Binder use in vendor
     * domains. This attribute must not be used on production Treble devices.
     */
    public void testNoExemptionsForBinderInVendorBan() throws Exception {
        if (!isFullTrebleDevice()) {
            return;
        }

        Set<String> types =
            sepolicyAnalyzeGetTypesAssociatedWithAttribute("binder_in_vendor_violators");
        if (!types.isEmpty()) {
            List<String> sortedTypes = new ArrayList<>(types);
            Collections.sort(sortedTypes);
            fail("Policy exempts vendor domains from ban on Binder: " + sortedTypes);
        }
    }

    /**
     * Asserts that no HAL server domains are exempted from the prohibition of socket use with the
     * only exceptions for the automotive device type.
     */
    public void testNoExemptionsForSocketsUseWithinHalServer() throws Exception {
        if (!isFullTrebleDevice()) {
            return;
        }

        if (getDevice().hasFeature("android.hardware.type.automotive")) {
            return;
        }

        Set<String> types = sepolicyAnalyzeGetTypesAssociatedWithAttribute(
                "hal_automotive_socket_exemption");
        if (!types.isEmpty()) {
            List<String> sortedTypes = new ArrayList<>(types);
            Collections.sort(sortedTypes);
            fail("Policy exempts domains from ban on socket usage from HAL servers: "
                    + sortedTypes);
        }
    }

    /**
     * Asserts that no domains are exempted from the prohibition on initiating socket communications
     * between core and vendor domains.
     *
     * <p>NOTE: socket_between_core_and_vendor_violators attribute is only there to help bring up
     * Treble devices. It offers a convenient way to temporarily bypass the prohibition on
     * initiating socket communications between core and vendor domains. This attribute must not be
     * used on production Treble devices.
     */
    public void testNoExemptionsForSocketsBetweenCoreAndVendorBan() throws Exception {
        if (!isFullTrebleDevice()) {
            return;
        }

        Set<String> types =
                sepolicyAnalyzeGetTypesAssociatedWithAttribute(
                        "socket_between_core_and_vendor_violators");
        if (!types.isEmpty()) {
            List<String> sortedTypes = new ArrayList<>(types);
            Collections.sort(sortedTypes);
            fail("Policy exempts domains from ban on socket communications between core and"
                    + " vendor: " + sortedTypes);
        }
    }

    /**
     * Asserts that no vendor domains are exempted from the prohibition on directly
     * executing binaries from /system.
     * */
    public void testNoExemptionsForVendorExecutingCore() throws Exception {
        if (!isFullTrebleDevice()) {
            return;
        }

        Set<String> types =
                sepolicyAnalyzeGetTypesAssociatedWithAttribute(
                        "vendor_executes_system_violators");
        if (!types.isEmpty()) {
            List<String> sortedTypes = new ArrayList<>(types);
            Collections.sort(sortedTypes);
            fail("Policy exempts vendor domains from ban on executing files in /system: "
                    + sortedTypes);
        }
    }

    /**
     * Tests that mlstrustedsubject does not include untrusted_app
     * and that mlstrustedobject does not include app_data_file.
     * This helps prevent circumventing the per-user isolation of
     * normal apps via levelFrom=user.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testMLSAttributes() throws Exception {
        assertNotInAttribute("mlstrustedsubject", "untrusted_app");
        assertNotInAttribute("mlstrustedobject", "app_data_file");
    }

    /**
     * Tests that the seapp_contexts file on the device is valid.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testValidSeappContexts() throws Exception {

        /* obtain seapp_contexts file from running device */
        devicePlatSeappFile = File.createTempFile("plat_seapp_contexts", ".tmp");
        devicePlatSeappFile.deleteOnExit();
        deviceNonplatSeappFile = File.createTempFile("nonplat_seapp_contexts", ".tmp");
        deviceNonplatSeappFile.deleteOnExit();
        if (mDevice.pullFile("/system/etc/selinux/plat_seapp_contexts", devicePlatSeappFile)) {
            mDevice.pullFile("/vendor/etc/selinux/nonplat_seapp_contexts", deviceNonplatSeappFile);
        }else {
            mDevice.pullFile("/plat_seapp_contexts", devicePlatSeappFile);
            mDevice.pullFile("/nonplat_seapp_contexts", deviceNonplatSeappFile);
	}

        /* retrieve the checkseapp executable from jar */
        checkSeapp = copyResourceToTempFile("/checkseapp");
        checkSeapp.setExecutable(true);

        /* retrieve the AOSP seapp_neverallows file from jar */
        seappNeverAllowFile = copyResourceToTempFile("/plat_seapp_neverallows");

        /* run checkseapp on seapp_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkSeapp.getAbsolutePath(),
                "-p", devicePolicyFile.getAbsolutePath(),
                seappNeverAllowFile.getAbsolutePath(),
                devicePlatSeappFile.getAbsolutePath(),
                deviceNonplatSeappFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The seapp_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Asserts that the actual file contents starts with the expected file
     * contents.
     *
     * @param expectedFile
     *  The file with the expected contents.
     * @param actualFile
     *  The actual file being checked.
     */
    private void assertFileStartsWith(File expectedFile, File actualFile) throws Exception {
        BufferedReader expectedReader = new BufferedReader(new FileReader(expectedFile.getAbsolutePath()));
        BufferedReader actualReader = new BufferedReader(new FileReader(actualFile.getAbsolutePath()));
        String expectedLine, actualLine;
        while ((expectedLine = expectedReader.readLine()) != null) {
            actualLine = actualReader.readLine();
            assertEquals("Lines do not match:", expectedLine, actualLine);
        }
    }

    /**
     * Tests that the seapp_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testAospSeappContexts() throws Exception {

        /* obtain seapp_contexts file from running device */
        devicePlatSeappFile = File.createTempFile("seapp_contexts", ".tmp");
        devicePlatSeappFile.deleteOnExit();
        if (!mDevice.pullFile("/system/etc/selinux/plat_seapp_contexts", devicePlatSeappFile)) {
            mDevice.pullFile("/plat_seapp_contexts", devicePlatSeappFile);
        }
        /* retrieve the AOSP seapp_contexts file from jar */
        aospSeappFile = copyResourceToTempFile("/plat_seapp_contexts");

        assertFileStartsWith(aospSeappFile, devicePlatSeappFile);
    }

    /**
     * Tests that the file_contexts.bin file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testAospFileContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* retrieve the AOSP file_contexts file from jar */
        aospFcFile = copyResourceToTempFile("/plat_file_contexts");

        /* run checkfc -c plat_file_contexts file_contexts.bin */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                "-c", aospFcFile.getAbsolutePath(),
                devicePlatFcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line = result.readLine();
        assertTrue("The file_contexts file did not include the AOSP entries:\n"
                   + line + "\n",
                   line.equals("equal") || line.equals("subset"));
    }

    /**
     * Tests that the property_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testAospPropertyContexts() throws Exception {

        /* obtain property_contexts file from running device */
        devicePcFile = File.createTempFile("plat_property_contexts", ".tmp");
        devicePcFile.deleteOnExit();
        // plat_property_contexts may be either in /system/etc/sepolicy or in /
        if (!mDevice.pullFile("/system/etc/selinux/plat_property_contexts", devicePcFile)) {
            mDevice.pullFile("/plat_property_contexts", devicePcFile);
        }

        // Retrieve the AOSP property_contexts file from JAR.
        // The location of this file in the JAR has nothing to do with the location of this file on
        // Android devices. See build script of this CTS module.
        aospPcFile = copyResourceToTempFile("/plat_property_contexts");

        assertFileStartsWith(aospPcFile, devicePcFile);
    }

    /**
     * Tests that the service_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testAospServiceContexts() throws Exception {

        /* obtain service_contexts file from running device */
        deviceSvcFile = File.createTempFile("service_contexts", ".tmp");
        deviceSvcFile.deleteOnExit();
        if (!mDevice.pullFile("/system/etc/selinux/plat_service_contexts", deviceSvcFile)) {
            mDevice.pullFile("/plat_service_contexts", deviceSvcFile);
        }

        /* retrieve the AOSP service_contexts file from jar */
        aospSvcFile = copyResourceToTempFile("/plat_service_contexts");

        assertFileStartsWith(aospSvcFile, deviceSvcFile);
    }

    /**
     * Tests that the file_contexts file(s) on the device is valid.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testValidFileContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* combine plat and nonplat policies for testing */
        File combinedFcFile = File.createTempFile("combined_file_context", ".tmp");
        combinedFcFile.deleteOnExit();
        appendTo(combinedFcFile.getAbsolutePath(), devicePlatFcFile.getAbsolutePath());
        appendTo(combinedFcFile.getAbsolutePath(), deviceNonplatFcFile.getAbsolutePath());

        /* run checkfc sepolicy file_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(),
                combinedFcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("file_contexts was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that the property_contexts file on the device is valid.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testValidPropertyContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        File propertyInfoChecker = copyResourceToTempFile("/property_info_checker");
        propertyInfoChecker.setExecutable(true);

        /* obtain property_contexts file from running device */
        devicePcFile = File.createTempFile("property_contexts", ".tmp");
        devicePcFile.deleteOnExit();
        // plat_property_contexts may be either in /system/etc/sepolicy or in /
        if (!mDevice.pullFile("/system/etc/selinux/plat_property_contexts", devicePcFile)) {
            mDevice.pullFile("/plat_property_contexts", devicePcFile);
        }

        /* run property_info_checker on property_contexts */
        ProcessBuilder pb = new ProcessBuilder(propertyInfoChecker.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(),
                devicePcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The property_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that the service_contexts file on the device is valid.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testValidServiceContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* obtain service_contexts file from running device */
        deviceSvcFile = File.createTempFile("service_contexts", ".tmp");
        deviceSvcFile.deleteOnExit();
        mDevice.pullFile("/service_contexts", deviceSvcFile);

        /* run checkfc -s on service_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                "-s", devicePolicyFile.getAbsolutePath(),
                deviceSvcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The service_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    public static boolean isMac() {
        String os = System.getProperty("os.name").toLowerCase();
        return (os.startsWith("mac") || os.startsWith("darwin"));
    }

    private void setupLibraries() throws Exception {
        // The host side binary tests are host OS specific. Use Linux
        // libraries on Linux and Mac libraries on Mac.
        if (isMac()) {
            libsepolwrap = copyResourceToTempFile("/libsepolwrap.dylib");
            libcpp = copyResourceToTempFile("/libc++.dylib");
            libcpp.renameTo(new File(System.getProperty("java.io.tmpdir") + "/libc++.dylib"));
        } else {
            libsepolwrap = copyResourceToTempFile("/libsepolwrap.so");
            libcpp = copyResourceToTempFile("/libc++.so");
            libcpp.renameTo(new File(System.getProperty("java.io.tmpdir") + "/libc++.so"));
        }
        libsepolwrap.deleteOnExit();
        libcpp.deleteOnExit();
    }

    private void assertSepolicyTests(String test, String testExecutable) throws Exception {
        setupLibraries();
        sepolicyTests = copyResourceToTempFile(testExecutable);
        sepolicyTests.setExecutable(true);
        ProcessBuilder pb = new ProcessBuilder(
                sepolicyTests.getAbsolutePath(),
                "-l", libsepolwrap.getAbsolutePath(),
                "-f", devicePlatFcFile.getAbsolutePath(),
                "-f", deviceNonplatFcFile.getAbsolutePath(),
                "-p", devicePolicyFile.getAbsolutePath(),
                "--test", test);
        Map<String, String> env = pb.environment();
        if (isMac()) {
            env.put("DYLD_LIBRARY_PATH", System.getProperty("java.io.tmpdir"));
        } else {
            env.put("LD_LIBRARY_PATH", System.getProperty("java.io.tmpdir"));
        }
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue(errorString.toString(), errorString.length() == 0);
    }

    /**
     * Tests that all types on /data have the data_file_type attribute.
     *
     * @throws Exception
     */
    public void testDataTypeViolators() throws Exception {
        assertSepolicyTests("TestDataTypeViolations", "/sepolicy_tests");
    }

    /**
     * Tests that all types in /proc have the proc_type attribute.
     *
     * @throws Exception
     */
    public void testProcTypeViolators() throws Exception {
        assertSepolicyTests("TestProcTypeViolations", "/sepolicy_tests");
    }

    /**
     * Tests that all types in /sys have the sysfs_type attribute.
     *
     * @throws Exception
     */
    public void testSysfsTypeViolators() throws Exception {
        assertSepolicyTests("TestSysfsTypeViolations", "/sepolicy_tests");
    }

    /**
     * Tests that all types on /vendor have the vendor_file_type attribute.
     *
     * @throws Exception
     */
    public void testVendorTypeViolators() throws Exception {
        assertSepolicyTests("TestVendorTypeViolations", "/sepolicy_tests");
    }

    /**
     * Tests that all domains with entrypoints on /system have the coredomain
     * attribute, and that all domains with entrypoints on /vendor do not have the
     * coredomain attribute.
     *
     * @throws Exception
     */
    public void testCoredomainViolators() throws Exception {
        assertSepolicyTests("CoredomainViolations", "/treble_sepolicy_tests");
    }

   /**
     * Tests that the policy defines no booleans (runtime conditional policy).
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testNoBooleans() throws Exception {

        /* run sepolicy-analyze booleans check on policy file */
        ProcessBuilder pb = new ProcessBuilder(sepolicyAnalyze.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(), "booleans");
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The policy contained booleans:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that important domain labels are being appropriately applied.
     */

    /**
     * Asserts that no processes are running in a domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     */
    private void assertDomainEmpty(String domain) throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        String msg = "Expected no processes in SELinux domain \"" + domain + "\""
            + " Found: \"" + procs + "\"";
        assertNull(msg, procs);
    }

    /**
     * Asserts that a domain exists and that only one, well defined, process is
     * running in that domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executable
     *  The path of the executable or application package name.
     */
    private void assertDomainOne(String domain, String executable) throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(executable);
        String msg = "Expected 1 process in SELinux domain \"" + domain + "\""
            + " Found \"" + procs + "\"";
        assertNotNull(msg, procs);
        assertEquals(msg, 1, procs.size());

        msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
            + "Found: \"" + procs + "\"";
        assertEquals(msg, executable, procs.get(0).procTitle);

        msg = "Expected 1 process with executable \"" + executable + "\""
            + " Found \"" + procs + "\"";
        assertNotNull(msg, exeProcs);
        assertEquals(msg, 1, exeProcs.size());

        msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
            + "Found: \"" + procs + "\"";
        assertEquals(msg, domain, exeProcs.get(0).label);
    }

    /**
     * Asserts that a domain may exist. If a domain exists, the cardinality of
     * the domain is verified to be 1 and that the correct process is running in
     * that domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executable
     *  The path of the executable or application package name.
     */
    private void assertDomainZeroOrOne(String domain, String executable)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(executable);

        if (procs != null) {
            String msg = "Expected 1 process in SELinux domain \"" + domain + "\""
            + " Found: \"" + procs + "\"";
            assertEquals(msg, 1, procs.size());

            msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
                + "Found: \"" + procs.get(0) + "\"";
            assertEquals(msg, executable, procs.get(0).procTitle);
        }

        if (exeProcs != null) {
            String msg = "Expected 1 process with executable \"" + executable + "\""
            + " Found: \"" + procs + "\"";
            assertEquals(msg, 1, exeProcs.size());

            msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
                + "Found: \"" + procs.get(0) + "\"";
            assertEquals(msg, domain, exeProcs.get(0).label);
        }
    }

    /**
     * Asserts that a domain must exist, and that the cardinality is greater
     * than or equal to 1.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executables
     *  The path of the allowed executables or application package names.
     */
    private void assertDomainN(String domain, String... executables)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        String msg = "Expected 1 or more processes in SELinux domain but found none.";
        assertNotNull(msg, procs);

        Set<String> execList = new HashSet<String>(Arrays.asList(executables));

        for (ProcessDetails p : procs) {
            msg = "Expected one of \"" + execList + "\" in SELinux domain \"" + domain + "\""
                + " Found: \"" + p + "\"";
            assertTrue(msg, execList.contains(p.procTitle));
        }

        for (String exe : executables) {
            List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(exe);

            if (exeProcs != null) {
                for (ProcessDetails p : exeProcs) {
                    msg = "Expected executable \"" + exe + "\" in SELinux domain \""
                        + domain + "\"" + " Found: \"" + p + "\"";
                    assertEquals(msg, domain, p.label);
                }
            }
        }
    }

    /**
     * Asserts that a domain, if it exists, is only running the listed executables.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executables
     *  The path of the allowed executables or application package names.
     */
    private void assertDomainHasExecutable(String domain, String... executables)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);

        if (procs != null) {
            Set<String> execList = new HashSet<String>(Arrays.asList(executables));

            for (ProcessDetails p : procs) {
                String msg = "Expected one of \"" + execList + "\" in SELinux domain \""
                    + domain + "\"" + " Found: \"" + p + "\"";
                assertTrue(msg, execList.contains(p.procTitle));
            }
        }
    }

    /**
     * Asserts that an executable exists and is only running in the listed domains.
     *
     * @param executable
     *  The path of the executable to check.
     * @param domains
     *  The list of allowed domains.
     */
    private void assertExecutableHasDomain(String executable, String... domains)
        throws DeviceNotAvailableException {
        List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(executable);
        Set<String> domainList = new HashSet<String>(Arrays.asList(domains));

        String msg = "Expected 1 or more processes for executable \"" + executable + "\".";
        assertNotNull(msg, exeProcs);

        for (ProcessDetails p : exeProcs) {
            msg = "Expected one of  \"" + domainList + "\" for executable \"" + executable
                    + "\"" + " Found: \"" + p.label + "\"";
            assertTrue(msg, domainList.contains(p.label));
        }
    }

    /* Init is always there */
    @CddTest(requirement="9.7")
    public void testInitDomain() throws DeviceNotAvailableException {
        assertDomainHasExecutable("u:r:init:s0", "/init");
        assertDomainHasExecutable("u:r:vendor_init:s0", "/init");
        assertExecutableHasDomain("/init", "u:r:init:s0", "u:r:vendor_init:s0");
    }

    /* Ueventd is always there */
    @CddTest(requirement="9.7")
    public void testUeventdDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:ueventd:s0", "/sbin/ueventd");
    }

    /* healthd may or may not exist */
    @CddTest(requirement="9.7")
    public void testHealthdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:healthd:s0", "/system/bin/healthd");
    }

    /* Servicemanager is always there */
    @CddTest(requirement="9.7")
    public void testServicemanagerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:servicemanager:s0", "/system/bin/servicemanager");
    }

    /* Vold is always there */
    @CddTest(requirement="9.7")
    public void testVoldDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:vold:s0", "/system/bin/vold");
    }

    /* netd is always there */
    @CddTest(requirement="9.7")
    public void testNetdDomain() throws DeviceNotAvailableException {
        assertDomainN("u:r:netd:s0", "/system/bin/netd", "/system/bin/iptables-restore", "/system/bin/ip6tables-restore");
    }

    /* Surface flinger is always there */
    @CddTest(requirement="9.7")
    public void testSurfaceflingerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:surfaceflinger:s0", "/system/bin/surfaceflinger");
    }

    /* Zygote is always running */
    @CddTest(requirement="9.7")
    public void testZygoteDomain() throws DeviceNotAvailableException {
        assertDomainN("u:r:zygote:s0", "zygote", "zygote64");
    }

    /* Checks drmserver for devices that require it */
    @CddTest(requirement="9.7")
    public void testDrmServerDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:drmserver:s0", "/system/bin/drmserver");
    }

    /* Installd is always running */
    @CddTest(requirement="9.7")
    public void testInstalldDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:installd:s0", "/system/bin/installd");
    }

    /* keystore is always running */
    @CddTest(requirement="9.7")
    public void testKeystoreDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:keystore:s0", "/system/bin/keystore");
    }

    /* System server better be running :-P */
    @CddTest(requirement="9.7")
    public void testSystemServerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:system_server:s0", "system_server");
    }

    /* Watchdogd may or may not be there */
    @CddTest(requirement="9.7")
    public void testWatchdogdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:watchdogd:s0", "/sbin/watchdogd");
    }

    /* logd may or may not be there */
    @CddTest(requirement="9.7")
    public void testLogdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:logd:s0", "/system/bin/logd");
    }

    /* lmkd may or may not be there */
    @CddTest(requirement="9.7")
    public void testLmkdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:lmkd:s0", "/system/bin/lmkd");
    }

    /* Wifi may be off so cardinality of 0 or 1 is ok */
    @CddTest(requirement="9.7")
    public void testWpaDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:wpa:s0", "/system/bin/wpa_supplicant");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    @CddTest(requirement="9.7")
    public void testInitShellDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:init_shell:s0");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    @CddTest(requirement="9.7")
    public void testRecoveryDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:recovery:s0");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    @CddTest(requirement="9.7")
    @RestrictedBuildTest
    public void testSuDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:su:s0");
    }

    /*
     * All kthreads should be in kernel context.
     */
    @CddTest(requirement="9.7")
    public void testKernelDomain() throws DeviceNotAvailableException {
        String domain = "u:r:kernel:s0";
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        if (procs != null) {
            for (ProcessDetails p : procs) {
                assertTrue("Non Kernel thread \"" + p + "\" found!", p.isKernel());
            }
        }
    }

    private static class ProcessDetails {
        public String label;
        public String user;
        public int pid;
        public int ppid;
        public String procTitle;

        private static HashMap<String, ArrayList<ProcessDetails>> procMap;
        private static HashMap<String, ArrayList<ProcessDetails>> exeMap;
        private static int kernelParentThreadpid = -1;

        ProcessDetails(String label, String user, int pid, int ppid, String procTitle) {
            this.label = label;
            this.user = user;
            this.pid = pid;
            this.ppid = ppid;
            this.procTitle = procTitle;
        }

        @Override
        public String toString() {
            return "label: " + label
                    + " user: " + user
                    + " pid: " + pid
                    + " ppid: " + ppid
                    + " cmd: " + procTitle;
        }


        private static void createProcMap(ITestDevice tDevice) throws DeviceNotAvailableException {

            /* take the output of a ps -Z to do our analysis */
            CollectingOutputReceiver psOut = new CollectingOutputReceiver();
            // TODO: remove "toybox" below and just run "ps"
            tDevice.executeShellCommand("toybox ps -A -o label,user,pid,ppid,cmdline", psOut);
            String psOutString = psOut.getOutput();
            Pattern p = Pattern.compile(
                    "^([\\w_:]+)\\s+([\\w_]+)\\s+(\\d+)\\s+(\\d+)\\s+(\\p{Graph}+)(\\s\\p{Graph}+)*\\s*$",
                    Pattern.MULTILINE);
            Matcher m = p.matcher(psOutString);
            procMap = new HashMap<String, ArrayList<ProcessDetails>>();
            exeMap = new HashMap<String, ArrayList<ProcessDetails>>();
            while(m.find()) {
                String domainLabel = m.group(1);
                String user = m.group(2);
                int pid = Integer.parseInt(m.group(3));
                int ppid = Integer.parseInt(m.group(4));
                String procTitle = m.group(5);
                ProcessDetails proc = new ProcessDetails(domainLabel, user, pid, ppid, procTitle);
                if (procMap.get(domainLabel) == null) {
                    procMap.put(domainLabel, new ArrayList<ProcessDetails>());
                }
                procMap.get(domainLabel).add(proc);
                if (procTitle.equals("[kthreadd]") && ppid == 0) {
                    kernelParentThreadpid = pid;
                }
                if (exeMap.get(procTitle) == null) {
                    exeMap.put(procTitle, new ArrayList<ProcessDetails>());
                }
                exeMap.get(procTitle).add(proc);
            }
        }

        public static HashMap<String, ArrayList<ProcessDetails>> getProcMap(ITestDevice tDevice)
                throws DeviceNotAvailableException{
            if (procMap == null) {
                createProcMap(tDevice);
            }
            return procMap;
        }

        public static HashMap<String, ArrayList<ProcessDetails>> getExeMap(ITestDevice tDevice)
                throws DeviceNotAvailableException{
            if (exeMap == null) {
                createProcMap(tDevice);
            }
            return exeMap;
        }

        public boolean isKernel() {
            return (pid == kernelParentThreadpid || ppid == kernelParentThreadpid);
        }
    }
}
