/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.framework.tests;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import org.junit.Assert;
import org.w3c.dom.Document;
import org.w3c.dom.Node;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathFactory;
/**
 * Utility method used for PackageMangerOTATests. Requires adb root.
 */
public class PackageManagerOTATestUtils {
    public static final String USERDATA_PARTITION = "userdata";
    public static final String PACKAGE_XML_FILE = "/data/system/packages.xml";

    private ITestDevice mDevice = null;

    /**
     * Constructor.
     *
     * @param device the {@link ITestDevice} to use when performing operations.
     * @throws DeviceNotAvailableException
     */
    public PackageManagerOTATestUtils(ITestDevice device)
            throws DeviceNotAvailableException {
        mDevice = device;
    }

    /**
     * Wipe userdata partition on device.
     *
     * @throws DeviceNotAvailableException
     */
    public void wipeDevice() throws DeviceNotAvailableException {
        // Make sure to keep the local.prop file for testing purposes.
        File prop = mDevice.pullFile("/data/local.prop");
        mDevice.rebootIntoBootloader();
        mDevice.fastbootWipePartition(USERDATA_PARTITION);
        mDevice.rebootUntilOnline();
        if (prop != null) {
            mDevice.pushFile(prop, "/data/local.prop");
            mDevice.executeShellCommand("chmod 644 /data/local.prop");
            mDevice.reboot();
        }
    }

    /**
     * Remove a system app.
     * @param systemApp {@link String} name for the application in the /system/app folder
     * @param reboot set to <code>true</code> to optionally reboot device after app removal
     *
     * @throws DeviceNotAvailableException
     */
    public void removeSystemApp(String systemApp, boolean reboot)
            throws DeviceNotAvailableException {
        mDevice.remountSystemWritable();
        String cmd = String.format("rm %s", systemApp);
        mDevice.executeShellCommand(cmd);
        if (reboot) {
            mDevice.reboot();
        }
        mDevice.waitForDeviceAvailable();
    }

    /**
     * Remove a system app and wipe the device.
     * @param systemApp {@link String} name for the application in the /system/app folder
     *
     * @throws DeviceNotAvailableException
     */
    public void removeAndWipe(String systemApp)
            throws DeviceNotAvailableException {
        removeSystemApp(systemApp, false);
        wipeDevice();
    }

    /**
     * Expect that a given xpath exists in a given xml file.
     * @param xmlFile {@link File} xml file to process
     * @param xPathString {@link String} Xpath to look for
     *
     * @return true if the xpath is found
     */
    public boolean expectExists(File xmlFile, String xPathString) {
        Node n = getNodeForXPath(xmlFile, xPathString);
        if (n != null) {
            CLog.d("Found node %s for xpath %s", n.getNodeName(), xPathString);
            return true;
        }
        return false;
    }

    /**
     * Expect that the value of a given xpath starts with a given string.
     *
     * @param xmlFile {@link File} the xml file in question
     * @param xPathString {@link String} the xpath to look for
     * @param value {@link String} the expected start string of the xpath
     *
     * @return true if the value for the xpath starts with value, false otherwise
     */
    public boolean expectStartsWith(File xmlFile, String xPathString, String value) {
        Node n = getNodeForXPath(xmlFile, xPathString);
        if ( n==null ) {
            CLog.d("Failed to find node for xpath %s", xPathString);
            return false;
        }
        CLog.d("Value of node %s: %s", xPathString, n.getNodeValue());
        return n.getNodeValue().toLowerCase().startsWith(value.toLowerCase());
    }

    /**
     * Expect that the value of a given xpath matches.
     *
     * @param xmlFile {@link File} the xml file in question
     * @param xPathString {@link String} the xpath to look for
     * @param value {@link String} the expected string value
     *
     * @return true if the value for the xpath matches, false otherwise
     */
    public boolean expectEquals(File xmlFile, String xPathString, String value) {
        Node n = getNodeForXPath(xmlFile, xPathString);
        if ( n==null ) {
            CLog.d("Failed to find node for xpath %s", xPathString);
            return false;
        }
        boolean result = n.getNodeValue().equalsIgnoreCase(value);
        if (!result) {
            CLog.v("Value of node %s: \"%s\", expected: \"%s\"",
                    xPathString, n.getNodeValue(), value);
        }
        return result;
    }

