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

import com.android.cts.apicoverage.CtsReportProto.*;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;

import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
/**
 * Class that outputs an XML report of the {@link ApiCoverage} collected. It can be viewed in a
 * browser when used with the api-coverage.css and api-coverage.xsl files.
 */
class CtsReportParser {
    public static final String TEST_RESULT_FILE_NAME = "test_result.xml";

    private static void printUsage() {
        System.out.println("Usage: CtsReportParser [OPTION]... [APK]...");
        System.out.println();
        System.out.println("Generates a report about what Android NDK methods.");
        System.out.println();
        System.out.println("Options:");
        System.out.println("  -o FILE                output file or standard out if not given");
        System.out.println("  -i PATH                path to the Test_Result.xml");
        System.out.println("  -s FILE                summary output file");

        System.out.println();
        System.exit(1);
    }

    private static CtsReport parseCtsReport(String testResultPath)
            throws Exception {
        XMLReader xmlReader = XMLReaderFactory.createXMLReader();
        CtsReportHandler ctsReportXmlHandler =
                new CtsReportHandler(testResultPath);
        xmlReader.setContentHandler(ctsReportXmlHandler);
        FileReader fileReader = null;
        try {
            fileReader = new FileReader(testResultPath);
            xmlReader.parse(new InputSource(fileReader));
        } finally {
            if (fileReader != null) {
                fileReader.close();
            }
        }
        return ctsReportXmlHandler.getCtsReport();
    }

    private static void printCtsReport(CtsReport ctsReport) {
        //Header
        System.out.println("Module,Class,Test,no,Abi,Result");
        int i = 1;
        for (CtsReport.TestPackage tPkg : ctsReport.getTestPackageList()) {
            for (CtsReport.TestPackage.TestSuite tSuite : tPkg.getTestSuiteList()) {
                for (CtsReport.TestPackage.TestSuite.TestCase testCase : tSuite.getTestCaseList()) {
                    for (CtsReport.TestPackage.TestSuite.TestCase.Test test : testCase.getTestList()) {
                        System.out.printf(
                                "%s,%s,%s,%d,%s,%s\n",
                                tPkg.getName(),
                                testCase.getName(),
                                test.getName(),
                                i++,
                                tPkg.getAbi(),
                                test.getResult());
                    }
                }
            }
        }
    }

    static void printCtsReportSummary(CtsReport ctsReport, String fName)
        throws IOException{

        FileWriter fWriter = new FileWriter(fName);
        PrintWriter pWriter = new PrintWriter(fWriter);

        //Header
        pWriter.print("Module,Test#,no,abi\n");

        int moduleCnt = 0;
        for (CtsReport.TestPackage tPkg : ctsReport.getTestPackageList()) {
            int testCaseCnt = 0;
            for (CtsReport.TestPackage.TestSuite tSuite : tPkg.getTestSuiteList()) {
                for (CtsReport.TestPackage.TestSuite.TestCase testCase : tSuite.getTestCaseList()) {
                    for (CtsReport.TestPackage.TestSuite.TestCase.Test test : testCase.getTestList()) {
                        testCaseCnt++;
                    }
                }
            }
            pWriter.printf(
                    "%s,%d,%d,%s\n", tPkg.getName(), testCaseCnt, moduleCnt++, tPkg.getAbi());
        }

        pWriter.close();
    }

    public static void main(String[] args)
            throws IOException, SAXException, Exception {
        String testResultPath = "./test_result.xml";
        String outputFileName = "./test_result.pb";
        String outputSummaryFilePath = "./reportSummary.csv";
        int numTestModule = 0;

        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-")) {
                if ("-o".equals(args[i])) {
                    outputFileName = getExpectedArg(args, ++i);
                } else if ("-i".equals(args[i])) {
                    testResultPath = getExpectedArg(args, ++i);
                } else if ("-s".equals(args[i])) {
                    outputSummaryFilePath = getExpectedArg(args, ++i);
                } else {
                    printUsage();
                }
            } else {
                printUsage();
            }
        }

        // Read message from the file and print them out
        CtsReport ctsReport = parseCtsReport(testResultPath);

        printCtsReport(ctsReport);
        printCtsReportSummary(ctsReport, outputSummaryFilePath);


        // Write test case list message to disk.
        FileOutputStream output = new FileOutputStream(outputFileName);
        try {
          ctsReport.writeTo(output);
        } finally {
          output.close();
        }
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
