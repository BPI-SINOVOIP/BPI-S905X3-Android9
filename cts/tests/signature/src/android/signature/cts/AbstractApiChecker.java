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
package android.signature.cts;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

/**
 * Base class for those that process a set of API definition files and perform some checking on
 * them.
 */
public abstract class AbstractApiChecker {

    final ResultObserver resultObserver;

    final ClassProvider classProvider;

    AbstractApiChecker(ClassProvider classProvider, ResultObserver resultObserver) {
        this.classProvider = classProvider;
        this.resultObserver = resultObserver;
    }

    /**
     * Checks test class's name, modifier, fields, constructors, and
     * methods.
     */
    public void checkSignatureCompliance(JDiffClassDescription classDescription) {
        Class<?> runtimeClass = checkClassCompliance(classDescription);
        if (runtimeClass != null) {
            checkFieldsCompliance(classDescription, runtimeClass);
            checkConstructorCompliance(classDescription, runtimeClass);
            checkMethodCompliance(classDescription, runtimeClass);
        }
    }

    /**
     * Checks that the class found through reflection matches the
     * specification from the API xml file.
     *
     * @param classDescription a description of a class in an API.
     */
    @SuppressWarnings("unchecked")
    private Class<?> checkClassCompliance(JDiffClassDescription classDescription) {
        try {
            Class<?> runtimeClass = ReflectionHelper
                    .findRequiredClass(classDescription, classProvider);

            if (runtimeClass == null) {
                // No class found, notify the observer according to the class type
                resultObserver.notifyFailure(FailureType.missing(classDescription),
                        classDescription.getAbsoluteClassName(),
                        "Classloader is unable to find " + classDescription
                                .getAbsoluteClassName());

                return null;
            }

            if (!checkClass(classDescription, runtimeClass)) {
                return null;
            }

            return runtimeClass;
        } catch (Exception e) {
            LogHelper.loge("Got exception when checking class compliance", e);
            resultObserver.notifyFailure(
                    FailureType.CAUGHT_EXCEPTION,
                    classDescription.getAbsoluteClassName(),
                    "Exception while checking class compliance!");
            return null;
        }
    }

    /**
     * Perform any additional checks that can only be done after all api files have been processed.
     */
    public abstract void checkDeferred();

    /**
     * Implement to provide custom check of the supplied class description.
     *
     * <p>This should not peform checks on the members, those will be done separately depending
     * on the result of this method.
     *
     * @param classDescription the class description to check
     * @param runtimeClass the runtime class corresponding to the class description.
     * @return true if the checks passed and the members should now be checked.
     */
    protected abstract boolean checkClass(JDiffClassDescription classDescription,
            Class<?> runtimeClass);


    /**
     * Checks all fields in test class for compliance with the API xml.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     */
    @SuppressWarnings("unchecked")
    private void checkFieldsCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        // A map of field name to field of the fields contained in runtimeClass.
        Map<String, Field> classFieldMap = buildFieldMap(runtimeClass);
        for (JDiffClassDescription.JDiffField field : classDescription.getFields()) {
            try {
                Field f = classFieldMap.get(field.mName);
                if (f == null) {
                    resultObserver.notifyFailure(FailureType.MISSING_FIELD,
                            field.toReadableString(classDescription.getAbsoluteClassName()),
                            "No field with correct signature found:" +
                                    field.toSignatureString());
                } else {
                    checkField(classDescription, runtimeClass, field, f);
                }
            } catch (Exception e) {
                LogHelper.loge("Got exception when checking field compliance", e);
                resultObserver.notifyFailure(
                        FailureType.CAUGHT_EXCEPTION,
                        field.toReadableString(classDescription.getAbsoluteClassName()),
                        "Exception while checking field compliance");
            }
        }
    }

    /**
     * Scan a class (an its entire inheritance chain) for fields.
     *
     * @return a {@link Map} of fieldName to {@link Field}
     */
    private static Map<String, Field> buildFieldMap(Class testClass) {
        Map<String, Field> fieldMap = new HashMap<>();
        // Scan the superclass
        if (testClass.getSuperclass() != null) {
            fieldMap.putAll(buildFieldMap(testClass.getSuperclass()));
        }

        // Scan the interfaces
        for (Class interfaceClass : testClass.getInterfaces()) {
            fieldMap.putAll(buildFieldMap(interfaceClass));
        }

        // Check the fields in the test class
        for (Field field : testClass.getDeclaredFields()) {
            fieldMap.put(field.getName(), field);
        }

        return fieldMap;
    }

    protected abstract void checkField(JDiffClassDescription classDescription,
            Class<?> runtimeClass,
            JDiffClassDescription.JDiffField fieldDescription, Field field);


    /**
     * Checks whether the constructor parsed from API xml file and
     * Java reflection are compliant.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     */
    @SuppressWarnings("unchecked")
    private void checkConstructorCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        for (JDiffClassDescription.JDiffConstructor con : classDescription.getConstructors()) {
            try {
                Constructor<?> c = ReflectionHelper.findMatchingConstructor(runtimeClass, con);
                if (c == null) {
                    resultObserver.notifyFailure(FailureType.MISSING_CONSTRUCTOR,
                            con.toReadableString(classDescription.getAbsoluteClassName()),
                            "No constructor with correct signature found:" +
                                    con.toSignatureString());
                } else {
                    checkConstructor(classDescription, runtimeClass, con, c);
                }
            } catch (Exception e) {
                LogHelper.loge("Got exception when checking constructor compliance", e);
                resultObserver.notifyFailure(FailureType.CAUGHT_EXCEPTION,
                        con.toReadableString(classDescription.getAbsoluteClassName()),
                        "Exception while checking constructor compliance!");
            }
        }
    }

    protected abstract void checkConstructor(JDiffClassDescription classDescription,
            Class<?> runtimeClass,
            JDiffClassDescription.JDiffConstructor ctorDescription, Constructor<?> ctor);

    /**
     * Checks that the method found through reflection matches the
     * specification from the API xml file.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     */
    private void checkMethodCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        for (JDiffClassDescription.JDiffMethod method : classDescription.getMethods()) {
            try {

                Method m = ReflectionHelper.findMatchingMethod(runtimeClass, method);
                if (m == null) {
                    resultObserver.notifyFailure(FailureType.MISSING_METHOD,
                            method.toReadableString(classDescription.getAbsoluteClassName()),
                            "No method with correct signature found:" +
                                    method.toSignatureString());
                } else {
                    checkMethod(classDescription, runtimeClass, method, m);
                }
            } catch (Exception e) {
                LogHelper.loge("Got exception when checking method compliance", e);
                resultObserver.notifyFailure(FailureType.CAUGHT_EXCEPTION,
                        method.toReadableString(classDescription.getAbsoluteClassName()),
                        "Exception while checking method compliance!");
            }
        }
    }

    protected abstract void checkMethod(JDiffClassDescription classDescription,
            Class<?> runtimeClass,
            JDiffClassDescription.JDiffMethod methodDescription, Method method);
}
