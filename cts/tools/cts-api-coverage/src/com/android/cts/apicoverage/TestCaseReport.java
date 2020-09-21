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

package com.android.cts.apicoverage;


import com.android.cts.apicoverage.TestSuiteProto.*;
import com.android.tradefed.testtype.IRemoteTest;

import junit.framework.Test;

import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.iface.Annotation;
import org.jf.dexlib2.iface.AnnotationElement;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.iface.Method;
import org.junit.runners.Suite.SuiteClasses;
import org.junit.runner.RunWith;

import java.lang.reflect.Modifier;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.Paths;
import java.security.NoSuchAlgorithmException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Enumeration;
import java.util.List;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

class TestCaseReport {
    // JUNIT3 Test suffix
    private static final String TEST_TAG = "Test;";
    // Some may ends with Tests e.g. cts/tests/tests/accounts/src/android/accounts/cts/AbstractAuthenticatorTests.java
    private static final String TESTS_TAG = "Tests;";
    private static final String TEST_PREFIX_TAG = "test";
    private static final String DEPRECATED_ANNOTATION_TAG = "Ljava/lang/Deprecated;";
    private static final String RUN_WITH_ANNOTATION_TAG =
            "Lorg/junit/runner/RunWith;";
    private static final String TEST_ANNOTATION_TAG =
            "Lorg/junit/Test;";
    private static final String SUPPRESS_ANNOTATION_TAG = "/Suppress;";
    private static final String PARAMETERS_TAG =
            "Lorg/junit/runners/Parameterized$Parameters";
    private static final String ANDROID_JUNIT4_TEST_TAG =
            "AndroidJUnit4.class";
    private static final String PARAMETERIZED_TAG =
            "Parameterized.class";

    // configuration option
    private static final String NOT_SHARDABLE_TAG = "not-shardable";
    // test class option
    private static final String RUNTIME_HIT_TAG = "runtime-hint";
    // com.android.tradefed.testtype.AndroidJUnitTest option
    private static final String PACKAGE_TAG = "package";
    // com.android.compatibility.common.tradefed.testtype.JarHostTest option
    private static final String JAR_TAG = "jar";
    // com.android.tradefed.testtype.GTest option
    private static final String NATIVE_TEST_DEVICE_PATH_TAG = "native-test-device-path";
    private static final String MODULE_TAG = "module-name";

    private static final String SUITE_API_INSTALLER_TAG = "com.android.tradefed.targetprep.suite.SuiteApkInstaller";
    private static final String HOST_TEST_CLASS_TAG =
            "com.android.compatibility.common.tradefed.testtype.JarHostTest";
    // com.android.tradefed.targetprep.suite.SuiteApkInstaller option
    private static final String TEST_FILE_NAME_TAG = "test-file-name";
    // com.android.compatibility.common.tradefed.targetprep.FilePusher option
    private static final String PUSH_TAG = "push";

    // test class
    private static final String ANDROID_JUNIT_TEST_TAG = "com.android.tradefed.testtype.AndroidJUnitTest";
    private static final String DEQP_TEST_TAG = "com.drawelements.deqp.runner.DeqpTestRunner";
    private static final String GTEST_TAG = "com.android.tradefed.testtype.GTest";
    private static final String LIBCORE_TEST_TAG = "com.android.compatibility.testtype.LibcoreTest";
    private static final String DALVIK_TEST_TAG = "com.android.compatibility.testtype.DalvikTest";


    // Target File Extensions
    private static final String CONFIG_EXT_TAG = ".config";
    private static final String CONFIG_REGEX = ".config$";
    private static final String JAR_EXT_TAG = ".jar";
    private static final String APK_EXT_TAG = ".apk";
    private static final String SO_EXT_TAG = ".so";

    // [module].[class]#[method]
    public static final String TESTCASE_NAME_FORMAT = "%s.%s#%s";

    private static void printUsage() {
        System.out.println("Usage: test-suite-content-report [OPTION]...");
        System.out.println();
        System.out.println("Generates test test list protocal buffer message.");
        System.out.println();
        System.out.println(
                "$ANDROID_HOST_OUT/bin/test-suite-content-report "
                        + "-i out/host/linux-x86/cts/android-cts/testcases "
                        + "-c ./cts-content.pb"
                        + "-o ./cts-list.pb");
        System.out.println();
        System.out.println("Options:");
        System.out.println("  -i PATH                path to the Test Suite Folder");
        System.out.println("  -c FILE                input file of Test Content Protocal Buffer");
        System.out.println("  -o FILE                output file of Test Case List Protocal Buffer");
        System.out.println();
        System.exit(1);
    }

