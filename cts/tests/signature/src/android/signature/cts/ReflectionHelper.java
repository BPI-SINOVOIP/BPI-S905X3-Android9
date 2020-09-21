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
import java.lang.reflect.GenericArrayType;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.lang.reflect.TypeVariable;
import java.lang.reflect.WildcardType;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Uses reflection to obtain runtime representations of elements in the API.
 */
public class ReflectionHelper {

    /**
     * Finds the reflected class for the class under test.
     *
     * @param classDescription the description of the class to find.
     * @return the reflected class, or null if not found.
     */
    @SuppressWarnings("unchecked")
    public static Class<?> findMatchingClass(JDiffClassDescription classDescription, ClassProvider classProvider)
            throws ClassNotFoundException {
        // even if there are no . in the string, split will return an
        // array of length 1
        String shortClassName = classDescription.getShortClassName();
        String[] classNameParts = shortClassName.split("\\.");
        String packageName = classDescription.getPackageName();
        String outermostClassName = packageName + "." + classNameParts[0];
        int firstInnerClassNameIndex = 0;

        return searchForClass(classProvider, classDescription.getAbsoluteClassName(),
                outermostClassName, classNameParts,
                firstInnerClassNameIndex);
    }

    private static Class<?> searchForClass(
            ClassProvider classProvider,
            String absoluteClassName,
            String outermostClassName, String[] classNameParts,
            int outerClassNameIndex) throws ClassNotFoundException {

        Class<?> clz = classProvider.getClass(outermostClassName);
        if (clz.getCanonicalName().equals(absoluteClassName)) {
            return clz;
        }

        // Then it must be an inner class.
        for (int x = outerClassNameIndex + 1; x < classNameParts.length; x++) {
            clz = findInnerClassByName(clz, classNameParts[x]);
            if (clz == null) {
                return null;
            }
            if (clz.getCanonicalName().equals(absoluteClassName)) {
                return clz;
            }
        }
        return null;
    }

    static Class<?> findMatchingClass(String absoluteClassName, ClassProvider classProvider)
            throws ClassNotFoundException {

        String[] classNameParts = absoluteClassName.split("\\.");
        StringBuilder builder = new StringBuilder();
        String separator = "";
        int start;
        for (start = 0; start < classNameParts.length; start++) {
            String classNamePart = classNameParts[start];
            builder.append(separator).append(classNamePart);
            separator = ".";
            if (Character.isUpperCase(classNamePart.charAt(0))) {
                break;
            }
        }
        String outermostClassName = builder.toString();

        return searchForClass(classProvider, absoluteClassName, outermostClassName, classNameParts,
                start);
    }

    /**
     * Searches the class for the specified inner class.
     *
     * @param clz the class to search in.
     * @param simpleName the simpleName of the class to find
     * @return the class being searched for, or null if it can't be found.
     */
    private static Class<?> findInnerClassByName(Class<?> clz, String simpleName) {
        for (Class<?> c : clz.getDeclaredClasses()) {
            if (c.getSimpleName().equals(simpleName)) {
                return c;
            }
        }
        return null;
    }

    /**
     * Searches available constructor.
     *
     * @param runtimeClass the class in which to search.
     * @param jdiffDes constructor description to find.
     * @return reflected constructor, or null if not found.
     */
    @SuppressWarnings("unchecked")
    static Constructor<?> findMatchingConstructor(Class<?> runtimeClass,
            JDiffClassDescription.JDiffConstructor jdiffDes) {
        for (Constructor<?> c : runtimeClass.getDeclaredConstructors()) {
            Type[] params = c.getGenericParameterTypes();
            boolean isStaticClass = ((runtimeClass.getModifiers() & Modifier.STATIC) != 0);

            int startParamOffset = 0;
            int numberOfParams = params.length;

            // non-static inner class -> skip implicit parent pointer
            // as first arg
            if (runtimeClass.isMemberClass() && !isStaticClass && params.length >= 1) {
                startParamOffset = 1;
                --numberOfParams;
            }

            ArrayList<String> jdiffParamList = jdiffDes.mParamList;
            if (jdiffParamList.size() == numberOfParams) {
                boolean isFound = true;
                // i counts jdiff params, j counts reflected params
                int i = 0;
                int j = startParamOffset;
                while (i < jdiffParamList.size()) {
                    if (!compareParam(jdiffParamList.get(i), params[j])) {
                        isFound = false;
                        break;
                    }
                    ++i;
                    ++j;
                }
                if (isFound) {
                    return c;
                }
            }
        }
        return null;
    }

