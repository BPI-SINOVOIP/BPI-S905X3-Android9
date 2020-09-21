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
import java.lang.reflect.Executable;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class DexMemberChecker {

    public interface Observer {
        void classAccessible(boolean accessible, DexMember member);
        void fieldAccessibleViaReflection(boolean accessible, DexField field);
        void fieldAccessibleViaJni(boolean accessible, DexField field);
        void methodAccessibleViaReflection(boolean accessible, DexMethod method);
        void methodAccessibleViaJni(boolean accessible, DexMethod method);
    }

    public static void init() {
        System.loadLibrary("cts_dexchecker");
    }

    private static void call_VMDebug_allowHiddenApiReflectionFrom(Class<?> klass) throws Exception {
      Method method = null;

      try {
        Class<?> vmdebug = Class.forName("dalvik.system.VMDebug");
        method = vmdebug.getDeclaredMethod("allowHiddenApiReflectionFrom", Class.class);
      } catch (Exception ex) {
        // Could not find the method. Report the problem as a RuntimeException.
        throw new RuntimeException(ex);
      }

      try {
        method.invoke(null, klass);
      } catch (InvocationTargetException ex) {
        // Rethrow the original exception.
        Throwable cause = ex.getCause();
        // Please the compiler's 'throws' static analysis.
        if (cause instanceof Exception) {
          throw (Exception) cause;
        } else {
          throw (Error) cause;
        }
      }
    }

    public static boolean requestExemptionFromHiddenApiChecks() throws Exception {
      try {
        call_VMDebug_allowHiddenApiReflectionFrom(DexMemberChecker.class);
        return true;
      } catch (SecurityException ex) {
        return false;
      }
    }

    public static void checkSingleMember(DexMember dexMember, DexMemberChecker.Observer observer) {
        Class<?> klass = findClass(dexMember);
        if (klass == null) {
            // Class not found. Therefore its members are not visible.
            observer.classAccessible(false, dexMember);
            return;
        }
        observer.classAccessible(true, dexMember);

        if (dexMember instanceof DexField) {
            DexField field = (DexField) dexMember;
            observer.fieldAccessibleViaReflection(
                    hasMatchingField_Reflection(klass, field),
                    field);
            observer.fieldAccessibleViaJni(
                    hasMatchingField_JNI(klass, field),
                    field);
        } else if (dexMember instanceof DexMethod) {
            DexMethod method = (DexMethod) dexMember;
            observer.methodAccessibleViaReflection(
                    hasMatchingMethod_Reflection(klass, method),
                    method);
            observer.methodAccessibleViaJni(
                    hasMatchingMethod_JNI(klass, method),
                    method);
        } else {
            throw new IllegalStateException("Unexpected type of dex member");
        }
    }

    private static boolean typesMatch(Class<?>[] classes, List<String> typeNames) {
        if (classes.length != typeNames.size()) {
            return false;
        }
        for (int i = 0; i < classes.length; ++i) {
            if (!classes[i].getTypeName().equals(typeNames.get(i))) {
                return false;
            }
        }
        return true;
    }

    private static Class<?> findClass(DexMember dexMember) {
        try {
            return Class.forName(dexMember.getJavaClassName());
        } catch (ClassNotFoundException ex) {
            return null;
        }
    }

    private static boolean hasMatchingField_Reflection(Class<?> klass, DexField dexField) {
        try {
            klass.getDeclaredField(dexField.getName());
            return true;
        } catch (NoSuchFieldException ex) {
            return false;
        }
    }

    private static boolean hasMatchingField_JNI(Class<?> klass, DexField dexField) {
        Field ifield = getField_JNI(klass, dexField.getName(), dexField.getDexType());
        Field sfield = getStaticField_JNI(klass, dexField.getName(), dexField.getDexType());
        return (ifield != null && ifield.getDeclaringClass() == klass) ||
               (sfield != null && sfield.getDeclaringClass() == klass);
    }

    private static boolean hasMatchingMethod_Reflection(Class<?> klass, DexMethod dexMethod) {
        List<String> methodParams = dexMethod.getJavaParameterTypes();

        if (dexMethod.isConstructor()) {
            for (Constructor constructor : klass.getDeclaredConstructors()) {
                if (typesMatch(constructor.getParameterTypes(), methodParams)) {
                    return true;
                }
            }
        } else {
            String methodReturnType = dexMethod.getJavaType();
            for (Method method : klass.getDeclaredMethods()) {
                if (method.getName().equals(dexMethod.getName())
                        && method.getReturnType().getTypeName().equals(methodReturnType)
                        && typesMatch(method.getParameterTypes(), methodParams)) {
                    return true;
                }
            }
        }
        return false;
    }

    private static boolean hasMatchingMethod_JNI(Class<?> klass, DexMethod dexMethod) {
        Executable imethod = getMethod_JNI(klass, dexMethod.getName(), dexMethod.getDexSignature());
        Executable smethod = dexMethod.isConstructor() ? null :
             getStaticMethod_JNI(klass, dexMethod.getName(), dexMethod.getDexSignature());
        return (imethod != null && imethod.getDeclaringClass() == klass) ||
               (smethod != null && smethod.getDeclaringClass() == klass);
    }

    private static native Field getField_JNI(Class<?> klass, String name, String type);
    private static native Field getStaticField_JNI(Class<?> klass, String name, String type);
    private static native Executable getMethod_JNI(Class<?> klass, String name, String signature);
    private static native Executable getStaticMethod_JNI(Class<?> klass, String name,
            String signature);

}
