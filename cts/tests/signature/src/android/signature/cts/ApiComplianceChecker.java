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

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Type;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

/**
 * Checks that the runtime representation of a class matches the API representation of a class.
 */
public class ApiComplianceChecker extends AbstractApiChecker {

    /** Indicates that the class is an annotation. */
    private static final int CLASS_MODIFIER_ANNOTATION = 0x00002000;

    /** Indicates that the class is an enum. */
    private static final int CLASS_MODIFIER_ENUM       = 0x00004000;

    /** Indicates that the method is a bridge method. */
    private static final int METHOD_MODIFIER_BRIDGE    = 0x00000040;

    /** Indicates that the method is takes a variable number of arguments. */
    private static final int METHOD_MODIFIER_VAR_ARGS  = 0x00000080;

    /** Indicates that the method is a synthetic method. */
    private static final int METHOD_MODIFIER_SYNTHETIC = 0x00001000;

    private final InterfaceChecker interfaceChecker;

    public ApiComplianceChecker(ResultObserver resultObserver, ClassProvider classProvider) {
        super(classProvider, resultObserver);
        interfaceChecker = new InterfaceChecker(resultObserver, classProvider);
    }

    @Override
    public void checkDeferred() {
        interfaceChecker.checkQueued();
    }

    @Override
    protected boolean checkClass(JDiffClassDescription classDescription, Class<?> runtimeClass) {
        if (JDiffClassDescription.JDiffType.INTERFACE.equals(classDescription.getClassType())) {
            // Queue the interface for deferred checking.
            interfaceChecker.queueForDeferredCheck(classDescription, runtimeClass);
        }

        String reason;
        if ((reason = checkClassModifiersCompliance(classDescription, runtimeClass)) != null) {
            resultObserver.notifyFailure(FailureType.mismatch(classDescription),
                    classDescription.getAbsoluteClassName(),
                    String.format("Non-compatible class found when looking for %s - because %s",
                            classDescription.toSignatureString(), reason));
            return false;
        }

        if (!checkClassAnnotationCompliance(classDescription, runtimeClass)) {
            resultObserver.notifyFailure(FailureType.mismatch(classDescription),
                    classDescription.getAbsoluteClassName(), "Annotation mismatch");
            return false;
        }

        if (!runtimeClass.isAnnotation()) {
            // check father class
            if (!checkClassExtendsCompliance(classDescription, runtimeClass)) {
                resultObserver.notifyFailure(FailureType.mismatch(classDescription),
                        classDescription.getAbsoluteClassName(), "Extends mismatch");
                return false;
            }

            // check implements interface
            if (!checkClassImplementsCompliance(classDescription, runtimeClass)) {
                resultObserver.notifyFailure(FailureType.mismatch(classDescription),
                        classDescription.getAbsoluteClassName(), "Implements mismatch");
                return false;
            }
        }
        return true;
    }

    /**
     * Checks if the class under test has compliant modifiers compared to the API.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     * @return null if modifiers are compliant otherwise a reason why they are not.
     */
    private static String checkClassModifiersCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        int reflectionModifiers = runtimeClass.getModifiers();
        int apiModifiers = classDescription.getModifier();

        // If the api class isn't abstract
        if (((apiModifiers & Modifier.ABSTRACT) == 0) &&
                // but the reflected class is
                ((reflectionModifiers & Modifier.ABSTRACT) != 0) &&
                // and it isn't an enum
                !classDescription.isEnumType()) {
            // that is a problem
            return "description is abstract but class is not and is not an enum";
        }
        // ABSTRACT check passed, so mask off ABSTRACT
        reflectionModifiers &= ~Modifier.ABSTRACT;
        apiModifiers &= ~Modifier.ABSTRACT;

        if (classDescription.isAnnotation()) {
            reflectionModifiers &= ~CLASS_MODIFIER_ANNOTATION;
        }
        if (runtimeClass.isInterface()) {
            reflectionModifiers &= ~(Modifier.INTERFACE);
        }
        if (classDescription.isEnumType() && runtimeClass.isEnum()) {
            reflectionModifiers &= ~CLASS_MODIFIER_ENUM;
        }