    /**
     * Compares the parameter from the API and the parameter from
     * reflection.
     *
     * @param jdiffParam param parsed from the API xml file.
     * @param reflectionParamType param gotten from the Java reflection.
     * @return True if the two params match, otherwise return false.
     */
    private static boolean compareParam(String jdiffParam, Type reflectionParamType) {
        if (jdiffParam == null) {
            return false;
        }

        String reflectionParam = typeToString(reflectionParamType);
        // Most things aren't varargs, so just do a simple compare
        // first.
        if (jdiffParam.equals(reflectionParam)) {
            return true;
        }

        // Check for varargs.  jdiff reports varargs as ..., while
        // reflection reports them as []
        int jdiffParamEndOffset = jdiffParam.indexOf("...");
        int reflectionParamEndOffset = reflectionParam != null ? reflectionParam.indexOf("[]") : -1;
        if (jdiffParamEndOffset != -1 && reflectionParamEndOffset != -1) {
            jdiffParam = jdiffParam.substring(0, jdiffParamEndOffset);
            reflectionParam = reflectionParam.substring(0, reflectionParamEndOffset);
            return jdiffParam.equals(reflectionParam);
        }

        return false;
    }

    /**
     * Finds the reflected method specified by the method description.
     *
     * @param runtimeClass the class in which to search.
     * @param method description of the method to find
     * @return the reflected method, or null if not found.
     */
    @SuppressWarnings("unchecked")
    static Method findMatchingMethod(Class<?> runtimeClass,
            JDiffClassDescription.JDiffMethod method) {
        Method[] methods = runtimeClass.getDeclaredMethods();

        for (Method m : methods) {
            if (matches(method, m)) {
                return m;
            }
        }

        return null;
    }

    /**
     * Checks if the two types of methods are the same.
     *
     * @param jDiffMethod the jDiffMethod to compare
     * @param reflectedMethod the reflected method to compare
     * @return true, if both methods are the same
     */
    static boolean matches(JDiffClassDescription.JDiffMethod jDiffMethod,
            Method reflectedMethod) {
        // If the method names aren't equal, the methods can't match.
        if (!jDiffMethod.mName.equals(reflectedMethod.getName())) {
            return false;
        }
        String jdiffReturnType = jDiffMethod.mReturnType;
        String reflectionReturnType = typeToString(reflectedMethod.getGenericReturnType());
        List<String> jdiffParamList = jDiffMethod.mParamList;

        // Next, compare the return types of the two methods.  If
        // they aren't equal, the methods can't match.
        if (!jdiffReturnType.equals(reflectionReturnType)) {
            return false;
        }

        Type[] params = reflectedMethod.getGenericParameterTypes();

        // Next, check the method parameters.  If they have different
        // parameter lengths, the two methods can't match.
        if (jdiffParamList.size() != params.length) {
            return false;
        }

        boolean piecewiseParamsMatch = true;

        // Compare method parameters piecewise and return true if they all match.
        for (int i = 0; i < jdiffParamList.size(); i++) {
            piecewiseParamsMatch &= compareParam(jdiffParamList.get(i), params[i]);
        }
        if (piecewiseParamsMatch) {
            return true;
        }

        /* NOTE: There are cases where piecewise method parameter checking
         * fails even though the strings are equal, so compare entire strings
         * against each other. This is not done by default to avoid a
         * TransactionTooLargeException.
         * Additionally, this can fail anyway due to extra
         * information dug up by reflection.
         *
         * TODO: fix parameter equality checking and reflection matching
         * See https://b.corp.google.com/issues/27726349
         */

        StringBuilder reflectedMethodParams = new StringBuilder("");
        StringBuilder jdiffMethodParams = new StringBuilder("");

        for (int i = 0; i < jdiffParamList.size(); i++) {
            jdiffMethodParams.append(jdiffParamList.get(i));
            reflectedMethodParams.append(params[i]);
        }

        String jDiffFName = jdiffMethodParams.toString();
        String refName = reflectedMethodParams.toString();

        return jDiffFName.equals(refName);
    }