    /** Get the argument or print out the usage and exit. */
    private static String getExpectedArg(String[] args, int index) {
        if (index < args.length) {
            return args[index];
        } else {
            printUsage();
            return null;    // Never will happen because print Usage will call exit(1)
        }
    }

    private static boolean hasAnnotation(Set<? extends Annotation> annotations, String tag) {
        for (Annotation annotation : annotations) {
            if (annotation.getType().equals(tag)) {
                return true;
            }
        }
        return false;
    }

    private static boolean hasAnnotationSuffix(Set<? extends Annotation> annotations, String tag) {
        for (Annotation annotation : annotations) {
            if (annotation.getType().endsWith(tag)) {
                return true;
            }
        }
        return false;
    }

    private static TestSuite.Package.Class.ClassType chkTestType(ClassDef classDef) {
        // Only care about Public Class
        if ((classDef.getAccessFlags() & AccessFlags.PUBLIC.getValue()) == 0) {
            return TestSuite.Package.Class.ClassType.UNKNOWN;
        }

        for (Annotation annotation : classDef.getAnnotations()) {
            if (annotation.getType().equals(DEPRECATED_ANNOTATION_TAG)) {
                return TestSuite.Package.Class.ClassType.UNKNOWN;
            }
            if (annotation.getType().equals(RUN_WITH_ANNOTATION_TAG)) {
                for (AnnotationElement annotationEle : annotation.getElements()) {
                    String aName = annotationEle.getName();
                    if (aName.equals(ANDROID_JUNIT4_TEST_TAG)) {
                        return TestSuite.Package.Class.ClassType.JUNIT4;
                    } else if (aName.equals(PARAMETERIZED_TAG)) {
                        return TestSuite.Package.Class.ClassType.PARAMETERIZED;
                    }
                }
                return TestSuite.Package.Class.ClassType.JUNIT4;
            }
        }

        if (classDef.getType().endsWith(TEST_TAG) || classDef.getType().endsWith(TESTS_TAG)) {
            return TestSuite.Package.Class.ClassType.JUNIT3;
        } else {
            return TestSuite.Package.Class.ClassType.UNKNOWN;
        }
    }

    private static boolean isTargetClass(List<String> pkgList, String className) {
        boolean found = false;
        for (String pkg : pkgList) {
            if (className.startsWith(pkg)) {
                found = true;
                break;
            }
        }
        return found;
    }

    // Get test case list from an APK
    private static TestSuite.Package.Builder parseApkTestCase(List<String> apkList,
            List<String> classList, String tsPath, int api)
            throws Exception {

        TestSuite.Package.Builder tsPkgBuilder = TestSuite.Package.newBuilder();
        for (String apkName : apkList) {
            DexFile dexFile = null;
            String apkPath = Paths.get(tsPath, apkName).toString();
            try {
                dexFile = DexFileFactory.loadDexFile(apkPath, Opcodes.forApi(api));
            } catch (IOException | DexFileFactory.DexFileNotFoundException ex) {
                System.err.println("Unable to load dex file: " + apkPath);
                // ex.printStackTrace();
                continue;
            }

            tsPkgBuilder.setName(apkName);
            for (ClassDef classDef : dexFile.getClasses()) {
                // adjust the format Lclass/y;
                String className = classDef.getType().replace('/','.');
                // remove L...;
                if (className.length() > 2) {
                    className = className.substring(1, className.length() - 1);
                }

                TestSuite.Package.Class.ClassType cType;
                cType = chkTestType(classDef);
                if (TestSuite.Package.Class.ClassType.JUNIT3 == cType) {
                    TestSuite.Package.Class.Builder tClassBuilder =
                            TestSuite.Package.Class.newBuilder();
                    tClassBuilder.setClassType(cType);
                    tClassBuilder.setApk(apkName);
                    tClassBuilder.setName(className);

                    for (Method method : classDef.getMethods()) {
                        if ((method.getAccessFlags() & AccessFlags.PUBLIC.getValue()) != 0) {
                            String mName = method.getName();
                            if (hasAnnotationSuffix(
                                    method.getAnnotations(), SUPPRESS_ANNOTATION_TAG)) {
                                System.err.printf("%s#%s with Suppress:\n", className, mName);
                                System.err.println(method.getAnnotations());
                            } else if (mName.startsWith(TEST_PREFIX_TAG)) {
                                TestSuite.Package.Class.Method.Builder methodBuilder =
                                        TestSuite.Package.Class.Method.newBuilder();
                                methodBuilder.setName(mName);
                                tClassBuilder.addMethods(methodBuilder);
                            }
                        }
                    }
                    tsPkgBuilder.addClasses(tClassBuilder);
                } else if (TestSuite.Package.Class.ClassType.JUNIT4 == cType) {
                    TestSuite.Package.Class.Builder tClassBuilder =
                            TestSuite.Package.Class.newBuilder();
                    tClassBuilder.setClassType(cType);
                    tClassBuilder.setApk(apkName);
                    tClassBuilder.setName(className);

                    for (Method method : classDef.getMethods()) {
                        if (hasAnnotation(method.getAnnotations(), TEST_ANNOTATION_TAG)) {
                            String mName = method.getName();
                            TestSuite.Package.Class.Method.Builder methodBuilder =
                                    TestSuite.Package.Class.Method.newBuilder();
                            methodBuilder.setName(mName);
                            tClassBuilder.addMethods(methodBuilder);
                        }
                    }
                    tsPkgBuilder.addClasses(tClassBuilder);
                } else if (TestSuite.Package.Class.ClassType.PARAMETERIZED == cType) {
                    TestSuite.Package.Class.Builder tClassBuilder =
                            TestSuite.Package.Class.newBuilder();
                    tClassBuilder.setClassType(cType);
                    tClassBuilder.setApk(apkName);
                    tClassBuilder.setName(className);

                    for (Method method : classDef.getMethods()) {
                        if (hasAnnotation(method.getAnnotations(), TEST_ANNOTATION_TAG)) {
                            String mName = method.getName();
                            TestSuite.Package.Class.Method.Builder methodBuilder =
                                    TestSuite.Package.Class.Method.newBuilder();
                            methodBuilder.setName(mName);
                            tClassBuilder.addMethods(methodBuilder);
                        }
                    }
                    tsPkgBuilder.addClasses(tClassBuilder);
                }
            }
        }
        return tsPkgBuilder;
    }

