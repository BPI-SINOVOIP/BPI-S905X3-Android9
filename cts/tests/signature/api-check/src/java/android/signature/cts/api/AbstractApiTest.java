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
package android.signature.cts.api;

import android.os.Bundle;
import android.signature.cts.ApiDocumentParser;
import android.signature.cts.ClassProvider;
import android.signature.cts.ExcludingClassProvider;
import android.signature.cts.FailureType;
import android.signature.cts.JDiffClassDescription;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;
import java.util.zip.ZipFile;
import org.xmlpull.v1.XmlPullParserException;
import repackaged.android.test.InstrumentationTestCase;
import repackaged.android.test.InstrumentationTestRunner;

import static android.signature.cts.CurrentApi.API_FILE_DIRECTORY;

/**
 */
public class AbstractApiTest extends InstrumentationTestCase {

    /**
     * A set of class names that are inaccessible for some reason.
     */
    private static final Set<String> KNOWN_INACCESSIBLE_CLASSES = new HashSet<>();

    static {
        // TODO(b/63383787) - These classes, which are nested annotations with @Retention(SOURCE)
        // are removed from framework.dex for an as yet unknown reason.
        KNOWN_INACCESSIBLE_CLASSES.add("android.content.pm.PackageManager.PermissionFlags");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.ProgramSelector.IdentifierType");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.ProgramSelector.ProgramType");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.RadioManager.Band");
        KNOWN_INACCESSIBLE_CLASSES.add("android.os.UserManager.UserRestrictionSource");
        KNOWN_INACCESSIBLE_CLASSES.add(
                "android.service.persistentdata.PersistentDataBlockManager.FlashLockState");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.ProgramSelector.IdentifierType");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.ProgramSelector.ProgramType");
        KNOWN_INACCESSIBLE_CLASSES.add("android.hardware.radio.RadioManager.Band");
    }

    private TestResultObserver mResultObserver;

    ClassProvider classProvider;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResultObserver = new TestResultObserver();

        // Get the arguments passed to the instrumentation.
        Bundle instrumentationArgs =
                ((InstrumentationTestRunner) getInstrumentation()).getArguments();

        // Prepare for a class provider that loads classes from bootclasspath but filters
        // out known inaccessible classes.
        // Note that com.android.internal.R.* inner classes are also excluded as they are
        // not part of API though exist in the runtime.
        classProvider = new ExcludingClassProvider(
                new BootClassPathClassesProvider(),
                name -> KNOWN_INACCESSIBLE_CLASSES.contains(name)
                        || (name != null && name.startsWith("com.android.internal.R.")));


        initializeFromArgs(instrumentationArgs);
    }

    protected void initializeFromArgs(Bundle instrumentationArgs) throws Exception {

    }

    private static boolean isAccessibleClass(JDiffClassDescription classDescription) {
        // Ignore classes that are known to be inaccessible.
        return !KNOWN_INACCESSIBLE_CLASSES.contains(classDescription.getAbsoluteClassName());
    }

    protected interface RunnableWithTestResultObserver {
        void run(TestResultObserver observer) throws Exception;
    }

    void runWithTestResultObserver(RunnableWithTestResultObserver runnable) {
        try {
            runnable.run(mResultObserver);
        } catch (Exception e) {
            StringWriter writer = new StringWriter();
            writer.write(e.toString());
            writer.write("\n");
            e.printStackTrace(new PrintWriter(writer));
            mResultObserver.notifyFailure(FailureType.CAUGHT_EXCEPTION, e.getClass().getName(),
                    writer.toString());
        }
        if (mResultObserver.mDidFail) {
            StringBuilder errorString = mResultObserver.mErrorString;
            ClassLoader classLoader = getClass().getClassLoader();
            errorString.append("\nClassLoader hierarchy\n");
            while (classLoader != null) {
                errorString.append("    ").append(classLoader).append("\n");
                classLoader = classLoader.getParent();
            }
            fail(errorString.toString());
        }
    }

    static String[] getCommaSeparatedList(Bundle instrumentationArgs, String key) {
        String argument = instrumentationArgs.getString(key);
        if (argument == null) {
            return new String[0];
        }
        return argument.split(",");
    }

    static Stream<InputStream> readFile(File file) {
        try {
            if (file.getName().endsWith(".zip")) {
                ZipFile zip = new ZipFile(file);
                return zip.stream().map(entry -> {
                    try {
                        return zip.getInputStream(entry);
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }});
            } else {
                return Stream.of(new FileInputStream(file));
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    static Stream<JDiffClassDescription> parseApiFilesAsStream(
            ApiDocumentParser apiDocumentParser, String[] apiFiles)
            throws XmlPullParserException, IOException {
        return Stream.of(apiFiles)
                .map(name -> new File(API_FILE_DIRECTORY + "/" + name))
                .flatMap(file -> readFile(file))
                .flatMap(stream -> {
                    try {
                        return apiDocumentParser.parseAsStream(stream)
                              .filter(AbstractApiTest::isAccessibleClass);
                    } catch (IOException | XmlPullParserException e) {
                        throw new RuntimeException(e);
                    }
                });
    }
}