    /**
     * Converts WildcardType array into a jdiff compatible string..
     * This is a helper function for typeToString.
     *
     * @param types array of types to format.
     * @return the jdiff formatted string.
     */
    private static String concatWildcardTypes(Type[] types) {
        StringBuilder sb = new StringBuilder();
        int elementNum = 0;
        for (Type t : types) {
            sb.append(typeToString(t));
            if (++elementNum < types.length) {
                sb.append(" & ");
            }
        }
        return sb.toString();
    }

    /**
     * Converts a Type into a jdiff compatible String.  The returned
     * types from this function should match the same Strings that
     * jdiff is providing to us.
     *
     * @param type the type to convert.
     * @return the jdiff formatted string.
     */
    private static String typeToString(Type type) {
        if (type instanceof ParameterizedType) {
            ParameterizedType pt = (ParameterizedType) type;

            StringBuilder sb = new StringBuilder();
            sb.append(typeToString(pt.getRawType()));
            sb.append("<");

            int elementNum = 0;
            Type[] types = pt.getActualTypeArguments();
            for (Type t : types) {
                sb.append(typeToString(t));
                if (++elementNum < types.length) {
                    sb.append(", ");
                }
            }

            sb.append(">");
            return sb.toString();
        } else if (type instanceof TypeVariable) {
            return ((TypeVariable<?>) type).getName();
        } else if (type instanceof Class) {
            return ((Class<?>) type).getCanonicalName();
        } else if (type instanceof GenericArrayType) {
            String typeName = typeToString(((GenericArrayType) type).getGenericComponentType());
            return typeName + "[]";
        } else if (type instanceof WildcardType) {
            WildcardType wt = (WildcardType) type;
            Type[] lowerBounds = wt.getLowerBounds();
            if (lowerBounds.length == 0) {
                String name = "? extends " + concatWildcardTypes(wt.getUpperBounds());

                // Special case for ?
                if (name.equals("? extends java.lang.Object")) {
                    return "?";
                } else {
                    return name;
                }
            } else {
                String name = concatWildcardTypes(wt.getUpperBounds()) +
                        " super " +
                        concatWildcardTypes(wt.getLowerBounds());
                // Another special case for ?
                name = name.replace("java.lang.Object", "?");
                return name;
            }
        } else {
            throw new RuntimeException("Got an unknown java.lang.Type");
        }
    }

    /**
     * Returns a Class representing an annotation type of the given name.
     */
    @SuppressWarnings("unchecked")
    public static Class<? extends Annotation> getAnnotationClass(String name) {
        try {
            Class<?> clazz = Class.forName(
                    name, false, ReflectionHelper.class.getClassLoader());
            if (clazz.isAnnotation()) {
                return (Class<? extends Annotation>) clazz;
            } else {
                return null;
            }
        } catch (ClassNotFoundException e) {
            return null;
        }
    }