    private static Collection<Class<?>> getJarTestClasses(File jarTestFile, String tfPath)
            throws IllegalArgumentException {
        List<Class<?>> classes = new ArrayList<>();

        try (JarFile jarFile = new JarFile(jarTestFile)) {
            Enumeration<JarEntry> e = jarFile.entries();

            URL[] urls = {
                new URL(String.format("jar:file:%s!/", jarTestFile.getAbsolutePath())),
                new URL(String.format("jar:file:%s!/", tfPath))
            };
            URLClassLoader cl =
                    URLClassLoader.newInstance(urls, JarTestFinder.class.getClassLoader());

            while (e.hasMoreElements()) {
                JarEntry je = e.nextElement();
                if (je.isDirectory()
                        || !je.getName().endsWith(".class")
                        || je.getName().contains("$")
                        || je.getName().contains("junit/")) {
                    continue;
                }
                String className = getClassName(je.getName());

                /*if (!className.endsWith("Test")) {
                    continue;
                }*/
                try {
                    Class<?> cls = cl.loadClass(className);

                    if (IRemoteTest.class.isAssignableFrom(cls)
                            || Test.class.isAssignableFrom(cls)) {
                        classes.add(cls);
                    } else if (!Modifier.isAbstract(cls.getModifiers())
                            && hasJUnit4Annotation(cls)) {
                        classes.add(cls);
                    }
                } catch (ClassNotFoundException | Error x) {
                    System.err.println(
                            String.format(
                                    "Cannot find test class %s from %s",
                                    className, jarTestFile.getName()));
                    x.printStackTrace();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return classes;
    }

    /** Helper to determine if we are dealing with a Test class with Junit4 annotations. */
    protected static boolean hasJUnit4Annotation(Class<?> classObj) {
        if (classObj.isAnnotationPresent(SuiteClasses.class)) {
            return true;
        }
        if (classObj.isAnnotationPresent(RunWith.class)) {
            return true;
        }
        /*for (Method m : classObj.getMethods()) {
            if (m.isAnnotationPresent(org.junit.Test.class)) {
                return true;
            }
        }*/
        return false;
    }

    private static String getClassName(String name) {
        // -6 because of .class
        return name.substring(0, name.length() - 6).replace('/', '.');
    }

    private static TestSuite.Package.Builder parseJarTestCase(
            List<String> jarList, String tsPath, String tfPath) throws Exception {

        TestSuite.Package.Builder tsPkgBuilder = TestSuite.Package.newBuilder();
        for (String jarName : jarList) {
            tsPkgBuilder.setName(jarName);
            Collection<Class<?>> classes =
                    getJarTestClasses(Paths.get(tsPath, jarName).toFile(), tfPath);
            for (Class<?> c : classes) {
                TestSuite.Package.Class.Builder tClassBuilder =
                        TestSuite.Package.Class.newBuilder();
                tClassBuilder.setClassType(TestSuite.Package.Class.ClassType.JAVAHOST);
                tClassBuilder.setApk(jarName);
                tClassBuilder.setName(c.getName());

                System.err.printf("class: %s\n", c.getName());
                for (java.lang.reflect.Method m : c.getMethods()) {
                    int mdf = m.getModifiers();
                    if (Modifier.isPublic(mdf) || Modifier.isProtected(mdf)) {
                        if (m.getName().startsWith(TEST_PREFIX_TAG)) {
                            System.err.printf("test: %s\n", m.getName());
                            TestSuite.Package.Class.Method.Builder methodBuilder =
                                    TestSuite.Package.Class.Method.newBuilder();
                            methodBuilder.setName(m.getName());
                            tClassBuilder.addMethods(methodBuilder);
                        }
                    }
                }
                tsPkgBuilder.addClasses(tClassBuilder);
            }
        }
        return tsPkgBuilder;
    }

    // Iterates though all test suite content and prints them.
    static TestSuite.Builder listTestCases(TestSuiteContent tsContent, String tsPath, String tfPath)
            throws Exception {
        TestSuite.Builder tsBuilder = TestSuite.newBuilder();

        int i = 1;
        for (Entry entry: tsContent.getFileEntriesList()) {
            if (Entry.EntryType.CONFIG == entry.getType()) {
                TestSuite.Package.Type pType = null;
                ConfigMetadata config = entry.getFileMetadata().getConfigMetadata();

                // getting package/class list from Test Module Configuration
                ArrayList<String> testClassList = new ArrayList<String> ();
                ArrayList<String> hostTestJarList = new ArrayList<String>();
                List<Option> optList;
                List<ConfigMetadata.TestClass> testClassesList = config.getTestClassesList();
                for (ConfigMetadata.TestClass tClass : testClassesList) {
                    boolean isHostTest = false;
                    boolean isAndroidJunitTest = false;
                    optList = tClass.getOptionsList();
                    if (HOST_TEST_CLASS_TAG.equals(tClass.getTestClass())) {
                        isHostTest = true;
                        pType = TestSuite.Package.Type.JAVAHOST;
                    } else if (ANDROID_JUNIT_TEST_TAG.equals(tClass.getTestClass())) {
                        isAndroidJunitTest = true;
                    } else if (GTEST_TAG.equals(tClass.getTestClass())) {
                        pType = TestSuite.Package.Type.GTEST;
                    } else if (DEQP_TEST_TAG.equals(tClass.getTestClass())) {
                        pType = TestSuite.Package.Type.DEQP;
                    } else if (LIBCORE_TEST_TAG.equals(tClass.getTestClass())) {
                        pType = TestSuite.Package.Type.LIBCORE;
                    } else if (DALVIK_TEST_TAG.equals(tClass.getTestClass())) {
                        // cts/tests/jdwp/AndroidTest.xml
                        pType = TestSuite.Package.Type.DALVIK;
                    } else {
                        System.err.printf(
                                "Unknown Test Type: %s %s\n",
                                entry.getName(), tClass.getTestClass());
                    }

                    for (Option opt : optList) {
                        if (isAndroidJunitTest && PACKAGE_TAG.equalsIgnoreCase(opt.getName())) {
                            testClassList.add(opt.getValue());
                        } else if (isHostTest && JAR_TAG.equalsIgnoreCase(opt.getName())) {
                            hostTestJarList.add(opt.getValue());
                        }
                    }
                }

                // getting apk list from Test Module Configuration
                ArrayList<String> testApkList = new ArrayList<String> ();
                List<ConfigMetadata.TargetPreparer> tPrepList = config.getTargetPreparersList();
                for (ConfigMetadata.TargetPreparer tPrep : tPrepList) {
                    optList = tPrep.getOptionsList();
                    for (Option opt : optList) {
                        if (TEST_FILE_NAME_TAG.equalsIgnoreCase(opt.getName())) {
                            testApkList.add(opt.getValue());
                        }
                    }
                }

                TestSuite.Package.Builder tsPkgBuilder;

                if (pType == TestSuite.Package.Type.JAVAHOST) {
                    tsPkgBuilder = parseJarTestCase(hostTestJarList, tsPath, tfPath);
                } else {
                    tsPkgBuilder = parseApkTestCase(testApkList, testClassList, tsPath, 27);
                }

                tsPkgBuilder.setName(entry.getName().replaceAll(CONFIG_REGEX, ""));
                if (null != pType) {
                    tsPkgBuilder.setType(pType);
                }

                tsBuilder.addPackages(tsPkgBuilder);
            }
        }
        return tsBuilder;
    }

    private static boolean isKnownFailure(String tsName, List<String> knownFailures) {
        for (String kf : knownFailures) {
            if (tsName.startsWith(kf)) {
                return true;
            }
        }
        return false;
    }

    // Iterates though all test suite content and prints them.
    static void printTestSuite(TestSuite ts, List<String> knownFailures) {
        //Header
        System.out.println("Module,Class,Test,no,Apk,Type");
        int i = 1;
        for (TestSuite.Package pkg : ts.getPackagesList()) {
            for (TestSuite.Package.Class cls : pkg.getClassesList()) {
                for (TestSuite.Package.Class.Method mtd : cls.getMethodsList()) {
                    String testCaseName =
                            String.format(
                                    TESTCASE_NAME_FORMAT,
                                    pkg.getName(),
                                    cls.getName(),
                                    mtd.getName());
                    // Filter out known failures
                    if (!isKnownFailure(testCaseName, knownFailures)) {
                        System.out.printf(
                                "%s,%s,%s,%d,%s,%s\n",
                                pkg.getName(),
                                cls.getName(),
                                mtd.getName(),
                                i++,
                                cls.getApk(),
                                cls.getClassType());
                    }
                }
            }
        }
    }

    // Iterates though all test suite content and prints them.
    static void printTestSuiteSummary(TestSuite ts, String fName, List<String> knownFailures)
            throws IOException {
        FileWriter fWriter = new FileWriter(fName);
        PrintWriter pWriter = new PrintWriter(fWriter);

        //Header
        pWriter.print("Module,Test#,no,Type,Class#\n");

        int i = 0;
        for (TestSuite.Package pkg : ts.getPackagesList()) {
            int classCnt = 0;
            int methodCnt = 0;
            for (TestSuite.Package.Class cls : pkg.getClassesList()) {
                for (TestSuite.Package.Class.Method mtd : cls.getMethodsList()) {
                    String testCaseName =
                            String.format(
                                    TESTCASE_NAME_FORMAT,
                                    pkg.getName(),
                                    cls.getName(),
                                    mtd.getName());
                    // Filter out known failures
                    if (!isKnownFailure(testCaseName, knownFailures)) {
                        methodCnt++;
                    }
                }
                classCnt++;
            }
            pWriter.printf(
                    "%s,%s,%d,%s,%d\n", pkg.getName(), methodCnt, i++, pkg.getType(), classCnt);
        }

        pWriter.close();
    }

    public static void main(String[] args)
            throws IOException, NoSuchAlgorithmException, Exception {
        String tsContentFilePath = "./tsContentMessage.pb";
        String outputTestCaseListFilePath = "./tsTestCaseList.pb";
        String outputSummaryFilePath = "./tsSummary.csv";
        String tsPath = "";
        String tradFedJarPath = "";

        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-")) {
                if ("-o".equals(args[i])) {
                    outputTestCaseListFilePath = getExpectedArg(args, ++i);
                } else if ("-c".equals(args[i])) {
                    tsContentFilePath = getExpectedArg(args, ++i);
                } else if ("-s".equals(args[i])) {
                    outputSummaryFilePath = getExpectedArg(args, ++i);
                } else if ("-t".equals(args[i])) {
                    tradFedJarPath = getExpectedArg(args, ++i);
                } else if ("-i".equals(args[i])) {
                    tsPath = getExpectedArg(args, ++i);
                    File file = new File(tsPath);
                    // Only acception a folder
                    if (!file.isDirectory()) {
                        printUsage();
                    }
                } else {
                    printUsage();
                }
            }
        }

        // Read message from the file and print them out
        TestSuiteContent tsContent =
                TestSuiteContent.parseFrom(new FileInputStream(tsContentFilePath));

        TestSuite ts = listTestCases(tsContent, tsPath, tradFedJarPath).build();

        // Write test case list message to disk.
        FileOutputStream output = new FileOutputStream(outputTestCaseListFilePath);
        try {
          ts.writeTo(output);
        } finally {
          output.close();
        }

        // Read message from the file and print them out
        TestSuite ts1 = TestSuite.parseFrom(new FileInputStream(outputTestCaseListFilePath));
        printTestSuite(ts1, tsContent.getKnownFailuresList());
        printTestSuiteSummary(ts1, outputSummaryFilePath, tsContent.getKnownFailuresList());
    }
}
