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

package com.android.tradefed.util;

import junit.framework.TestCase;

/** Tests for {@link AaptParser}. */
public class AaptParserTest extends TestCase {

    public void testParseInvalidInput() {
        AaptParser p = new AaptParser();
        assertFalse(p.parse("Bad data"));
    }

    public void testParseEmptyVersionCode() {
        AaptParser p = new AaptParser();
        assertTrue(p.parse("package: name='android.support.graphics.drawable.animated.test'" +
                " versionCode='' versionName=''\n" +
                "sdkVersion:'11'\n" +
                "targetSdkVersion:'23'\n" +
                "uses-permission:'android.permission.WRITE_EXTERNAL_STORAGE'\n" +
                "application: label='' icon=''\n" +
                "application-debuggable\n" +
                "uses-library:'android.test.runner'\n" +
                "uses-permission:'android.permission.READ_EXTERNAL_STORAGE'\n" +
                "uses-implied-permission:'android.permission.READ_EXTERNAL_STORAGE'," +
                "'requested WRITE_EXTERNAL_STORAGE'\n" +
                "uses-feature:'android.hardware.touchscreen'\n" +
                "uses-implied-feature:'android.hardware.touchscreen'," +
                "'assumed you require a touch screen unless explicitly made optional'\n" +
                "other-activities\n" +
                "supports-screens: 'small' 'normal' 'large' 'xlarge'\n" +
                "supports-any-density: 'true'\n" +
                "locales: '--_--'\n" +
                "densities: '160'"));
        assertEquals("", p.getVersionCode());
    }

    public void testParsePackageNameVersionLabel() {
        AaptParser p = new AaptParser();
        p.parse("package: name='com.android.foo' versionCode='13' versionName='2.3'\n" +
                "sdkVersion:'5'\n" +
                "application-label:'Foo'\n" +
                "application-label-fr:'Faa'\n" +
                "uses-permission:'android.permission.INTERNET'");
        assertEquals("com.android.foo", p.getPackageName());
        assertEquals("13", p.getVersionCode());
        assertEquals("2.3", p.getVersionName());
        assertEquals("Foo", p.getLabel());
        assertEquals(5, p.getSdkVersion());
    }

    public void testParseVersionMultipleFieldsNoLabel() {
        AaptParser p = new AaptParser();
        p.parse("package: name='com.android.foo' versionCode='217173' versionName='1.7173' " +
                "platformBuildVersionName=''\n" +
                "install-location:'preferExternal'\n" +
                "sdkVersion:'10'\n" +
                "targetSdkVersion:'21'\n" +
                "uses-permission: name='android.permission.INTERNET'\n" +
                "uses-permission: name='android.permission.ACCESS_NETWORK_STATE'\n");
        assertEquals("com.android.foo", p.getPackageName());
        assertEquals("217173", p.getVersionCode());
        assertEquals("1.7173", p.getVersionName());
        assertEquals("com.android.foo", p.getLabel());
        assertEquals(10, p.getSdkVersion());
    }

    public void testParseInvalidSdkVersion() {
        AaptParser p = new AaptParser();
        p.parse("package: name='com.android.foo' versionCode='217173' versionName='1.7173' " +
                "platformBuildVersionName=''\n" +
                "install-location:'preferExternal'\n" +
                "sdkVersion:'notavalidsdk'\n" +
                "targetSdkVersion:'21'\n" +
                "uses-permission: name='android.permission.INTERNET'\n" +
                "uses-permission: name='android.permission.ACCESS_NETWORK_STATE'\n");
        assertEquals(-1, p.getSdkVersion());
    }

    public void testParseNativeCode() {
        AaptParser p = new AaptParser();
        p.parse(
                "package: name='com.android.foo' versionCode='217173' versionName='1.7173' "
                        + "platformBuildVersionName=''\n"
                        + "install-location:'preferExternal'\n"
                        + "sdkVersion:'notavalidsdk'\n"
                        + "targetSdkVersion:'21'\n"
                        + "uses-permission: name='android.permission.INTERNET'\n"
                        + "uses-permission: name='android.permission.ACCESS_NETWORK_STATE'\n"
                        + "densities: '160'"
                        + "native-code: 'arm64-v8a'");
        assertEquals("arm64-v8a", p.getNativeCode().get(0));
    }

    public void testParseNativeCode_multi() {
        AaptParser p = new AaptParser();
        p.parse(
                "package: name='com.android.foo' versionCode='217173' versionName='1.7173' "
                        + "platformBuildVersionName=''\n"
                        + "install-location:'preferExternal'\n"
                        + "sdkVersion:'notavalidsdk'\n"
                        + "targetSdkVersion:'21'\n"
                        + "uses-permission: name='android.permission.INTERNET'\n"
                        + "uses-permission: name='android.permission.ACCESS_NETWORK_STATE'\n"
                        + "densities: '160'"
                        + "native-code: 'arm64-v8a' 'armeabi-v7a'");
        assertEquals("arm64-v8a", p.getNativeCode().get(0));
        assertEquals("armeabi-v7a", p.getNativeCode().get(1));
    }

    public void testParseNativeCode_alt() {
        AaptParser p = new AaptParser();
        p.parse(
                "package: name='com.android.foo' versionCode='217173' versionName='1.7173' "
                        + "platformBuildVersionName=''\n"
                        + "install-location:'preferExternal'\n"
                        + "sdkVersion:'notavalidsdk'\n"
                        + "targetSdkVersion:'21'\n"
                        + "uses-permission: name='android.permission.INTERNET'\n"
                        + "uses-permission: name='android.permission.ACCESS_NETWORK_STATE'\n"
                        + "densities: '160'\n"
                        + "native-code: 'arm64-v8a'\n"
                        + "alt-native-code: 'armeabi-v7a'");
        assertEquals("arm64-v8a", p.getNativeCode().get(0));
        assertEquals("armeabi-v7a", p.getNativeCode().get(1));
    }
}
