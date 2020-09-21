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
import android.signature.cts.DexApiDocumentParser;
import android.signature.cts.DexField;
import android.signature.cts.DexMember;
import android.signature.cts.DexMemberChecker;
import android.signature.cts.DexMethod;
import android.signature.cts.FailureType;
import android.signature.cts.ResultObserver;

import java.io.File;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.stream.Stream;
import java.text.ParseException;

import static android.signature.cts.CurrentApi.API_FILE_DIRECTORY;

/**
 * Checks that it is not possible to access hidden APIs.
 */
public class HiddenApiTest extends AbstractApiTest {

    private String[] hiddenApiFiles;

    @Override
    protected void initializeFromArgs(Bundle instrumentationArgs) throws Exception {
        hiddenApiFiles = getCommaSeparatedList(instrumentationArgs, "hidden-api-files");
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        DexMemberChecker.init();
    }

    /**
     * Tests that the device does not expose APIs on the provided lists of
     * DEX signatures.
     *
     * Will check the entire API, and then report the complete list of failures
     */
    public void testSignature() {
        runWithTestResultObserver(resultObserver -> {
            DexMemberChecker.Observer observer = new DexMemberChecker.Observer() {
                @Override
                public void classAccessible(boolean accessible, DexMember member) {
                }

                @Override
                public void fieldAccessibleViaReflection(boolean accessible, DexField field) {
                    if (accessible) {
                        resultObserver.notifyFailure(
                                FailureType.EXTRA_FIELD,
                                field.toString(),
                                "Hidden field accessible through reflection");
                    }
                }

                @Override
                public void fieldAccessibleViaJni(boolean accessible, DexField field) {
                    if (accessible) {
                        resultObserver.notifyFailure(
                                FailureType.EXTRA_FIELD,
                                field.toString(),
                                "Hidden field accessible through JNI");
                    }
                }

                @Override
                public void methodAccessibleViaReflection(boolean accessible, DexMethod method) {
                    if (accessible) {
                        resultObserver.notifyFailure(
                                FailureType.EXTRA_METHOD,
                                method.toString(),
                                "Hidden method accessible through reflection");
                    }
                }

                @Override
                public void methodAccessibleViaJni(boolean accessible, DexMethod method) {
                    if (accessible) {
                        resultObserver.notifyFailure(
                                FailureType.EXTRA_METHOD,
                                method.toString(),
                                "Hidden method accessible through JNI");
                    }
                }
            };
            parseDexApiFilesAsStream(hiddenApiFiles).forEach(dexMember -> {
                DexMemberChecker.checkSingleMember(dexMember, observer);
            });
        });
    }

    private static Stream<DexMember> parseDexApiFilesAsStream(String[] apiFiles) {
        DexApiDocumentParser dexApiDocumentParser = new DexApiDocumentParser();
        return Stream.of(apiFiles)
                .map(name -> new File(API_FILE_DIRECTORY + "/" + name))
                .flatMap(file -> readFile(file))
                .flatMap(stream -> dexApiDocumentParser.parseAsStream(stream));
    }

}