    /**
     * Returns a list of constructors which are annotated with the given annotation class.
     */
    public static Set<Constructor<?>> getAnnotatedConstructors(Class<?> clazz,
            Class<? extends Annotation> annotation) {
        Set<Constructor<?>> result = new HashSet<>();
        if (annotation != null) {
            for (Constructor<?> c : clazz.getDeclaredConstructors()) {
                if (c.isAnnotationPresent(annotation)) {
                    // TODO(b/71630695): currently, some API members are not annotated, because
                    // a member is automatically added to the API set if it is in a class with
                    // annotation and it is not @hide. <member>.getDeclaringClass().
                    // isAnnotationPresent(annotationClass) won't help because it will then
                    // incorrectly include non-API members which are marked as @hide;
                    // @hide isn't visible at runtime. Until the issue is fixed, we should
                    // omit those automatically added API members from the test.
                    result.add(c);
                }
            }
        }
        return result;
    }

    /**
     * Returns a list of methods which are annotated with the given annotation class.
     */
    public static Set<Method> getAnnotatedMethods(Class<?> clazz,
            Class<? extends Annotation> annotation) {
        Set<Method> result = new HashSet<>();
        if (annotation != null) {
            for (Method m : clazz.getDeclaredMethods()) {
                if (m.isAnnotationPresent(annotation)) {
                    // TODO(b/71630695): see getAnnotatedConstructors for details
                    result.add(m);
                }
            }
        }
        return result;
    }

    /**
     * Returns a list of fields which are annotated with the given annotation class.
     */
    public static Set<Field> getAnnotatedFields(Class<?> clazz,
            Class<? extends Annotation> annotation) {
        Set<Field> result = new HashSet<>();
        if (annotation != null) {
            for (Field f : clazz.getDeclaredFields()) {
                if (f.isAnnotationPresent(annotation)) {
                    // TODO(b/71630695): see getAnnotatedConstructors for details
                    result.add(f);
                }
            }
        }
        return result;
    }

    private static boolean isInAnnotatedClass(Member m,
            Class<? extends Annotation> annotationClass) {
        Class<?> clazz = m.getDeclaringClass();
        do {
            if (clazz.isAnnotationPresent(annotationClass)) {
                return true;
            }
        } while ((clazz = clazz.getDeclaringClass()) != null);
        return false;
    }

    public static boolean isAnnotatedOrInAnnotatedClass(Field field,
            Class<? extends Annotation> annotationClass) {
        if (annotationClass == null) {
            return true;
        }
        return field.isAnnotationPresent(annotationClass)
                || isInAnnotatedClass(field, annotationClass);
    }

    public static boolean isAnnotatedOrInAnnotatedClass(Constructor<?> constructor,
            Class<? extends Annotation> annotationClass) {
        if (annotationClass == null) {
            return true;
        }
        return constructor.isAnnotationPresent(annotationClass)
                || isInAnnotatedClass(constructor, annotationClass);
    }

    public static boolean isAnnotatedOrInAnnotatedClass(Method method,
            Class<? extends Annotation> annotationClass) {
        if (annotationClass == null) {
            return true;
        }
        return method.isAnnotationPresent(annotationClass)
                || isInAnnotatedClass(method, annotationClass);
    }

    public static boolean isOverridingAnnotatedMethod(Method method,
            Class<? extends Annotation> annotationClass) {
        Class<?> clazz = method.getDeclaringClass();
        while (!(clazz = clazz.getSuperclass()).equals(Object.class)) {
            try {
                Method overriddenMethod;
                overriddenMethod = clazz.getDeclaredMethod(method.getName(),
                        method.getParameterTypes());
                if (overriddenMethod != null) {
                    return isAnnotatedOrInAnnotatedClass(overriddenMethod, annotationClass);
                }
            } catch (NoSuchMethodException e) {
                continue;
            } catch (SecurityException e) {
                throw new RuntimeException(
                        "Error while searching for overridden method. " + method.toString(), e);
            }
        }
        return false;
    }

    static Class<?> findRequiredClass(JDiffClassDescription classDescription,
            ClassProvider classProvider) {
        try {
            return findMatchingClass(classDescription, classProvider);
        } catch (ClassNotFoundException e) {
            LogHelper.loge("ClassNotFoundException for " + classDescription.getAbsoluteClassName(), e);
            return null;
        }
    }
}
