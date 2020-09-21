/*
* Copyright (C) 2015 The Android Open Source Project
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

package android.permission2.cts;

import static android.os.Build.VERSION.SECURITY_PATCH;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PermissionGroupInfo;
import android.content.pm.PermissionInfo;
import android.platform.test.annotations.AppModeFull;
import android.test.AndroidTestCase;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;
import android.util.Xml;

import org.xmlpull.v1.XmlPullParser;

import java.io.InputStream;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Tests for permission policy on the platform.
 */
@AppModeFull(reason = "Instant apps cannot read the system servers permission")
public class PermissionPolicyTest extends AndroidTestCase {
    private static final Date HIDE_NON_SYSTEM_OVERLAY_WINDOWS_PATCH_DATE = parseDate("2017-11-01");
    private static final String HIDE_NON_SYSTEM_OVERLAY_WINDOWS_PERMISSION
            = "android.permission.HIDE_NON_SYSTEM_OVERLAY_WINDOWS";

    private static final String LOG_TAG = "PermissionProtectionTest";

    private static final String PLATFORM_PACKAGE_NAME = "android";

    private static final String PLATFORM_ROOT_NAMESPACE = "android.";

    private static final String AUTOMOTIVE_SERVICE_PACKAGE_NAME = "com.android.car";

    private static final String TAG_PERMISSION = "permission";

    private static final String ATTR_NAME = "name";
    private static final String ATTR_PERMISSION_GROUP = "permissionGroup";
    private static final String ATTR_PROTECTION_LEVEL = "protectionLevel";

    public void testPlatformPermissionPolicyUnaltered() throws Exception {
        Map<String, PermissionInfo> declaredPermissionsMap =
                getPermissionsForPackage(getContext(), PLATFORM_PACKAGE_NAME);

        List<String> offendingList = new ArrayList<>();

        List<PermissionGroupInfo> declaredGroups = getContext().getPackageManager()
                .getAllPermissionGroups(0);
        Set<String> declaredGroupsSet = new ArraySet<>();
        for (PermissionGroupInfo declaredGroup : declaredGroups) {
            declaredGroupsSet.add(declaredGroup.name);
        }

        Set<String> expectedPermissionGroups = new ArraySet<>();
        List<PermissionInfo> expectedPermissions = loadExpectedPermissions(R.raw.android_manifest);

        if (getContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            expectedPermissions.addAll(loadExpectedPermissions(R.raw.automotive_android_manifest));
            declaredPermissionsMap.putAll(
                    getPermissionsForPackage(getContext(), AUTOMOTIVE_SERVICE_PACKAGE_NAME));
        }

        for (PermissionInfo expectedPermission : expectedPermissions) {
            String expectedPermissionName = expectedPermission.name;
            if (shouldSkipPermission(expectedPermissionName)) {
                continue;
            }

            // OEMs cannot remove permissions
            PermissionInfo declaredPermission = declaredPermissionsMap.get(expectedPermissionName);
            if (declaredPermission == null) {
                offendingList.add("Permission " + expectedPermissionName + " must be declared");
                continue;
            }

            // We want to end up with OEM defined permissions and groups to check their namespace
            declaredPermissionsMap.remove(expectedPermissionName);
            // Collect expected groups to check if OEM defined groups aren't in platform namespace
            expectedPermissionGroups.add(expectedPermission.group);

            // OEMs cannot change permission protection
            final int expectedProtection = expectedPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_BASE;
            final int declaredProtection = declaredPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_BASE;
            if (expectedProtection != declaredProtection) {
                offendingList.add(
                        String.format(
                                "Permission %s invalid protection level %x, expected %x",
                                expectedPermissionName, declaredProtection, expectedProtection));
            }

            // OEMs cannot change permission protection flags
            final int expectedProtectionFlags = expectedPermission.getProtectionFlags();
            final int declaredProtectionFlags = declaredPermission.getProtectionFlags();
            if (expectedProtectionFlags != declaredProtectionFlags) {
                offendingList.add(
                        String.format(
                                "Permission %s invalid enforced protection %x, expected %x",
                                expectedPermissionName,
                                declaredProtectionFlags,
                                expectedProtectionFlags));
            }

            // OEMs cannot change permission grouping
            if ((declaredPermission.protectionLevel & PermissionInfo.PROTECTION_DANGEROUS) != 0) {
                if (!expectedPermission.group.equals(declaredPermission.group)) {
                    offendingList.add(
                            "Permission " + expectedPermissionName + " not in correct group "
                            + "(expected=" + expectedPermission.group + " actual="
                                    + declaredPermission.group);
                }

                if (!declaredGroupsSet.contains(declaredPermission.group)) {
                    offendingList.add(
                            "Permission group " + expectedPermission.group + " must be defined");
                }
            }
        }

        // OEMs cannot define permissions in the platform namespace
        for (String permission : declaredPermissionsMap.keySet()) {
            if (permission.startsWith(PLATFORM_ROOT_NAMESPACE)) {
                final PermissionInfo permInfo = declaredPermissionsMap.get(permission);
                offendingList.add(
                        "Cannot define permission " + permission
                        + ", package " + permInfo.packageName
                        + " in android namespace");
            }
        }

        // OEMs cannot define groups in the platform namespace
        for (PermissionGroupInfo declaredGroup : declaredGroups) {
            if (!expectedPermissionGroups.contains(declaredGroup.name)) {
                if (declaredGroup.name != null) {
                    if (declaredGroup.packageName.equals(PLATFORM_PACKAGE_NAME)
                            && declaredGroup.name.startsWith(PLATFORM_ROOT_NAMESPACE)) {
                        offendingList.add(
                                "Cannot define group " + declaredGroup.name
                                + ", package " + declaredGroup.packageName
                                + " in android namespace");
                    }
                }
            }
        }

        // OEMs cannot define new ephemeral permissions
        for (String permission : declaredPermissionsMap.keySet()) {
            PermissionInfo info = declaredPermissionsMap.get(permission);
            if ((info.protectionLevel & PermissionInfo.PROTECTION_FLAG_INSTANT) != 0) {
                offendingList.add("Cannot define new instant permission " + permission);
            }
        }

        // Fail on any offending item
        String errMsg =
                String.format(
                        "Platform Permission Policy Unaltered:\n%s",
                        TextUtils.join("\n", offendingList));
        assertTrue(errMsg, offendingList.isEmpty());
    }