        if ((reflectionModifiers == apiModifiers)
                && (classDescription.isEnumType() == runtimeClass.isEnum())) {
            return null;
        } else {
            return String.format("modifier mismatch - description (%s), class (%s)",
                    Modifier.toString(apiModifiers), Modifier.toString(reflectionModifiers));
        }
    }

    /**
     * Checks if the class under test is compliant with regards to
     * annnotations when compared to the API.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     * @return true if the class is compliant
     */
    private static boolean checkClassAnnotationCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        if (runtimeClass.isAnnotation()) {
            // check annotation
            for (String inter : classDescription.getImplInterfaces()) {
                if ("java.lang.annotation.Annotation".equals(inter)) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }

    /**
     * Checks if the class under test extends the proper classes
     * according to the API.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     * @return true if the class is compliant.
     */
    private static boolean checkClassExtendsCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        // Nothing to check if it doesn't extend anything.
        if (classDescription.getExtendedClass() != null) {
            Class<?> superClass = runtimeClass.getSuperclass();

            while (superClass != null) {
                if (superClass.getCanonicalName().equals(classDescription.getExtendedClass())) {
                    return true;
                }
                superClass = superClass.getSuperclass();
            }
            // Couldn't find a matching superclass.
            return false;
        }
        return true;
    }

    /**
     * Checks if the class under test implements the proper interfaces
     * according to the API.
     *
     * @param classDescription a description of a class in an API.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     * @return true if the class is compliant
     */
    private static boolean checkClassImplementsCompliance(JDiffClassDescription classDescription,
            Class<?> runtimeClass) {
        Set<String> interFaceSet = new HashSet<>();

        addInterfacesToSetByName(runtimeClass, interFaceSet);

        for (String inter : classDescription.getImplInterfaces()) {
            if (!interFaceSet.contains(inter)) {
                return false;
            }
        }
        return true;
    }

    private static void addInterfacesToSetByName(Class<?> runtimeClass, Set<String> interFaceSet) {
        Class<?>[] interfaces = runtimeClass.getInterfaces();
        for (Class<?> c : interfaces) {
            interFaceSet.add(c.getCanonicalName());
        }

        // Add the interfaces that the super class implements as well just in case the super class
        // is hidden.
        Class<?> superClass = runtimeClass.getSuperclass();
        if (superClass != null) {
            addInterfacesToSetByName(superClass, interFaceSet);
        }
    }

    @Override
    protected void checkField(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffField fieldDescription, Field field) {
        if (field.getModifiers() != fieldDescription.mModifier) {
            resultObserver.notifyFailure(FailureType.MISMATCH_FIELD,
                    fieldDescription.toReadableString(classDescription.getAbsoluteClassName()),
                    "Non-compatible field modifiers found when looking for " +
                            fieldDescription.toSignatureString());
        } else if (!checkFieldValueCompliance(fieldDescription, field)) {
            resultObserver.notifyFailure(FailureType.MISMATCH_FIELD,
                    fieldDescription.toReadableString(classDescription.getAbsoluteClassName()),
                    "Incorrect field value found when looking for " +
                            fieldDescription.toSignatureString());
        } else if (!field.getType().getCanonicalName().equals(fieldDescription.mFieldType)) {
            // type name does not match, but this might be a generic
            String genericTypeName = null;
            Type type = field.getGenericType();
            if (type != null) {
                genericTypeName = type instanceof Class ? ((Class) type).getName() :
                        type.toString().replace('$', '.');
            }
            if (genericTypeName == null || !genericTypeName.equals(fieldDescription.mFieldType)) {
                resultObserver.notifyFailure(
                        FailureType.MISMATCH_FIELD,
                        fieldDescription.toReadableString(classDescription.getAbsoluteClassName()),
                        "Non-compatible field type found when looking for " +
                                fieldDescription.toSignatureString());
            }
        }
    }

    /**
     * Checks whether the field values are compatible.
     *
     * @param apiField The field as defined by the platform API.
     * @param deviceField The field as defined by the device under test.
     */
    private static boolean checkFieldValueCompliance(JDiffClassDescription.JDiffField apiField, Field deviceField) {
        if ((apiField.mModifier & Modifier.FINAL) == 0 ||
                (apiField.mModifier & Modifier.STATIC) == 0) {
            // Only final static fields can have fixed values.
            return true;
        }
        if (apiField.getValueString() == null) {
            // If we don't define a constant value for it, then it can be anything.
            return true;
        }
        // Some fields may be protected or package-private
        deviceField.setAccessible(true);
        try {
            switch (apiField.mFieldType) {
                case "byte":
                    return Objects.equals(apiField.getValueString(),
                            Byte.toString(deviceField.getByte(null)));
                case "char":
                    return Objects.equals(apiField.getValueString(),
                            Integer.toString(deviceField.getChar(null)));
                case "short":
                    return Objects.equals(apiField.getValueString(),
                            Short.toString(deviceField.getShort(null)));
                case "int":
                    return Objects.equals(apiField.getValueString(),
                            Integer.toString(deviceField.getInt(null)));
                case "long":
                    return Objects.equals(apiField.getValueString(),
                            Long.toString(deviceField.getLong(null)) + "L");
                case "float":
                    return Objects.equals(apiField.getValueString(),
                            canonicalizeFloatingPoint(
                                    Float.toString(deviceField.getFloat(null)), "f"));
                case "double":
                    return Objects.equals(apiField.getValueString(),
                            canonicalizeFloatingPoint(
                                    Double.toString(deviceField.getDouble(null)), ""));
                case "boolean":
                    return Objects.equals(apiField.getValueString(),
                            Boolean.toString(deviceField.getBoolean(null)));
                case "java.lang.String":
                    String value = apiField.getValueString();
                    // Remove the quotes the value string is wrapped in
                    value = unescapeFieldStringValue(value.substring(1, value.length() - 1));
                    return Objects.equals(value, deviceField.get(null));
                default:
                    return true;
            }
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Canonicalize the string representation of floating point numbers.
     *
     * This needs to be kept in sync with the doclava canonicalization.
     */
    private static String canonicalizeFloatingPoint(String val, String suffix) {
        switch (val) {
            case "Infinity":
                return "(1.0" + suffix + "/0.0" + suffix + ")";
            case "-Infinity":
                return "(-1.0" + suffix + "/0.0" + suffix + ")";
            case "NaN":
                return "(0.0" + suffix + "/0.0" + suffix + ")";
        }

        if (val.indexOf('E') != -1) {
            return val + suffix;
        }

        // 1.0 is the only case where a trailing "0" is allowed.
        // 1.00 is canonicalized as 1.0.
        int i = val.length() - 1;
        int d = val.indexOf('.');
        while (i >= d + 2 && val.charAt(i) == '0') {
            val = val.substring(0, i--);
        }
        return val + suffix;
    }

    // This unescapes the string format used by doclava and so needs to be kept in sync with any
    // changes made to that format.
    private static String unescapeFieldStringValue(String str) {
        final int N = str.length();

        // If there's no special encoding strings in the string then just return it.
        if (str.indexOf('\\') == -1) {
            return str;
        }

        final StringBuilder buf = new StringBuilder(str.length());
        char escaped = 0;
        final int START = 0;
        final int CHAR1 = 1;
        final int CHAR2 = 2;
        final int CHAR3 = 3;
        final int CHAR4 = 4;
        final int ESCAPE = 5;
        int state = START;

        for (int i = 0; i < N; i++) {
            final char c = str.charAt(i);
            switch (state) {
                case START:
                    if (c == '\\') {
                        state = ESCAPE;
                    } else {
                        buf.append(c);
                    }
                    break;
                case ESCAPE:
                    switch (c) {
                        case '\\':
                            buf.append('\\');
                            state = START;
                            break;
                        case 't':
                            buf.append('\t');
                            state = START;
                            break;
                        case 'b':
                            buf.append('\b');
                            state = START;
                            break;
                        case 'r':
                            buf.append('\r');
                            state = START;
                            break;
                        case 'n':
                            buf.append('\n');
                            state = START;
                            break;
                        case 'f':
                            buf.append('\f');
                            state = START;
                            break;
                        case '\'':
                            buf.append('\'');
                            state = START;
                            break;
                        case '\"':
                            buf.append('\"');
                            state = START;
                            break;
                        case 'u':
                            state = CHAR1;
                            escaped = 0;
                            break;
                    }
                    break;
                case CHAR1:
                case CHAR2:
                case CHAR3:
                case CHAR4:
                    escaped <<= 4;
                    if (c >= '0' && c <= '9') {
                        escaped |= c - '0';
                    } else if (c >= 'a' && c <= 'f') {
                        escaped |= 10 + (c - 'a');
                    } else if (c >= 'A' && c <= 'F') {
                        escaped |= 10 + (c - 'A');
                    } else {
                        throw new RuntimeException(
                                "bad escape sequence: '" + c + "' at pos " + i + " in: \""
                                        + str + "\"");
                    }
                    if (state == CHAR4) {
                        buf.append(escaped);
                        state = START;
                    } else {
                        state++;
                    }
                    break;
            }
        }
        if (state != START) {
            throw new RuntimeException("unfinished escape sequence: " + str);
        }
        return buf.toString();
    }

    @Override
    protected void checkConstructor(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffConstructor ctorDescription, Constructor<?> ctor) {
        if (ctor.isVarArgs()) {// some method's parameter are variable args
            ctorDescription.mModifier |= METHOD_MODIFIER_VAR_ARGS;
        }
        if (ctor.getModifiers() != ctorDescription.mModifier) {
            resultObserver.notifyFailure(
                    FailureType.MISMATCH_METHOD,
                    ctorDescription.toReadableString(classDescription.getAbsoluteClassName()),
                    "Non-compatible method found when looking for " +
                            ctorDescription.toSignatureString());
        }
    }

    @Override
    protected void checkMethod(JDiffClassDescription classDescription, Class<?> runtimeClass,
            JDiffClassDescription.JDiffMethod methodDescription, Method method) {
        if (method.isVarArgs()) {
            methodDescription.mModifier |= METHOD_MODIFIER_VAR_ARGS;
        }
        if (method.isBridge()) {
            methodDescription.mModifier |= METHOD_MODIFIER_BRIDGE;
        }
        if (method.isSynthetic()) {
            methodDescription.mModifier |= METHOD_MODIFIER_SYNTHETIC;
        }

        // FIXME: A workaround to fix the final mismatch on enumeration
        if (runtimeClass.isEnum() && methodDescription.mName.equals("values")) {
            return;
        }

        String reason;
        if ((reason = areMethodsModifiedCompatible(
                classDescription, methodDescription, method)) != null) {
            resultObserver.notifyFailure(FailureType.MISMATCH_METHOD,
                    methodDescription.toReadableString(classDescription.getAbsoluteClassName()),
                    String.format("Non-compatible method found when looking for %s - because %s",
                            methodDescription.toSignatureString(), reason));
        }
    }

    /**
     * Checks to ensure that the modifiers value for two methods are compatible.
     *
     * Allowable differences are:
     *   - synchronized is allowed to be removed from an apiMethod
     *     that has it
     *   - the native modified is ignored
     *
     * @param classDescription a description of a class in an API.
     * @param apiMethod the method read from the api file.
     * @param reflectedMethod the method found via reflection.
     * @return null if the method modifiers are compatible otherwise the reason why not.
     */
    private static String areMethodsModifiedCompatible(
            JDiffClassDescription classDescription,
            JDiffClassDescription.JDiffMethod apiMethod,
            Method reflectedMethod) {

        // If the apiMethod isn't synchronized
        if (((apiMethod.mModifier & Modifier.SYNCHRONIZED) == 0) &&
                // but the reflected method is
                ((reflectedMethod.getModifiers() & Modifier.SYNCHRONIZED) != 0)) {
            // that is a problem
            return "description is synchronize but class is not";
        }

        // Mask off NATIVE since it is a don't care.  Also mask off
        // SYNCHRONIZED since we've already handled that check.
        int ignoredMods = (Modifier.NATIVE | Modifier.SYNCHRONIZED | Modifier.STRICT);
        int reflectionModifiers = reflectedMethod.getModifiers() & ~ignoredMods;
        int apiModifiers = apiMethod.mModifier & ~ignoredMods;

        // We can ignore FINAL for classes
        if ((classDescription.getModifier() & Modifier.FINAL) != 0) {
            reflectionModifiers &= ~Modifier.FINAL;
            apiModifiers &= ~Modifier.FINAL;
        }

        if (reflectionModifiers == apiModifiers) {
            return null;
        } else {
            return String.format("modifier mismatch - description (%s), method (%s)",
                    Modifier.toString(apiModifiers), Modifier.toString(reflectionModifiers));
        }
    }

    public void addBaseClass(JDiffClassDescription classDescription) {
        // Keep track of all the base interfaces that may by extended.
        if (classDescription.getClassType() == JDiffClassDescription.JDiffType.INTERFACE) {
            try {
                Class<?> runtimeClass =
                        ReflectionHelper.findMatchingClass(classDescription, classProvider);
                interfaceChecker.queueForDeferredCheck(classDescription, runtimeClass);
            } catch (ClassNotFoundException e) {
                // Do nothing.
            }
        }
    }
}
