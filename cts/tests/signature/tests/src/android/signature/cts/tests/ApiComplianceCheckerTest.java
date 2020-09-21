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

import android.signature.cts.ApiComplianceChecker;
import android.signature.cts.ClassProvider;
import android.signature.cts.FailureType;
import android.signature.cts.JDiffClassDescription;
import android.signature.cts.ResultObserver;
import android.signature.cts.tests.data.ExtendedNormalInterface;
import android.signature.cts.tests.data.NormalClass;
import android.signature.cts.tests.data.NormalInterface;
import java.lang.reflect.Modifier;

/**
 * Test class for JDiffClassDescription.
 */
@SuppressWarnings("deprecation")
public class ApiComplianceCheckerTest extends AbstractApiCheckerTest<ApiComplianceChecker> {
    
    @Override
    protected ApiComplianceChecker createChecker(ResultObserver resultObserver,
            ClassProvider provider) {
        return new ApiComplianceChecker(resultObserver, provider);
    }

    public void testNormalClassCompliance() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public class NormalClass");
    }

    public void testMissingClass() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISSING_CLASS);
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "NoSuchClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        checkSignatureCompliance(clz, observer);
        observer.validate();
    }

    public void testSimpleConstructor() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffConstructor constructor =
                new JDiffClassDescription.JDiffConstructor("NormalClass", Modifier.PUBLIC);
        clz.addConstructor(constructor);
        checkSignatureCompliance(clz);
        assertEquals(constructor.toSignatureString(), "public NormalClass()");
    }

    public void testOneArgConstructor() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffConstructor constructor =
                new JDiffClassDescription.JDiffConstructor("NormalClass", Modifier.PRIVATE);
        constructor.addParam("java.lang.String");
        clz.addConstructor(constructor);
        checkSignatureCompliance(clz);
        assertEquals(constructor.toSignatureString(), "private NormalClass(java.lang.String)");
    }

    public void testConstructorThrowsException() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffConstructor constructor =
                new JDiffClassDescription.JDiffConstructor("NormalClass", Modifier.PROTECTED);
        constructor.addParam("java.lang.String");
        constructor.addParam("java.lang.String");
        constructor.addException("android.signature.cts.tests.data.NormalException");
        clz.addConstructor(constructor);
        checkSignatureCompliance(clz);
        assertEquals(constructor.toSignatureString(),
                "protected NormalClass(java.lang.String, java.lang.String) " +
                        "throws android.signature.cts.tests.data.NormalException");
    }

    public void testPackageProtectedConstructor() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffConstructor constructor =
                new JDiffClassDescription.JDiffConstructor("NormalClass", 0);
        constructor.addParam("java.lang.String");
        constructor.addParam("java.lang.String");
        constructor.addParam("java.lang.String");
        clz.addConstructor(constructor);
        checkSignatureCompliance(clz);
        assertEquals(constructor.toSignatureString(),
                "NormalClass(java.lang.String, java.lang.String, java.lang.String)");
    }

    public void testStaticMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("staticMethod",
                Modifier.STATIC | Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "public static void staticMethod()");
    }

    public void testSyncMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("syncMethod",
                Modifier.SYNCHRONIZED | Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "public synchronized void syncMethod()");
    }

    public void testPackageProtectMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("packageProtectedMethod", 0, "boolean");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "boolean packageProtectedMethod()");
    }

    public void testPrivateMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("privateMethod", Modifier.PRIVATE,
                "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "private void privateMethod()");
    }

    public void testProtectedMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("protectedMethod", Modifier.PROTECTED,
                "java.lang.String");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "protected java.lang.String protectedMethod()");
    }

    public void testThrowsMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("throwsMethod", Modifier.PUBLIC, "void");
        method.addException("android.signature.cts.tests.data.NormalException");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "public void throwsMethod() " +
                "throws android.signature.cts.tests.data.NormalException");
    }

    public void testNativeMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("nativeMethod",
                Modifier.PUBLIC | Modifier.NATIVE, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "public native void nativeMethod()");
    }

    public void testFinalField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "FINAL_FIELD", "java.lang.String", Modifier.PUBLIC | Modifier.FINAL, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "public final java.lang.String FINAL_FIELD");
    }

    public void testStaticField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "STATIC_FIELD", "java.lang.String", Modifier.PUBLIC | Modifier.STATIC, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "public static java.lang.String STATIC_FIELD");
    }

    public void testVolatileFiled() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "VOLATILE_FIELD", "java.lang.String", Modifier.PUBLIC | Modifier.VOLATILE, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "public volatile java.lang.String VOLATILE_FIELD");
    }

    public void testTransientField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "TRANSIENT_FIELD", "java.lang.String",
                Modifier.PUBLIC | Modifier.TRANSIENT, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(),
                "public transient java.lang.String TRANSIENT_FIELD");
    }

    public void testPackageField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "PACAKGE_FIELD", "java.lang.String", 0, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "java.lang.String PACAKGE_FIELD");
    }

    public void testPrivateField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "PRIVATE_FIELD", "java.lang.String", Modifier.PRIVATE, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "private java.lang.String PRIVATE_FIELD");
    }

    public void testProtectedField() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "PROTECTED_FIELD", "java.lang.String", Modifier.PROTECTED, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(), "protected java.lang.String PROTECTED_FIELD");
    }

    public void testFieldValue() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "VALUE_FIELD", "java.lang.String",
                Modifier.PUBLIC | Modifier.FINAL | Modifier.STATIC, "\"\\u2708\"");
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(field.toSignatureString(),
                "public static final java.lang.String VALUE_FIELD");
    }

    public void testFieldValueChanged() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISMATCH_FIELD);
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "VALUE_FIELD", "java.lang.String",
                Modifier.PUBLIC | Modifier.FINAL | Modifier.STATIC, "\"&#9992;\"");
        clz.addField(field);
        checkSignatureCompliance(clz, observer);
        assertEquals(field.toSignatureString(),
                "public static final java.lang.String VALUE_FIELD");
        observer.validate();
    }

    public void testInnerClass() {
        JDiffClassDescription clz = createClass("NormalClass.InnerClass");
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "innerClassData", "java.lang.String", Modifier.PRIVATE, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public class NormalClass.InnerClass");
    }

    public void testInnerInnerClass() {
        JDiffClassDescription clz = createClass(
                "NormalClass.InnerClass.InnerInnerClass");
        JDiffClassDescription.JDiffField field = new JDiffClassDescription.JDiffField(
                "innerInnerClassData", "java.lang.String", Modifier.PRIVATE, VALUE);
        clz.addField(field);
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(),
                "public class NormalClass.InnerClass.InnerInnerClass");
    }

    public void testInnerInterface() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "NormalClass.InnerInterface");
        clz.setType(JDiffClassDescription.JDiffType.INTERFACE);
        clz.setModifier(Modifier.PUBLIC | Modifier.STATIC | Modifier.ABSTRACT);
        clz.addMethod(
                method("doSomething", Modifier.PUBLIC | Modifier.ABSTRACT, "void"));
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public interface NormalClass.InnerInterface");
    }

    public void testInterface() {
        JDiffClassDescription clz = createInterface("NormalInterface");
        clz.addMethod(
                method("doSomething", Modifier.ABSTRACT | Modifier.PUBLIC, "void"));
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public interface NormalInterface");
    }

    public void testFinalClass() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "FinalClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        clz.setModifier(Modifier.PUBLIC | Modifier.FINAL);
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public final class FinalClass");
    }

    /**
     * Test the case where the API declares the method not synchronized, but it
     * actually is.
     */
    public void testAddingSync() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISMATCH_METHOD);
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("syncMethod", Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz, observer);
        observer.validate();
    }

    /**
     * Test the case where the API declares the method is synchronized, but it
     * actually is not.
     */
    public void testRemovingSync() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("notSyncMethod",
                Modifier.SYNCHRONIZED | Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
    }

    /**
     * API says method is not native, but it actually is. http://b/1839558
     */
    public void testAddingNative() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("nativeMethod", Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
    }

    /**
     * API says method is native, but actually isn't. http://b/1839558
     */
    public void testRemovingNative() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("notNativeMethod",
                Modifier.NATIVE | Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
    }

    public void testAbstractClass() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "AbstractClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        clz.setModifier(Modifier.PUBLIC | Modifier.ABSTRACT);
        checkSignatureCompliance(clz);
        assertEquals(clz.toSignatureString(), "public abstract class AbstractClass");
    }

    /**
     * API lists class as abstract, reflection does not. http://b/1839622
     */
    public void testRemovingAbstractFromAClass() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "NormalClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        clz.setModifier(Modifier.PUBLIC | Modifier.ABSTRACT);
        checkSignatureCompliance(clz);
    }

    /**
     * reflection lists class as abstract, api does not. http://b/1839622
     */
    public void testAddingAbstractToAClass() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISMATCH_CLASS);
        JDiffClassDescription clz = createClass("AbstractClass");
        checkSignatureCompliance(clz, observer);
        observer.validate();
    }

    public void testFinalMethod() {
        JDiffClassDescription clz = createClass(NormalClass.class.getSimpleName());
        JDiffClassDescription.JDiffMethod method = method("finalMethod",
                Modifier.PUBLIC | Modifier.FINAL, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
        assertEquals(method.toSignatureString(), "public final void finalMethod()");
    }

    /**
     * Final Class, API lists methods as non-final, reflection has it as final.
     * http://b/1839589
     */
    public void testAddingFinalToAMethodInAFinalClass() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "FinalClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        clz.setModifier(Modifier.PUBLIC | Modifier.FINAL);
        JDiffClassDescription.JDiffMethod method = method("finalMethod", Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
    }

    /**
     * Final Class, API lists methods as final, reflection has it as non-final.
     * http://b/1839589
     */
    public void testRemovingFinalToAMethodInAFinalClass() {
        JDiffClassDescription clz = new JDiffClassDescription(
                "android.signature.cts.tests.data", "FinalClass");
        clz.setType(JDiffClassDescription.JDiffType.CLASS);
        clz.setModifier(Modifier.PUBLIC | Modifier.FINAL);
        JDiffClassDescription.JDiffMethod method = method("nonFinalMethod",
                Modifier.PUBLIC | Modifier.FINAL, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz);
    }

    /**
     * non-final Class, API lists methods as non-final, reflection has it as
     * final. http://b/1839589
     */
    public void testAddingFinalToAMethodInANonFinalClass() {
        ExpectFailure observer = new ExpectFailure(FailureType.MISMATCH_METHOD);
        JDiffClassDescription clz = createClass("NormalClass");
        JDiffClassDescription.JDiffMethod method = method("finalMethod", Modifier.PUBLIC, "void");
        clz.addMethod(method);
        checkSignatureCompliance(clz, observer);
        observer.validate();
    }

    public void testExtendedNormalInterface() {
        NoFailures observer = new NoFailures();
        runWithApiChecker(observer, checker -> {
            JDiffClassDescription iface = createInterface(NormalInterface.class.getSimpleName());
            iface.addMethod(method("doSomething", Modifier.PUBLIC, "void"));
            checker.addBaseClass(iface);

            JDiffClassDescription clz =
                    createInterface(ExtendedNormalInterface.class.getSimpleName());
            clz.addMethod(method("doSomethingElse", Modifier.PUBLIC | Modifier.ABSTRACT, "void"));
            clz.addImplInterface(iface.getAbsoluteClassName());
            checker.checkSignatureCompliance(clz);
        });
    }
}