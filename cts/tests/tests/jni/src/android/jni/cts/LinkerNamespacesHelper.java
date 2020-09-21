/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.jni.cts;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.support.test.InstrumentationRegistry;
import dalvik.system.PathClassLoader;

class LinkerNamespacesHelper {
    private final static String PUBLIC_CONFIG_DIR = "/system/etc/";
    private final static String PRODUCT_CONFIG_DIR = "/product/etc/";
    private final static String SYSTEM_CONFIG_FILE = PUBLIC_CONFIG_DIR + "public.libraries.txt";
    private final static Pattern EXTENSION_CONFIG_FILE_PATTERN = Pattern.compile(
            "public\\.libraries-([A-Za-z0-9\\-_]+)\\.txt");
    private final static Pattern EXTENSION_LIBRARY_FILE_PATTERN = Pattern.compile(
            "lib[^.]+\\.([A-Za-z0-9\\-_]+)\\.so");
    private final static String VENDOR_CONFIG_FILE = "/vendor/etc/public.libraries.txt";
    private final static String[] PUBLIC_SYSTEM_LIBRARIES = {
        "libaaudio.so",
        "libandroid.so",
        "libc.so",
        "libcamera2ndk.so",
        "libdl.so",
        "libEGL.so",
        "libGLESv1_CM.so",
        "libGLESv2.so",
        "libGLESv3.so",
        "libicui18n.so",
        "libicuuc.so",
        "libjnigraphics.so",
        "liblog.so",
        "libmediandk.so",
        "libm.so",
        "libnativewindow.so",
        "libneuralnetworks.so",
        "libOpenMAXAL.so",
        "libOpenSLES.so",
        "libRS.so",
        "libstdc++.so",
        "libsync.so",
        "libvulkan.so",
        "libz.so"
    };
    // The grey-list.
    private final static String[] PRIVATE_SYSTEM_LIBRARIES = {
        "libandroid_runtime.so",
        "libbinder.so",
        "libcrypto.so",
        "libcutils.so",
        "libexpat.so",
        "libgui.so",
        "libmedia.so",
        "libnativehelper.so",
        "libskia.so",
        "libssl.so",
        "libstagefright.so",
        "libsqlite.so",
        "libui.so",
        "libutils.so",
        "libvorbisidec.so",
    };

    private final static String WEBVIEW_PLAT_SUPPORT_LIB = "libwebviewchromium_plat_support.so";

    private static List<String> readPublicLibrariesFile(File file) throws IOException {
        List<String> libs = new ArrayList<>();
        if (file.exists()) {
            try (BufferedReader br = new BufferedReader(new FileReader(file))) {
                String line;
                while ((line = br.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty() || line.startsWith("#")) {
                        continue;
                    }
                    libs.add(line);
                }
            }
        }
        return libs;
    }

    private static String readExtensionConfigFiles(String configDir, List<String> libs) throws IOException {
        File[] configFiles = new File(configDir).listFiles(
                new FilenameFilter() {
                    public boolean accept(File dir, String name) {
                        return EXTENSION_CONFIG_FILE_PATTERN.matcher(name).matches();
                    }
                });
        if (configFiles == null) return null;

        for (File configFile: configFiles) {
            String fileName = configFile.toPath().getFileName().toString();
            Matcher configMatcher = EXTENSION_CONFIG_FILE_PATTERN.matcher(fileName);
            if (configMatcher.matches()) {
                String companyName = configMatcher.group(1);
                // a lib in public.libraries-acme.txt should be
                // libFoo.acme.so
                List<String> libNames = readPublicLibrariesFile(configFile);
                for (String lib : libNames) {
                    Matcher libMatcher = EXTENSION_LIBRARY_FILE_PATTERN.matcher(lib);
                    if (libMatcher.matches() && libMatcher.group(1).equals(companyName)) {
                        libs.add(lib);
                    } else {
                        return "Library \"" + lib + "\" in " + configFile.toString()
                                + " must have company name " + companyName + " as suffix.";
                    }
                }
            }
        }
        return null;
    }