    private List<PermissionInfo> loadExpectedPermissions(int resourceId) throws Exception {
        List<PermissionInfo> permissions = new ArrayList<>();
        try (InputStream in = getContext().getResources().openRawResource(resourceId)) {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(in, null);

            final int outerDepth = parser.getDepth();
            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }
                if (TAG_PERMISSION.equals(parser.getName())) {
                    PermissionInfo permissionInfo = new PermissionInfo();
                    permissionInfo.name = parser.getAttributeValue(null, ATTR_NAME);
                    permissionInfo.group = parser.getAttributeValue(null, ATTR_PERMISSION_GROUP);
                    permissionInfo.protectionLevel = parseProtectionLevel(
                            parser.getAttributeValue(null, ATTR_PROTECTION_LEVEL));
                    permissions.add(permissionInfo);
                } else {
                    Log.e(LOG_TAG, "Unknown tag " + parser.getName());
                }
            }
        }
        return permissions;
    }

    private static int parseProtectionLevel(String protectionLevelString) {
        int protectionLevel = 0;
        String[] fragments = protectionLevelString.split("\\|");
        for (String fragment : fragments) {
            switch (fragment.trim()) {
                case "normal": {
                    protectionLevel |= PermissionInfo.PROTECTION_NORMAL;
                } break;
                case "dangerous": {
                    protectionLevel |= PermissionInfo.PROTECTION_DANGEROUS;
                } break;
                case "signature": {
                    protectionLevel |= PermissionInfo.PROTECTION_SIGNATURE;
                } break;
                case "signatureOrSystem": {
                    protectionLevel |= PermissionInfo.PROTECTION_SIGNATURE;
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SYSTEM;
                } break;
                case "system": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SYSTEM;
                } break;
                case "installer": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_INSTALLER;
                } break;
                case "verifier": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_VERIFIER;
                } break;
                case "preinstalled": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PREINSTALLED;
                } break;
                case "pre23": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PRE23;
                } break;
                case "appop": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_APPOP;
                } break;
                case "development": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_DEVELOPMENT;
                } break;
                case "privileged": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PRIVILEGED;
                } break;
                case "vendorPrivileged": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_VENDOR_PRIVILEGED;
                } break;
                case "setup": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SETUP;
                } break;
                case "textClassifier": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SYSTEM_TEXT_CLASSIFIER;
                } break;
                case "instant": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_INSTANT;
                } break;
                case "runtime": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_RUNTIME_ONLY;
                } break;
            }
        }
        return protectionLevel;
    }

    private static Map<String, PermissionInfo> getPermissionsForPackage(Context context, String pkg)
            throws NameNotFoundException {
        PackageInfo packageInfo = context.getPackageManager()
                .getPackageInfo(pkg, PackageManager.GET_PERMISSIONS);
        Map<String, PermissionInfo> declaredPermissionsMap = new ArrayMap<>();

        for (PermissionInfo declaredPermission : packageInfo.permissions) {
            declaredPermissionsMap.put(declaredPermission.name, declaredPermission);
        }
        return declaredPermissionsMap;
    }

    private static Date parseDate(String date) {
        Date patchDate = new Date();
        try {
            SimpleDateFormat template = new SimpleDateFormat("yyyy-MM-dd");
            patchDate = template.parse(date);
        } catch (ParseException e) {
        }

        return patchDate;
    }

    private boolean shouldSkipPermission(String permissionName) {
        return parseDate(SECURITY_PATCH).before(HIDE_NON_SYSTEM_OVERLAY_WINDOWS_PATCH_DATE) &&
                HIDE_NON_SYSTEM_OVERLAY_WINDOWS_PERMISSION.equals(permissionName);

    }
}
