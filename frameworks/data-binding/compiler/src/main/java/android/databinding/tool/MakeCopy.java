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
package android.databinding.tool;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.w3c.dom.Document;

import android.databinding.tool.store.ResourceBundle.LayoutFileBundle;
import android.databinding.tool.util.GenerationalClassUtil;
import android.databinding.tool.writer.JavaFileWriter;

import java.io.File;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

/**
 * This class is used by make to copy resources to an intermediate directory and start processing
 * them. When aapt takes over, this can be easily extracted to a short script.
 */
public class MakeCopy {
    private static final int MANIFEST_INDEX = 0;
    private static final int SRC_INDEX = 1;
    private static final int XML_INDEX = 2;
    private static final int RES_OUT_INDEX = 3;
    private static final int RES_IN_INDEX = 4;

    private static final String APP_SUBPATH = LayoutXmlProcessor.RESOURCE_BUNDLE_PACKAGE
            .replace('.', File.separatorChar);
    private static final FilenameFilter LAYOUT_DIR_FILTER = new FilenameFilter() {
        @Override
        public boolean accept(File dir, String name) {
            return name.toLowerCase().startsWith("layout");
        }
    };

    private static final FilenameFilter XML_FILENAME_FILTER = new FilenameFilter() {
        @Override
        public boolean accept(File dir, String name) {
            return name.toLowerCase().endsWith(".xml");
        }
    };

    public static void main(String[] args) {
        if (args.length < 5) {
            System.out.println("required parameters: [-l] manifest adk-dir src-out-dir xml-out-dir " +
                            "res-out-dir res-in-dir...");
            System.out.println("Creates an android data binding class and copies resources from");
            System.out.println("res-source to res-target and modifies binding layout files");
            System.out.println("in res-target. Binding data is extracted into XML files");
            System.out.println("and placed in xml-out-dir.");
            System.out.println("  -l          indicates that this is a library");
            System.out.println("  manifest    path to AndroidManifest.xml file");
            System.out.println("  src-out-dir path to where generated source goes");
            System.out.println("  xml-out-dir path to where generated binding XML goes");
            System.out.println("  res-out-dir path to the where modified resources should go");
            System.out.println("  res-in-dir  path to source resources \"res\" directory. One" +
                    " or more are allowed.");
            System.exit(1);
        }
        final boolean isLibrary = args[0].equals("-l");
        final int indexOffset = isLibrary ? 1 : 0;
        final String applicationPackage;
        final int minSdk;
        final Document androidManifest = readAndroidManifest(
                new File(args[MANIFEST_INDEX + indexOffset]));
        try {
            final XPathFactory xPathFactory = XPathFactory.newInstance();
            final XPath xPath = xPathFactory.newXPath();
            applicationPackage = xPath.evaluate("string(/manifest/@package)", androidManifest);
            final Double minSdkNumber = (Double) xPath.evaluate(
                    "number(/manifest/uses-sdk/@android:minSdkVersion)", androidManifest,
                    XPathConstants.NUMBER);
            minSdk = minSdkNumber == null ? 1 : minSdkNumber.intValue();
        } catch (XPathExpressionException e) {
            e.printStackTrace();
            System.exit(6);
            return;
        }
        final File srcDir = new File(args[SRC_INDEX + indexOffset], APP_SUBPATH);
        if (!makeTargetDir(srcDir)) {
            System.err.println("Could not create source directory " + srcDir);
            System.exit(2);
        }
        final File resTarget = new File(args[RES_OUT_INDEX + indexOffset]);
        if (!makeTargetDir(resTarget)) {
            System.err.println("Could not create resource directory: " + resTarget);
            System.exit(4);
        }
        final File xmlDir = new File(args[XML_INDEX + indexOffset]);
        if (!makeTargetDir(xmlDir)) {
            System.err.println("Could not create xml output directory: " + xmlDir);
            System.exit(5);
        }
        System.out.println("Application Package: " + applicationPackage);
        System.out.println("Minimum SDK: " + minSdk);
        System.out.println("Target Resources: " + resTarget.getAbsolutePath());
        System.out.println("Target Source Dir: " + srcDir.getAbsolutePath());
        System.out.println("Target XML Dir: " + xmlDir.getAbsolutePath());
        System.out.println("Library? " + isLibrary);

        boolean foundSomeResources = false;
        for (int i = RES_IN_INDEX + indexOffset; i < args.length; i++) {
            final File resDir = new File(args[i]);
            if (!resDir.exists()) {
                System.out.println("Could not find resource directory: " + resDir);
            } else {
                System.out.println("Source Resources: " + resDir.getAbsolutePath());
                try {
                    FileUtils.copyDirectory(resDir, resTarget);
                    addFromFile(resDir, resTarget);
                    foundSomeResources = true;
                } catch (IOException e) {
                    System.err.println("Could not copy resources from " + resDir + " to " + resTarget +
                            ": " + e.getLocalizedMessage());
                    System.exit(3);
                }
            }
        }

        if (!foundSomeResources) {
            System.err.println("No resource directories were found.");
            System.exit(7);
        }
        processLayoutFiles(applicationPackage, resTarget, srcDir, xmlDir, minSdk,
                isLibrary);
    }