    public static String runAccessibilityTest() throws IOException {
        List<String> systemLibs = new ArrayList<>();

        Collections.addAll(systemLibs, PUBLIC_SYSTEM_LIBRARIES);

        if (InstrumentationRegistry.getContext().getPackageManager().
                hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) {
            systemLibs.add(WEBVIEW_PLAT_SUPPORT_LIB);
        }

        // Check if public.libraries.txt contains libs other than the
        // public system libs (NDK libs).

        List<String> oemLibs = new ArrayList<>();
        String oemLibsError = readExtensionConfigFiles(PUBLIC_CONFIG_DIR, oemLibs);
        if (oemLibsError != null) return oemLibsError;
        // OEM libs that passed above tests are available to Android app via JNI
        systemLibs.addAll(oemLibs);

        // PRODUCT libs that passed are also available
        List<String> productLibs = new ArrayList<>();
        String productLibsError = readExtensionConfigFiles(PRODUCT_CONFIG_DIR, productLibs);
        if (productLibsError != null) return productLibsError;

        List<String> vendorLibs = readPublicLibrariesFile(new File(VENDOR_CONFIG_FILE));

        // Make sure that the libs in grey-list are not exposed to apps. In fact, it
        // would be better for us to run this check against all system libraries which
        // are not NDK libs, but grey-list libs are enough for now since they have been
        // the most popular violators.
        Set<String> greyListLibs = new HashSet<>();
        Collections.addAll(greyListLibs, PRIVATE_SYSTEM_LIBRARIES);
        // Note: check for systemLibs isn't needed since we already checked
        // /system/etc/public.libraries.txt against NDK and
        // /system/etc/public.libraries-<company>.txt against lib<name>.<company>.so.
        for (String lib : vendorLibs) {
            if (greyListLibs.contains(lib)) {
                return "Internal library \"" + lib + "\" must not be available to apps.";
            }
        }

        return runAccessibilityTestImpl(systemLibs.toArray(new String[systemLibs.size()]),
                                        vendorLibs.toArray(new String[vendorLibs.size()]),
                                        productLibs.toArray(new String[productLibs.size()]));
    }

    private static native String runAccessibilityTestImpl(String[] publicSystemLibs,
                                                          String[] publicVendorLibs,
                                                          String[] publicProductLibs);

    private static void invokeIncrementGlobal(Class<?> clazz) throws Exception {
        clazz.getMethod("incrementGlobal").invoke(null);
    }
    private static int invokeGetGlobal(Class<?> clazz) throws Exception  {
        return (Integer)clazz.getMethod("getGlobal").invoke(null);
    }

    private static ApplicationInfo getApplicationInfo(String packageName) {
        PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        try {
            return pm.getApplicationInfo(packageName, 0);
        } catch (NameNotFoundException nnfe) {
            throw new RuntimeException(nnfe);
        }
    }

    private static String getSourcePath(String packageName) {
        String sourcePath = getApplicationInfo(packageName).sourceDir;
        if (sourcePath == null) {
            throw new IllegalStateException("No source path path found for " + packageName);
        }
        return sourcePath;
    }

    private static String getNativePath(String packageName) {
        String nativePath = getApplicationInfo(packageName).nativeLibraryDir;
        if (nativePath == null) {
            throw new IllegalStateException("No native path path found for " + packageName);
        }
        return nativePath;
    }

