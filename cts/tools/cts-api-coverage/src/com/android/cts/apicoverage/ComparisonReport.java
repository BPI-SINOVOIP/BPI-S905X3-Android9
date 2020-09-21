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
import com.android.cts.apicoverage.CtsReportProto.*;

import org.xml.sax.SAXException;

import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Class that outputs an XML report of the {@link ApiCoverage} collected. It can be viewed in a
 * browser when used with the api-coverage.css and api-coverage.xsl files.
 */
class ComparisonReport {
    public static final String TEST_RESULT_FILE_NAME = "test_result.xml";
    // [module].[class]#[method]
    public static final String TESTCASE_NAME_FORMAT = "%s.%s#%s";

    private static void printUsage() {
        System.out.println("Usage: ReportChecker [OPTION]...");
        System.out.println();
        System.out.println("Generates a comparsion report about Test Suite Content and Report.");
        System.out.println();
        System.out.println("Options:");
        System.out.println("  -o FILE                output file of missing test case list");
        System.out.println("  -e FILE                output file of extra test case list ");
        System.out.println("  -r FILE                test report PB file");
        System.out.println("  -t FILE                test suite content PB file");

        System.out.println();
        System.exit(1);
    }

    private static boolean isKnownFailure(String tsName, List<String> knownFailures) {
        for (String kf : knownFailures) {
            if (tsName.startsWith(kf)) {
                return true;
            }
        }
        return false;
    }

    private static void writeComparsionReport(
            TestSuite ts,
            CtsReport tsReport,
            String outputFileName,
            String outputExtraTestFileName,
            List<String> knownFailures,
            String extraTestSubPlanFileName)
            throws IOException {

        HashMap<String, String> testCaseMap = new HashMap<String, String>();
        HashMap<String, String> extraTestCaseMap = new HashMap<String, String>();
        // Add all test case into a HashMap
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
                        String testApkType = String.format("%s,%s", cls.getApk(), pkg.getType());
                        testCaseMap.put(testCaseName, testApkType);
                        extraTestCaseMap.put(testCaseName, testApkType);
                        System.out.printf("add: %s: %s\n", testCaseName, testApkType);
                    } else {
                        System.out.printf("KF: %s\n", testCaseName);
                    }
                }
            }
        }
        System.out.printf(
                "Before: testCaseMap entries: %d & extraTestCaseMap: %d\n",
                testCaseMap.size(), extraTestCaseMap.size());

        //List missing Test Cases
        FileWriter fWriter = new FileWriter(outputFileName);
        PrintWriter pWriter = new PrintWriter(fWriter);

        pWriter.printf("no,Module,TestCase,abi\n");
        int i = 1;
        for (CtsReport.TestPackage tPkg : tsReport.getTestPackageList()) {
            for (CtsReport.TestPackage.TestSuite module : tPkg.getTestSuiteList()) {
                for (CtsReport.TestPackage.TestSuite.TestCase testCase : module.getTestCaseList()) {
                    for (CtsReport.TestPackage.TestSuite.TestCase.Test test :
                            testCase.getTestList()) {
                        String testCaseName =
                                String.format(
                                        TESTCASE_NAME_FORMAT,
                                        tPkg.getName(),
                                        testCase.getName(),
                                        test.getName());

                        System.out.printf(
                                "rm: %s, %s\n",
                                testCaseName, extraTestCaseMap.remove(testCaseName));
                        if (!testCaseMap.containsKey(testCaseName)) {
                            pWriter.printf(
                                    "%d,%s,%s,%s\n",
                                    i++, module.getName(), testCaseName, tPkg.getAbi());
                        }
                    }
                }
            }
        }
        pWriter.close();

        //List extra Test Cases
        fWriter = new FileWriter(outputExtraTestFileName);
        pWriter = new PrintWriter(fWriter);
        pWriter.printf("no,TestCase,Apk,Type\n");
        int j = 1;
        for (Map.Entry<String, String> test : extraTestCaseMap.entrySet()) {
            pWriter.printf("%d,%s,%s\n", j++, test.getKey(), test.getValue());
        }
        pWriter.close();
        System.out.printf(
                "After: testCaseMap entries: %d & extraTestCaseMap: %d\n",
                testCaseMap.size(), extraTestCaseMap.size());
                
        if (null != extraTestSubPlanFileName) {
            //Create extra Test Cases subplan
            fWriter = new FileWriter(extraTestSubPlanFileName);
            pWriter = new PrintWriter(fWriter);
            pWriter.println("<?xml version=\'1.0\' encoding=\'UTF-8\' standalone=\'no\' ?>");
            pWriter.println("<SubPlan version=\'2.0\'>");
            for (Map.Entry<String, String> test : extraTestCaseMap.entrySet()) {
                pWriter.printf(
                        "  <Entry include=\"%s\"/>\n", test.getKey().replaceFirst("\\.", " "));
            }
            pWriter.println("</SubPlan>");
            pWriter.close();
        }

    }

    public static void main(String[] args) throws IOException, SAXException, Exception {
        String outputFileName = "./missingTestCase.csv";
        String testReportFileName = "./extraTestCase.csv";
        String testSuiteFileName = null;
        String testSuiteContentFileName = null;
        String outputExtraTestFileName = null;
        String extraTestSubPlanFileName = null;
        int numTestModule = 0;

        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-")) {
                if ("-o".equals(args[i])) {
                    outputFileName = getExpectedArg(args, ++i);
                } else if ("-r".equals(args[i])) {
                    testReportFileName = getExpectedArg(args, ++i);
                } else if ("-e".equals(args[i])) {
                    outputExtraTestFileName = getExpectedArg(args, ++i);
                } else if ("-t".equals(args[i])) {
                    testSuiteFileName = getExpectedArg(args, ++i);
                } else if ("-s".equals(args[i])) {
                    testSuiteContentFileName = getExpectedArg(args, ++i);
                } else if ("-p".equals(args[i])) {
                    extraTestSubPlanFileName = getExpectedArg(args, ++i);
                } else {
                    printUsage();
                }
            } else {
                printUsage();
            }
        }

        if (null == testReportFileName || null == testSuiteContentFileName) {
            printUsage();
        }

        TestSuiteContent tsContent =
                TestSuiteContent.parseFrom(new FileInputStream(testSuiteContentFileName));
        TestSuite ts = TestSuite.parseFrom(new FileInputStream(testSuiteFileName));
        CtsReport tsReport = CtsReport.parseFrom(new FileInputStream(testReportFileName));

        writeComparsionReport(
                ts,
                tsReport,
                outputFileName,
                outputExtraTestFileName,
                tsContent.getKnownFailuresList(),
                extraTestSubPlanFileName);
        System.err.printf("%s", tsContent.getKnownFailuresList());
    }

    /** Get the argument or print out the usage and exit. */
    private static String getExpectedArg(String[] args, int index) {
        if (index < args.length) {
            return args[index];
        } else {
            printUsage();
            return null; // Never will happen because printUsage will call exit(1)
        }
    }
}
