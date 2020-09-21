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
import android.provider.Settings;
import android.signature.cts.DexApiDocumentParser;
import android.signature.cts.DexField;
import android.signature.cts.DexMember;
import android.signature.cts.DexMemberChecker;
import android.signature.cts.DexMethod;
import android.signature.cts.FailureType;
import repackaged.android.test.InstrumentationTestRunner;

public abstract class BaseKillswitchTest extends AbstractApiTest {

    protected String mErrorMessageAppendix;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        DexMemberChecker.init();
    }

    protected String getGlobalExemptions() {
      return Settings.Global.getString(
          getInstrumentation().getContext().getContentResolver(),
          Settings.Global.HIDDEN_API_BLACKLIST_EXEMPTIONS);
    }

    // Test shared by all the subclasses.
    public void testKillswitchMechanism() {
        runWithTestResultObserver(resultObserver -> {
            DexMemberChecker.Observer observer = new DexMemberChecker.Observer() {
                @Override
                public void classAccessible(boolean accessible, DexMember member) {
                    if (!accessible) {
                        resultObserver.notifyFailure(
                                FailureType.MISSING_CLASS,
                                member.toString(),
                                "Class from boot classpath is not accessible"
                                        + mErrorMessageAppendix);
                    }
                }

                @Override
                public void fieldAccessibleViaReflection(boolean accessible, DexField field) {
                    if (!accessible) {
                        resultObserver.notifyFailure(
                                FailureType.MISSING_FIELD,
                                field.toString(),
                                "Field from boot classpath is not accessible via reflection"
                                        + mErrorMessageAppendix);
                    }
                }

                @Override
                public void fieldAccessibleViaJni(boolean accessible, DexField field) {
                    if (!accessible) {
                        resultObserver.notifyFailure(
                                FailureType.MISSING_FIELD,
                                field.toString(),
                                "Field from boot classpath is not accessible via JNI"
                                        + mErrorMessageAppendix);
                    }
                }

                @Override
                public void methodAccessibleViaReflection(boolean accessible, DexMethod method) {
                    if (method.isStaticConstructor()) {
                        // Skip static constructors. They cannot be discovered with reflection.
                        return;
                    }

                    if (!accessible) {
                        resultObserver.notifyFailure(
                                FailureType.MISSING_METHOD,
                                method.toString(),
                                "Method from boot classpath is not accessible via reflection"
                                        + mErrorMessageAppendix);
                    }
                }

                @Override
                public void methodAccessibleViaJni(boolean accessible, DexMethod method) {
                    if (!accessible) {
                        resultObserver.notifyFailure(
                                FailureType.MISSING_METHOD,
                                method.toString(),
                                "Method from boot classpath is not accessible via JNI"
                                        + mErrorMessageAppendix);
                    }
                }

            };
            classProvider.getAllClasses().forEach(klass -> {
                classProvider.getAllMembers(klass).forEach(member -> {
                    DexMemberChecker.checkSingleMember(member, observer);
                });
            });
        });
    }
}
