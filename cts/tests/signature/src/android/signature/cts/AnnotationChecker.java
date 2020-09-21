/*
 * Copyright (C) 2017 The Android Open Source Project
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

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Checks that the runtime representation of a class matches the API representation of a class.
 */
public class AnnotationChecker extends AbstractApiChecker {

    private final Class<? extends Annotation> annotationClass;

    private final Map<String, Class<?>> annotatedClassesMap = new HashMap<>();
    private final Map<String, Set<Constructor<?>>> annotatedConstructorsMap = new HashMap<>();
    private final Map<String, Set<Method>> annotatedMethodsMap = new HashMap<>();
    private final Map<String, Set<Field>> annotatedFieldsMap = new HashMap<>();

    /**
     * @param annotationName name of the annotation class for the API type (e.g.
     *      android.annotation.SystemApi)
     */
    public AnnotationChecker(
            ResultObserver resultObserver, ClassProvider classProvider, String annotationName) {
        super(classProvider, resultObserver);

        annotationClass = ReflectionHelper.getAnnotationClass(annotationName);
        classProvider.getAllClasses().forEach(clazz -> {
            if (clazz.isAnnotationPresent(annotationClass)) {
                annotatedClassesMap.put(clazz.getName(), clazz);
            }
            Set<Constructor<?>> constructors = ReflectionHelper.getAnnotatedConstructors(clazz,
                    annotationClass);
            if (!constructors.isEmpty()) {
                annotatedConstructorsMap.put(clazz.getName(), constructors);
            }
            Set<Method> methods = ReflectionHelper.getAnnotatedMethods(clazz, annotationClass);
            if (!methods.isEmpty()) {
                annotatedMethodsMap.put(clazz.getName(), methods);
            }
            Set<Field> fields = ReflectionHelper.getAnnotatedFields(clazz, annotationClass);
            if (!fields.isEmpty()) {
                annotatedFieldsMap.put(clazz.getName(), fields);
            }
        });
    }

    @Override
    public void checkDeferred() {
        for (Class<?> clazz : annotatedClassesMap.values()) {
            resultObserver.notifyFailure(FailureType.EXTRA_CLASS, clazz.getName(),
                    "Class annotated with " + annotationClass.getName()
                            + " does not exist in the documented API");
        }
        for (Set<Constructor<?>> set : annotatedConstructorsMap.values()) {
            for (Constructor<?> c : set) {
                resultObserver.notifyFailure(FailureType.EXTRA_METHOD, c.toString(),
                        "Constructor annotated with " + annotationClass.getName()
                                + " does not exist in the API");
            }
        }
        for (Set<Method> set : annotatedMethodsMap.values()) {
            for (Method m : set) {
                resultObserver.notifyFailure(FailureType.EXTRA_METHOD, m.toString(),
                        "Method annotated with " + annotationClass.getName()
                                + " does not exist in the API");
            }
        }
        for (Set<Field> set : annotatedFieldsMap.values()) {
            for (Field f : set) {
                resultObserver.notifyFailure(FailureType.EXTRA_FIELD, f.toString(),
                        "Field annotated with " + annotationClass.getName()
                                + " does not exist in the API");
            }
        }
    }

    @Override
    protected boolean checkClass(JDiffClassDescription classDescription, Class<?> runtimeClass) {
        // remove the class from the set if found
        annotatedClassesMap.remove(runtimeClass.getName());
        return true;
    }

    @Override
    protected void checkField(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffField fieldDescription, Field field) {
        // make sure that the field (or its declaring class) is annotated
        if (!ReflectionHelper.isAnnotatedOrInAnnotatedClass(field, annotationClass)) {
            resultObserver.notifyFailure(FailureType.MISSING_ANNOTATION,
                    field.toString(),
                    "Annotation " + annotationClass.getName() + " is missing");
        }

        // remove it from the set if found in the API doc
        Set<Field> annotatedFields = annotatedFieldsMap.get(runtimeClass.getName());
        if (annotatedFields != null) {
            annotatedFields.remove(field);
        }
    }

    @Override
    protected void checkConstructor(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffConstructor ctorDescription, Constructor<?> ctor) {
        Set<Constructor<?>> annotatedConstructors = annotatedConstructorsMap
                .get(runtimeClass.getName());

        // make sure that the constructor (or its declaring class) is annotated
        if (annotationClass != null) {
            if (!ReflectionHelper.isAnnotatedOrInAnnotatedClass(ctor, annotationClass)) {
                resultObserver.notifyFailure(FailureType.MISSING_ANNOTATION,
                        ctor.toString(),
                        "Annotation " + annotationClass.getName() + " is missing");
            }
        }

        // remove it from the set if found in the API doc
        if (annotatedConstructors != null) {
            annotatedConstructors.remove(ctor);
        }
    }

    @Override
    protected void checkMethod(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffMethod methodDescription, Method method) {
        // make sure that the method (or its declaring class) is annotated or overriding
        // annotated method.
        if (!ReflectionHelper.isAnnotatedOrInAnnotatedClass(method, annotationClass)
                && !ReflectionHelper.isOverridingAnnotatedMethod(method,
                        annotationClass)) {
            resultObserver.notifyFailure(FailureType.MISSING_ANNOTATION,
                    method.toString(),
                    "Annotation " + annotationClass.getName() + " is missing");
        }

        // remove it from the set if found in the API doc
        Set<Method> annotatedMethods = annotatedMethodsMap.get(runtimeClass.getName());
        if (annotatedMethods != null) {
            annotatedMethods.remove(method);
        }
    }
}