    public Node getNodeForXPath(File xmlFile, String xPathString) {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder documentBuilder = factory.newDocumentBuilder();
            Document doc = documentBuilder.parse(xmlFile);
            XPathFactory xpFactory = XPathFactory.newInstance();
            XPath xpath = xpFactory.newXPath();
            XPathExpression expr = xpath.compile(xPathString);
            Node node = (Node) expr.evaluate(doc, XPathConstants.NODE);
            return node;
        } catch (Exception e) {
            CLog.e(e);
        }
        return null;
    }

    /**
     * Check if a given package has the said permission.
     * @param packageName {@link String} the package in question
     * @param permission {@link String} the permission to look for
     *
     * @return true if the permission exists, false otherwise
     *
     * @throws DeviceNotAvailableException
     */
    public boolean packageHasPermission(String packageName, String permission)
            throws DeviceNotAvailableException {
        String cmd = "dumpsys package " + packageName;
        String res = mDevice.executeShellCommand(cmd);
        if (res != null) {
            if (res.contains("grantedPermissions:")) {
                return res.contains(permission);
            } else {
                Pattern perm = Pattern.compile(String.format("^.*%s.*granted=true.*$", permission),
                        Pattern.MULTILINE);
                Matcher m = perm.matcher(res);
                return m.find();
            }
        }
        CLog.d("Failed to execute shell command: %s", cmd);
        return false;
    }

    /**
     * Check if a given package has the said permission.
     * @param packageName {@link String} the package in question
     * @param flag {@link String} the permission to look for
     *
     * @return true if the permission exists, false otherwise
     *
     * @throws DeviceNotAvailableException
     */
    public boolean packageHasFlag(String packageName, String flag)
            throws DeviceNotAvailableException {
        String cmd = "dumpsys package " + packageName;
        String res = mDevice.executeShellCommand(cmd);
        if (res != null) {
            Pattern flags = Pattern.compile("^.*flags=\\[(.*?)\\]$", Pattern.MULTILINE);
            Matcher m = flags.matcher(res);
            if (m.find()) {
                return m.group(1).contains(flag);
            } else {
                CLog.d("Failed to find package flags record in dumpsys package output");
            }
        }
        CLog.d("Failed to execute shell command: %s", cmd);
        return false;
    }

    /**
     * Helper method to install a file
     *
     * @param localFile the {@link File} to install
     * @param replace set to <code>true</code> if re-install of app should be performed
     * @param extraArgs optional extra arguments to pass. See 'adb shell pm install --help' for
     *            available options.
     *
     * @throws DeviceNotAvailableException
     */
    public void installFile(final File localFile, final boolean replace, String... extraArgs)
            throws DeviceNotAvailableException {
        String result = mDevice.installPackage(localFile, replace, extraArgs);
        Assert.assertNull(String.format("Failed to install file %s with result %s",
                localFile.getAbsolutePath(), result), result);
    }

    /**
     * Helper method to stop system shell.
     *
     * @throws DeviceNotAvailableException
     */
    public void stopSystem() throws DeviceNotAvailableException {
        mDevice.executeShellCommand("stop");
    }

    /**
     * Helper method to start system shell. It also reset the flag dev.bootcomplete to 0 to ensure
     * that the package manager had a chance to finish.
     *
     * @throws DeviceNotAvailableException
     */
    public void startSystem() throws DeviceNotAvailableException {
        mDevice.executeShellCommand("setprop dev.bootcomplete 0");
        mDevice.executeShellCommand("start");
        mDevice.waitForDeviceAvailable();
    }

    /**
     * Convenience method to stop, then start the runtime.
     *
     * @throws DeviceNotAvailableException
     */
    public void restartSystem() throws DeviceNotAvailableException {
        stopSystem();
        startSystem();
    }

    /**
     * Push apk to system app directory on the device.
     *
     * @param localFile {@link File} the local file to install
     * @param deviceFilePath {@link String} the remote device path where to install the application
     *
     * @throws DeviceNotAvailableException
     */
    public void pushSystemApp(final File localFile, final String deviceFilePath)
            throws DeviceNotAvailableException {
        mDevice.remountSystemWritable();
        stopSystem();
        mDevice.pushFile(localFile, deviceFilePath);
        startSystem();
    }

    /**
     * Pulls packages xml file from the device.
     * @return {@link File} xml file for all packages on device.
     *
     * @throws DeviceNotAvailableException
     */
    public File pullPackagesXML() throws DeviceNotAvailableException {
        return mDevice.pullFile(PACKAGE_XML_FILE);
    }
}
