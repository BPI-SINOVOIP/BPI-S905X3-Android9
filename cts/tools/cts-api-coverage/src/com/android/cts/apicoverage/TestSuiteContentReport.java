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

import org.xml.sax.InputSource;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;

import java.io.File;
import java.io.FileReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.NoSuchAlgorithmException;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

class TestSuiteContentReport {
    // configuration option
    private static final String NOT_SHARDABLE_TAG = "not-shardable";
    // test class option
    private static final String RUNTIME_HIT_TAG = "runtime-hint";
    // com.android.tradefed.testtype.AndroidJUnitTest option
    private static final String PACKAGE_TAG = "package";
    // com.android.compatibility.common.tradefed.testtype.JarHostTest option
    private static final String JAR_NAME_TAG = "jar";
    // com.android.tradefed.testtype.GTest option
    private static final String NATIVE_TEST_DEVICE_PATH_TAG = "native-test-device-path";
    private static final String MODULE_TAG = "module-name";

    private static final String SUITE_API_INSTALLER_TAG = "com.android.tradefed.targetprep.suite.SuiteApkInstaller";
    private static final String JAR_HOST_TEST_TAG = "com.android.compatibility.common.tradefed.testtype.JarHostTest";
    // com.android.tradefed.targetprep.suite.SuiteApkInstaller option
    private static final String TEST_FILE_NAME_TAG = "test-file-name";
    // com.android.compatibility.common.tradefed.targetprep.FilePusher option
    private static final String PUSH_TAG = "push";

    // test class
    private static final String ANDROID_JUNIT_TEST_TAG = "com.android.tradefed.testtype.AndroidJUnitTest";

    // Target File Extensions
    private static final String CONFIG_EXT_TAG = ".config";
    private static final String JAR_EXT_TAG = ".jar";
    private static final String APK_EXT_TAG = ".apk";
    private static final String SO_EXT_TAG = ".so";
    private static final String TEST_SUITE_HARNESS = "-tradefed.jar";
    private static final String KNOWN_FAILURES_XML_FILE = "-known-failures.xml";

    private static final String OPTION_TAG = "option";
    private static final String NAME_TAG = "name";
    private static final String EXCLUDE_FILTER_TAG = "compatibility:exclude-filter";
    private static final String VALUE_TAG = "value";


    private static void printUsage() {
        System.out.println("Usage: test-suite-content-report [OPTION]...");
        System.out.println();
        System.out.println("Generates test suite content protocal buffer message.");
        System.out.println();
        System.out.println(
                "$ANDROID_HOST_OUT/bin/test-suite-content-report "
                        + "-i out/host/linux-x86/cts/android-cts "
                        + "-o ./cts-content.pb"
                        + "-t ./cts-list.pb");
        System.out.println();
        System.out.println("Options:");
        System.out.println("  -i PATH                path to the Test Suite Folder");
        System.out.println("  -o FILE                output file of Test Content Protocal Buffer");
        System.out.println("  -t FILE                output file of Test Case List Protocal Buffer");
        System.out.println();
        System.exit(1);
    }

    /** Get the argument or print out the usage and exit. */
    private static String getExpectedArg(String[] args, int index) {
        if (index < args.length) {
            return args[index];
        } else {
            printUsage();
            return null;    // Never will happen because printUsage will call exit(1)
        }
    }

    public static TestSuiteContent parseTestSuiteFolder(String testSuitePath)
            throws IOException, NoSuchAlgorithmException {

        TestSuiteContent.Builder testSuiteContent = TestSuiteContent.newBuilder();
        testSuiteContent.addFileEntries(parseFolder(testSuiteContent, testSuitePath, testSuitePath));
        return testSuiteContent.build();
    }

    // Parse a file
    private static FileMetadata parseFileMetadata(Entry.Builder fEntry, File file)
            throws Exception {
        if (file.getName().endsWith(CONFIG_EXT_TAG)) {
            fEntry.setType(Entry.EntryType.CONFIG);
            return parseConfigFile(file);
        } else if (file.getName().endsWith(APK_EXT_TAG)) {
            fEntry.setType(Entry.EntryType.APK);
        } else if (file.getName().endsWith(JAR_EXT_TAG)) {
            fEntry.setType(Entry.EntryType.JAR);
        } else if (file.getName().endsWith(SO_EXT_TAG)) {
            fEntry.setType(Entry.EntryType.SO);
        } else {
            // Just file in general
            fEntry.setType(Entry.EntryType.FILE);
        }
        return null;
    }

    private static FileMetadata parseConfigFile(File file)
            throws Exception {
        XMLReader xmlReader = XMLReaderFactory.createXMLReader();
        TestModuleConfigHandler testModuleXmlHandler = new TestModuleConfigHandler(file.getName());
        xmlReader.setContentHandler(testModuleXmlHandler);
        FileReader fileReader = null;
        try {
            fileReader = new FileReader(file);
            xmlReader.parse(new InputSource(fileReader));
            return testModuleXmlHandler.getFileMetadata();
        } finally {
            if (null != fileReader) {
                fileReader.close();
            }
        }
    }