    // Verify the behaviour of native library loading in class loaders.
    // In this test:
    //    - libjninamespacea1, libjninamespacea2 and libjninamespaceb depend on libjnicommon
    //    - loaderA will load ClassNamespaceA1 (loading libjninamespacea1)
    //    - loaderA will load ClassNamespaceA2 (loading libjninamespacea2)
    //    - loaderB will load ClassNamespaceB (loading libjninamespaceb)
    //    - incrementGlobal/getGlobal operate on a static global from libjnicommon
    //      and each class should get its own view on it.
    //
    // This is a test case for 2 different scenarios:
    //    - loading native libraries in different class loaders
    //    - loading native libraries in the same class loader
    // Ideally we would have 2 different tests but JNI doesn't allow loading the same library in
    // different class loaders. So to keep the number of native libraries manageable we just
    // re-use the same class loaders for the two tests.
    public static String runClassLoaderNamespaces() throws Exception {
        // Test for different class loaders.
        // Verify that common dependencies get a separate copy in each class loader.
        // libjnicommon should be loaded twice:
        // in the namespace for loaderA and the one for loaderB.
        String apkPath = getSourcePath("android.jni.cts");
        String nativePath = getNativePath("android.jni.cts");
        PathClassLoader loaderA = new PathClassLoader(
                apkPath, nativePath, ClassLoader.getSystemClassLoader());
        Class<?> testA1Class = loaderA.loadClass("android.jni.cts.ClassNamespaceA1");
        PathClassLoader loaderB = new PathClassLoader(
                apkPath, nativePath, ClassLoader.getSystemClassLoader());
        Class<?> testBClass = loaderB.loadClass("android.jni.cts.ClassNamespaceB");

        int globalA1 = invokeGetGlobal(testA1Class);
        int globalB = invokeGetGlobal(testBClass);
        if (globalA1 != 0 || globalB != 0) {
            return "Expected globals to be 0/0: globalA1=" + globalA1 + " globalB=" + globalB;
        }

        invokeIncrementGlobal(testA1Class);
        globalA1 = invokeGetGlobal(testA1Class);
        globalB = invokeGetGlobal(testBClass);
        if (globalA1 != 1 || globalB != 0) {
            return "Expected globals to be 1/0: globalA1=" + globalA1 + " globalB=" + globalB;
        }

        invokeIncrementGlobal(testBClass);
        globalA1 = invokeGetGlobal(testA1Class);
        globalB = invokeGetGlobal(testBClass);
        if (globalA1 != 1 || globalB != 1) {
            return "Expected globals to be 1/1: globalA1=" + globalA1 + " globalB=" + globalB;
        }

        // Test for the same class loaders.
        // Verify that if we load ClassNamespaceA2 into loaderA we get the same view on the
        // globals.
        Class<?> testA2Class = loaderA.loadClass("android.jni.cts.ClassNamespaceA2");

        int globalA2 = invokeGetGlobal(testA2Class);
        if (globalA1 != 1 || globalA2 !=1) {
            return "Expected globals to be 1/1: globalA1=" + globalA1 + " globalA2=" + globalA2;
        }

        invokeIncrementGlobal(testA1Class);
        globalA1 = invokeGetGlobal(testA1Class);
        globalA2 = invokeGetGlobal(testA2Class);
        if (globalA1 != 2 || globalA2 != 2) {
            return "Expected globals to be 2/2: globalA1=" + globalA1 + " globalA2=" + globalA2;
        }

        invokeIncrementGlobal(testA2Class);
        globalA1 = invokeGetGlobal(testA1Class);
        globalA2 = invokeGetGlobal(testA2Class);
        if (globalA1 != 3 || globalA2 != 3) {
            return "Expected globals to be 2/2: globalA1=" + globalA1 + " globalA2=" + globalA2;
        }
        // On success we return null.
        return null;
    }
}

class ClassNamespaceA1 {
    static {
        System.loadLibrary("jninamespacea1");
    }

    public static native void incrementGlobal();
    public static native int getGlobal();
}

class ClassNamespaceA2 {
    static {
        System.loadLibrary("jninamespacea2");
    }

    public static native void incrementGlobal();
    public static native int getGlobal();
}

class ClassNamespaceB {
    static {
        System.loadLibrary("jninamespaceb");
    }

    public static native void incrementGlobal();
    public static native int getGlobal();
}
