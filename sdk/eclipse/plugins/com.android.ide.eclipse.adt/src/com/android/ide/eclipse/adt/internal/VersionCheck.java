/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.ide.eclipse.adt.internal;

import com.android.SdkConstants;
import com.android.ide.eclipse.adt.AdtPlugin;
import com.android.ide.eclipse.adt.AdtPlugin.CheckSdkErrorHandler;
import com.android.ide.eclipse.adt.AdtPlugin.CheckSdkErrorHandler.Solution;
import com.android.ide.eclipse.adt.Messages;
import com.android.sdklib.repository.FullRevision;
import com.android.sdklib.repository.FullRevision.PreviewComparison;
import com.android.sdklib.repository.PkgProps;

import org.osgi.framework.Constants;
import org.osgi.framework.Version;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Class handling the version check for the plugin vs. the SDK.<br>
 * The plugin must be able to support all version of the SDK.
 *
 * <p/>An SDK can require a new version of the plugin.
 * <p/>The SDK contains a file with the minimum version for the plugin. This file is inside the
 * <code>tools/lib</code> directory, and is called <code>plugin.prop</code>.<br>
 * Inside that text file, there is a line in the format "plugin.version=#.#.#". This is checked
 * against the current plugin version.<br>
 *
 */
public final class VersionCheck {
    /**
     * The minimum version of the SDK Tools that this version of ADT requires.
     */
    private final static FullRevision MIN_TOOLS_REV = new FullRevision(24, 0, 2, 0);

    /**
     * Pattern to get the minimum plugin version supported by the SDK. This is read from
     * the file <code>$SDK/tools/lib/plugin.prop</code>.
     */
    private final static Pattern sPluginVersionPattern = Pattern.compile(
            "^plugin.version=(\\d+)\\.(\\d+)\\.(\\d+).*$"); //$NON-NLS-1$
    private final static Pattern sSourcePropPattern = Pattern.compile(
            "^" + PkgProps.PKG_REVISION + "=(.*)$"); //$NON-NLS-1$

    /**
     * Checks the plugin and the SDK have compatible versions.
     * @param osSdkPath The path to the SDK
     * @return true if compatible.
     */
    public static boolean checkVersion(String osSdkPath, CheckSdkErrorHandler errorHandler) {
        AdtPlugin plugin = AdtPlugin.getDefault();
        String osLibs = osSdkPath + SdkConstants.OS_SDK_TOOLS_LIB_FOLDER;

        // get the plugin property file, and grab the minimum plugin version required
        // to work with the sdk
        int minMajorVersion = -1;
        int minMinorVersion = -1;
        int minMicroVersion = -1;
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(osLibs + SdkConstants.FN_PLUGIN_PROP));
            String line;
            while ((line = reader.readLine()) != null) {
                Matcher m = sPluginVersionPattern.matcher(line);
                if (m.matches()) {
                    minMajorVersion = Integer.parseInt(m.group(1));
                    minMinorVersion = Integer.parseInt(m.group(2));
                    minMicroVersion = Integer.parseInt(m.group(3));
                    break;
                }
            }
        } catch (FileNotFoundException e) {
            // the build id will be null, and this is handled by the builders.
        } catch (IOException e) {
            // the build id will be null, and this is handled by the builders.
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                } finally {
                    reader = null;
                }
            }
        }

        // Failed to get the min plugin version number?
        if (minMajorVersion == -1 || minMinorVersion == -1 || minMicroVersion ==-1) {
            return errorHandler.handleWarning(
                    Solution.OPEN_SDK_MANAGER,
                    Messages.VersionCheck_Plugin_Version_Failed);
        }

        // Are the build tools installed? We can't query Sdk#getLatestBuildTool yet, since
        // SDK initialization typically hasn't completed yet and Sdk.getCurrent() is null.
        File buildToolsFolder = new File(osSdkPath, SdkConstants.FD_BUILD_TOOLS);
        if (!buildToolsFolder.isDirectory()) {
            return errorHandler.handleWarning(
                    Solution.OPEN_SDK_MANAGER,
                    Messages.VersionCheck_Build_Tool_Missing);
        }

        // test the plugin number
        String versionString = (String) plugin.getBundle().getHeaders().get(
                Constants.BUNDLE_VERSION);
        Version version = new Version(versionString);

        boolean valid = true;
        if (version.getMajor() < minMajorVersion) {
            valid = false;
        } else if (version.getMajor() == minMajorVersion) {
            if (version.getMinor() < minMinorVersion) {
                valid = false;
            } else if (version.getMinor() == minMinorVersion) {
                if (version.getMicro() < minMicroVersion) {
                    valid = false;
                }
            }
        }

        if (valid == false) {
            return errorHandler.handleError(
                    Solution.OPEN_P2_UPDATE,
                    String.format(Messages.VersionCheck_Plugin_Too_Old,
                            minMajorVersion, minMinorVersion, minMicroVersion, versionString));
        }

        // now check whether the tools are new enough.
        String osTools = osSdkPath + SdkConstants.OS_SDK_TOOLS_FOLDER;
        FullRevision toolsRevision = new FullRevision(Integer.MAX_VALUE);
        try {
            reader = new BufferedReader(new FileReader(osTools + SdkConstants.FN_SOURCE_PROP));
            String line;
            while ((line = reader.readLine()) != null) {
                Matcher m = sSourcePropPattern.matcher(line);
                if (m.matches()) {
                    try {
                        toolsRevision = FullRevision.parseRevision(m.group(1));
                    } catch (NumberFormatException ignore) {}
                    break;
                }
            }
        } catch (FileNotFoundException e) {
            // the build id will be null, and this is handled by the builders.
        } catch (IOException e) {
            // the build id will be null, and this is handled by the builders.
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                } finally {
                    reader = null;
                }
            }
        }

        if (toolsRevision.compareTo(MIN_TOOLS_REV, PreviewComparison.IGNORE) < 0) {
            // this is a warning only as we need to parse the SDK to allow updating
            // of the tools!
            return errorHandler.handleWarning(
                    Solution.OPEN_SDK_MANAGER,
                    String.format(Messages.VersionCheck_Tools_Too_Old,
                            MIN_TOOLS_REV, toolsRevision));
        }

        return true; // no error!
    }
}