    private static Document readAndroidManifest(File manifest) {
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder documentBuilder = dbf.newDocumentBuilder();
            return documentBuilder.parse(manifest);
        } catch (Exception e) {
            System.err.println("Could not load Android Manifest from " +
                    manifest.getAbsolutePath() + ": " + e.getLocalizedMessage());
            System.exit(8);
            return null;
        }
    }

    private static void processLayoutFiles(String applicationPackage, File resTarget, File srcDir,
            File xmlDir, int minSdk, boolean isLibrary) {
        MakeFileWriter makeFileWriter = new MakeFileWriter(srcDir);
        LayoutXmlProcessor xmlProcessor = new LayoutXmlProcessor(applicationPackage,
                makeFileWriter, minSdk, isLibrary, new LayoutXmlProcessor.OriginalFileLookup() {
            @Override
            public File getOriginalFileFor(File file) {
                return file;
            }
        });
        try {
            LayoutXmlProcessor.ResourceInput input = new LayoutXmlProcessor.ResourceInput(
                    false, resTarget,resTarget
            );
            xmlProcessor.processResources(input);
            xmlProcessor.writeLayoutInfoFiles(xmlDir);
            // TODO Looks like make does not support excluding from libs ?
            xmlProcessor.writeInfoClass(null, xmlDir, null);
            Map<String, List<LayoutFileBundle>> bundles =
                    xmlProcessor.getResourceBundle().getLayoutBundles();
            if (isLibrary) {
                for (String name : bundles.keySet()) {
                    LayoutFileBundle layoutFileBundle = bundles.get(name).get(0);
                    String pkgName = layoutFileBundle.getBindingClassPackage().replace('.', '/');
                    System.err.println(pkgName + '/' + layoutFileBundle.getBindingClassName() +
                        ".class");
                }
            }
            if (makeFileWriter.getErrorCount() > 0) {
                System.exit(9);
            }
        } catch (Exception e) {
            System.err.println("Error processing layout files: " + e.getLocalizedMessage());
            System.exit(10);
        }
    }

    private static void addFromFile(File resDir, File resTarget) {
        for (File layoutDir : resDir.listFiles(LAYOUT_DIR_FILTER)) {
            if (layoutDir.isDirectory()) {
                File targetDir = new File(resTarget, layoutDir.getName());
                for (File layoutFile : layoutDir.listFiles(XML_FILENAME_FILTER)) {
                    File targetFile = new File(targetDir, layoutFile.getName());
                    FileWriter appender = null;
                    try {
                        appender = new FileWriter(targetFile, true);
                        appender.write("<!-- From: " + layoutFile.toURI().toString() + " -->\n");
                    } catch (IOException e) {
                        System.err.println("Could not update " + layoutFile + ": " +
                                e.getLocalizedMessage());
                    } finally {
                        IOUtils.closeQuietly(appender);
                    }
                }
            }
        }
    }

    private static boolean makeTargetDir(File dir) {
        if (dir.exists()) {
            return dir.isDirectory();
        }

        return dir.mkdirs();
    }

    private static class MakeFileWriter extends JavaFileWriter {
        private final File mSourceRoot;
        private int mErrorCount;

        public MakeFileWriter(File sourceRoot) {
            mSourceRoot = sourceRoot;
        }

        @Override
        public void writeToFile(String canonicalName, String contents) {
            String fileName = canonicalName.replace('.', File.separatorChar) + ".java";
            File sourceFile = new File(mSourceRoot, fileName);
            FileWriter writer = null;
            try {
                sourceFile.getParentFile().mkdirs();
                writer = new FileWriter(sourceFile);
                writer.write(contents);
            } catch (IOException e) {
                System.err.println("Could not write to " + sourceFile + ": " +
                        e.getLocalizedMessage());
                mErrorCount++;
            } finally {
                IOUtils.closeQuietly(writer);
            }
        }

        public int getErrorCount() {
            return mErrorCount;
        }
    }
}