    private static class KnownFailuresXmlHandler extends DefaultHandler {
        private TestSuiteContent.Builder mTsBld = null;

        KnownFailuresXmlHandler(TestSuiteContent.Builder tsBld) {
            mTsBld = tsBld;
        }

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            super.startElement(uri, localName, name, attributes);

            System.err.printf(
                    "ele %s: %s: %s \n",
                    localName, attributes.getValue(NAME_TAG), attributes.getValue(VALUE_TAG));
            if (EXCLUDE_FILTER_TAG.equals(attributes.getValue(NAME_TAG))) {
                String kfFilter = attributes.getValue(VALUE_TAG).replace(' ', '.');
                mTsBld.addKnownFailures(kfFilter);
            }
        }
    }

    private static void parseKnownFailures(TestSuiteContent.Builder tsBld, File file)
            throws Exception {

        ZipFile zip = new ZipFile(file);
        try {
            Enumeration<? extends ZipEntry> entries = zip.entries();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();

                if (entry.getName().endsWith(KNOWN_FAILURES_XML_FILE)) {
                    SAXParserFactory spf = SAXParserFactory.newInstance();
                    spf.setNamespaceAware(false);
                    SAXParser saxParser = spf.newSAXParser();
                    InputStream xmlStream = zip.getInputStream(entry);
                    KnownFailuresXmlHandler kfXmlHandler = new KnownFailuresXmlHandler(tsBld);
                    saxParser.parse(xmlStream, kfXmlHandler);
                    xmlStream.close();
                }
            }
        } finally {
            zip.close();
        }
    }

    // Parse a folder to add all entries
    private static Entry.Builder parseFolder(TestSuiteContent.Builder testSuiteContent, String fPath, String rPath)
            throws IOException, NoSuchAlgorithmException {
        Entry.Builder folderEntry = Entry.newBuilder();

        File folder = new File(fPath);
        File rFolder = new File(rPath);
        Path folderPath = Paths.get(folder.getAbsolutePath());
        Path rootPath = Paths.get(rFolder.getAbsolutePath());
        String folderRelativePath = rootPath.relativize(folderPath).toString();
        String folderId = getId(folderRelativePath);
        File[] fileList = folder.listFiles();
        Long folderSize = 0L;
        List <Entry> entryList = new ArrayList<Entry> ();
        for (File file : fileList){
            if (file.isFile()){
                String fileRelativePath = rootPath.relativize(Paths.get(file.getAbsolutePath())).toString();
                Entry.Builder fileEntry = Entry.newBuilder();
                fileEntry.setId(getId(fileRelativePath));
                fileEntry.setName(file.getName());
                fileEntry.setSize(file.length());
                fileEntry.setContentId(getFileContentId(file));
                fileEntry.setRelativePath(fileRelativePath);
                fileEntry.setParentId(folderId);
                try {
                    FileMetadata fMetadata = parseFileMetadata(fileEntry, file);
                    if (null != fMetadata) {
                        fileEntry.setFileMetadata(fMetadata);
                    }
                    // get [cts]-known-failures.xml
                    if (file.getName().endsWith(TEST_SUITE_HARNESS)) {
                        parseKnownFailures(testSuiteContent, file);
                    }
                } catch (Exception ex) {
                    System.err.println(
                            String.format("Cannot parse %s",
                                    file.getAbsolutePath()));
                    ex.printStackTrace();
                }
                testSuiteContent.addFileEntries(fileEntry);
                entryList.add(fileEntry.build());
                folderSize += file.length();
            } else if (file.isDirectory()){
                Entry.Builder subFolderEntry = parseFolder(testSuiteContent, file.getAbsolutePath(), rPath);
                subFolderEntry.setParentId(folderId);
                testSuiteContent.addFileEntries(subFolderEntry);
                folderSize += subFolderEntry.getSize();
                entryList.add(subFolderEntry.build());
            }
        }

        folderEntry.setId(folderId);
        folderEntry.setName(folderRelativePath);
        folderEntry.setSize(folderSize);
        folderEntry.setType(Entry.EntryType.FOLDER);
        folderEntry.setContentId(getFolderContentId(folderEntry, entryList));
        folderEntry.setRelativePath(folderRelativePath);
        return folderEntry;
    }

    private static String getFileContentId(File file)
            throws IOException, NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        FileInputStream fis = new FileInputStream(file);

        byte[] dataBytes = new byte[10240];

        int nread = 0;
        while ((nread = fis.read(dataBytes)) != -1) {
          md.update(dataBytes, 0, nread);
        };
        byte[] mdbytes = md.digest();

        // Converts to Hex String
        StringBuffer hexString = new StringBuffer();
        for (int i=0;i<mdbytes.length;i++) {
            hexString.append(Integer.toHexString(0xFF & mdbytes[i]));
        }
        return hexString.toString();
    }

    private static String getFolderContentId(Entry.Builder folderEntry, List<Entry> entryList)
            throws IOException, NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance("SHA-256");

        for (Entry entry: entryList) {
            md.update(entry.getContentId().getBytes(StandardCharsets.UTF_8));
        }
        byte[] mdbytes = md.digest();

        // Converts to Hex String
        StringBuffer hexString = new StringBuffer();
        for (int i=0;i<mdbytes.length;i++) {
            hexString.append(Integer.toHexString(0xFF & mdbytes[i]));
        }
        return hexString.toString();
    }

    private static String getId(String name)
            throws IOException, NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        md.update(name.getBytes(StandardCharsets.UTF_8));
        byte[] mdbytes = md.digest();

        // Converts to Hex String
        StringBuffer hexString = new StringBuffer();
        for (int i=0;i<mdbytes.length;i++) {
            hexString.append(Integer.toHexString(0xFF & mdbytes[i]));
        }
        return hexString.toString();
    }

    // Iterates though all test suite content and prints them.
    static void printTestSuiteContent(TestSuiteContent tsContent) {
        //Header
        System.out.printf("no,type,name,size,relative path,id,content id,parent id,description,test class");
        // test class header
        System.out.printf(",%s,%s,%s,%s,%s",
                RUNTIME_HIT_TAG, PACKAGE_TAG, JAR_NAME_TAG, NATIVE_TEST_DEVICE_PATH_TAG, MODULE_TAG);
        // target preparer header
        System.out.printf(",%s,%s\n",
                TEST_FILE_NAME_TAG, PUSH_TAG);

        int i = 1;
        for (Entry entry: tsContent.getFileEntriesList()) {
            System.out.printf("%d,%s,%s,%d,%s,%s,%s,%s",
                i++, entry.getType(), entry.getName(), entry.getSize(),
                entry.getRelativePath(), entry.getId(), entry.getContentId(),
                entry.getParentId());

            if (Entry.EntryType.CONFIG == entry.getType()) {
                ConfigMetadata config = entry.getFileMetadata().getConfigMetadata();
                System.out.printf(",%s", entry.getFileMetadata().getDescription());
                List<Option> optList;
                List<ConfigMetadata.TestClass> testClassesList = config.getTestClassesList();
                String rtHit = "";
                String pkg = "";
                String jar = "";
                String ntdPath = "";
                String module = "";

                for (ConfigMetadata.TestClass tClass : testClassesList) {
                    System.out.printf(",%s", tClass.getTestClass());
                    optList = tClass.getOptionsList();
                    for (Option opt : optList) {
                        if (RUNTIME_HIT_TAG.equalsIgnoreCase(opt.getName())) {
                            rtHit = rtHit + opt.getValue() + " ";
                        } else if (PACKAGE_TAG.equalsIgnoreCase(opt.getName())) {
                            pkg = pkg + opt.getValue() + " ";
                        } else if (JAR_NAME_TAG.equalsIgnoreCase(opt.getName())) {
                            jar = jar + opt.getValue() + " ";
                        } else if (NATIVE_TEST_DEVICE_PATH_TAG.equalsIgnoreCase(opt.getName())) {
                            ntdPath = ntdPath + opt.getValue() + " ";
                        } else if (MODULE_TAG.equalsIgnoreCase(opt.getName())) {
                            module = module + opt.getValue() + " ";
                        }
                    }
                }
                System.out.printf(",%s,%s,%s,%s,%s", rtHit.trim(), pkg.trim(),
                        jar.trim(), module.trim(), ntdPath.trim());

                List<ConfigMetadata.TargetPreparer> tPrepList = config.getTargetPreparersList();
                String testFile = "";
                String pushList = "";
                for (ConfigMetadata.TargetPreparer tPrep : tPrepList) {
                    optList = tPrep.getOptionsList();
                    for (Option opt : optList) {
                        if (TEST_FILE_NAME_TAG.equalsIgnoreCase(opt.getName())) {
                            testFile = testFile + opt.getValue() + " ";
                        } else if (PUSH_TAG.equalsIgnoreCase(opt.getName())) {
                            pushList = pushList + opt.getValue() + " ";
                        }
                    }
                }
                System.out.printf(",%s,%s", testFile.trim(), pushList.trim());
            }
            System.out.printf("\n");
        }

        System.out.printf("\nKnown Failures\n");
        for (String kf : tsContent.getKnownFailuresList()) {
            System.out.printf("%s\n", kf);
        }
    }

    public static void main(String[] args)
            throws IOException, NoSuchAlgorithmException {
        String outputFilePath = "./tsContentMessage.pb";
        String outputTestCaseListFilePath = "./tsTestCaseList.pb";
        String tsPath = "";

        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-")) {
                if ("-o".equals(args[i])) {
                    outputFilePath = getExpectedArg(args, ++i);
                } else if ("-t".equals(args[i])) {
                    outputTestCaseListFilePath = getExpectedArg(args, ++i);
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

        TestSuiteContent tsContent = parseTestSuiteFolder(tsPath);

        // Write test suite content message to disk.
        FileOutputStream output = new FileOutputStream(outputFilePath);
        try {
          tsContent.writeTo(output);
        } finally {
          output.close();
        }

        // Read message from the file and print them out
        TestSuiteContent tsContent1 =
                TestSuiteContent.parseFrom(new FileInputStream(outputFilePath));
        printTestSuiteContent(tsContent1);
    }
}
