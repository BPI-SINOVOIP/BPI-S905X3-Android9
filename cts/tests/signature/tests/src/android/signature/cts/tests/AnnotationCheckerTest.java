/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.signature.cts.tests;

import android.signature.cts.AnnotationChecker;
import android.signature.cts.ClassProvider;
import android.signature.cts.FailureType;
import android.signature.cts.JDiffClassDescription;
import android.signature.cts.ResultObserver;
import android.signature.cts.tests.data.ApiAnnotation;
import java.lang.reflect.Modifier;

/**
 * Test class for {@link android.signature.cts.AnnotationChecker}.
 */
@SuppressWarnings("deprecation")
public class AnnotationCheckerTest extends AbstractApiCheckerTest<AnnotationChecker> {

    @Override
    protected AnnotationChecker createChecker(ResultObserver resultObserver,
            ClassProvider provider) {
        return new AnnotationChecker(resultObserver, provider, ApiAnnotation.class.getName());
    }

    private static void addConstructor(JDiffClassDescription clz, String... paramTypes) {
        JDiffClassDescription.JDiffConstructor constructor = new JDiffClassDescription.JDiffConstructor(
                clz.getShortClassName(), Modifier.PUBLIC);
        if (paramTypes != null) {
            for (String type : paramTypes) {
                constructor.addParam(type);
            }
        }
        clz.addConstructor(constructor);
    }

    private static void addPublicVoidMethod(JDiffClassDescription clz, String name) {
        clz.addMethod(method(name, Modifier.PUBLIC, "void"));
    }

    private static void addPublicBooleanField(JDiffClassDescription clz, String name) {
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                name, "boolean", Modifier.PUBLIC, VALUE);
        clz.addField(field);
    }

    /**
     * Documented API and runtime classes are exactly matched.
     */
    public void testExactApiMatch() {
        JDiffClassDescription clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
    }

    /**
     * A constructor is found in the runtime class, but not in the documented API
     */
    public void testDetectUnauthorizedConstructorApi() {
        ExpectFailure observer = new ExpectFailure(FailureType.EXTRA_METHOD);

        JDiffClassDescription clz = createClass("SystemApiClass");
        // (omitted) addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();

        observer = new ExpectFailure(FailureType.EXTRA_METHOD);

        clz = createClass("PublicApiClass");
        // (omitted) addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();
    }

    /**
     * A method is found in the runtime class, but not in the documented API
     */
    public void testDetectUnauthorizedMethodApi() {
        ExpectFailure observer = new ExpectFailure(FailureType.EXTRA_METHOD);

        JDiffClassDescription clz = createClass("SystemApiClass");
        addConstructor(clz);
        // (omitted) addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();

        observer = new ExpectFailure(FailureType.EXTRA_METHOD);

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        // (omitted) addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();
    }

    /**
     * A field is found in the runtime class, but not in the documented API
     */
    public void testDetectUnauthorizedFieldApi() {
        ExpectFailure observer = new ExpectFailure(FailureType.EXTRA_FIELD);

        JDiffClassDescription clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        // (omitted) addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();

        observer = new ExpectFailure(FailureType.EXTRA_FIELD);

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        // (omitted) addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();
    }

    /**
     * A class is found in the runtime classes, but not in the documented API
     */
    public void testDetectUnauthorizedClassApi() {
        ExpectFailure observer = new ExpectFailure(FailureType.EXTRA_CLASS);
        JDiffClassDescription clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.PublicApiClass");
        // Note that ForciblyPublicizedPrivateClass is now included in the runtime classes
        observer.validate();

        observer = new ExpectFailure(FailureType.EXTRA_CLASS);

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass");
        // Note that ForciblyPublicizedPrivateClass is now included in the runtime classes
        observer.validate();
    }

    /**
     * A member which is declared in an annotated class is currently recognized as an API.
     */
    public void testB71630695() {
        // TODO(b/71630695): currently, some API members are not annotated, because
        // a member is automatically added to the API set if it is in a class with
        // annotation and it is not @hide. This should be fixed, but until then,
        // CTS should respect the existing behavior.
        JDiffClassDescription clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addConstructor(clz, "float"); // this is not annotated

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");

        clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addPublicVoidMethod(clz, "unannotatedApiMethod"); // this is not annotated

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");

        clz = createClass("SystemApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addPublicBooleanField(clz, "unannotatedApiField"); // this is not annotated

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.PublicApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
    }

    /**
     * An API is documented, but isn't annotated in the runtime class. But, due to b/71630695, this
     * test can only be done for public API classes.
     */
    public void testDetectMissingAnnotation() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISSING_ANNOTATION);

        JDiffClassDescription clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addConstructor(clz, "int"); // this is not annotated

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();

        observer = new ExpectFailure(FailureType.MISSING_ANNOTATION);

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addPublicVoidMethod(clz, "privateMethod"); // this is not annotated

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();

        observer = new ExpectFailure(FailureType.MISSING_ANNOTATION);

        clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addPublicBooleanField(clz, "privateField"); // this is not annotated

        checkSignatureCompliance(clz, observer,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");
        observer.validate();
    }

    /**
     * A <code>@hide</code> method should be recognized as API though it is not annotated, if it is
     * overriding a method which is already an API.
     */
    public void testOverriddenHidenMethodIsApi() {
        JDiffClassDescription clz = createClass("PublicApiClass");
        addConstructor(clz);
        addPublicVoidMethod(clz, "apiMethod");
        addPublicBooleanField(clz, "apiField");
        addPublicVoidMethod(clz, "anOverriddenMethod"); // not annotated and @hide, but is API

        checkSignatureCompliance(clz,
                "android.signature.cts.tests.data.SystemApiClass",
                "android.signature.cts.tests.data.ForciblyPublicizedPrivateClass");

    }
}
