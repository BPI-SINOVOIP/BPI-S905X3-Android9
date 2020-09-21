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

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Checks that the runtime representation of the interfaces match the API definition.
 *
 * <p>Interfaces are treated differently to other classes. Whereas other classes are checked by
 * making sure that every member in the API is accessible through reflection. Interfaces are
 * checked to make sure that every method visible through reflection is defined in the API. The
 * reason for this difference is to ensure that no additional methods have been added to interfaces
 * that are expected to be implemented by Android developers because that would break backwards
 * compatibility.
 *
 * TODO(b/71886491): This also potentially applies to abstract classes that the App developers are
 * expected to extend.
 */
class InterfaceChecker {

    private static final Set<String> HIDDEN_INTERFACE_METHOD_WHITELIST = new HashSet<>();
    static {
        // Interfaces that define @hide or @SystemApi or @TestApi methods will by definition contain
        // methods that do not appear in current.txt. Interfaces added to this
        // list are probably not meant to be implemented in an application.
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract boolean android.companion.DeviceFilter.matches(D)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public static <D> boolean android.companion.DeviceFilter.matches(android.companion.DeviceFilter<D>,D)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract java.lang.String android.companion.DeviceFilter.getDeviceDisplayName(D)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract int android.companion.DeviceFilter.getMediumType()");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract void android.nfc.tech.TagTechnology.reconnect() throws java.io.IOException");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract void android.os.IBinder.shellCommand(java.io.FileDescriptor,java.io.FileDescriptor,java.io.FileDescriptor,java.lang.String[],android.os.ShellCallback,android.os.ResultReceiver) throws android.os.RemoteException");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract int android.text.ParcelableSpan.getSpanTypeIdInternal()");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract void android.text.ParcelableSpan.writeToParcelInternal(android.os.Parcel,int)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract void android.view.WindowManager.requestAppKeyboardShortcuts(android.view.WindowManager$KeyboardShortcutsReceiver,int)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract boolean javax.microedition.khronos.egl.EGL10.eglReleaseThread()");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract void org.w3c.dom.ls.LSSerializer.setFilter(org.w3c.dom.ls.LSSerializerFilter)");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract org.w3c.dom.ls.LSSerializerFilter org.w3c.dom.ls.LSSerializer.getFilter()");
        HIDDEN_INTERFACE_METHOD_WHITELIST.add("public abstract android.graphics.Region android.view.WindowManager.getCurrentImeTouchRegion()");
    }

    private final ResultObserver resultObserver;

    private final Map<Class<?>, JDiffClassDescription> class2Description =
            new TreeMap<>(Comparator.comparing(Class::getName));

    private final ClassProvider classProvider;

    InterfaceChecker(ResultObserver resultObserver, ClassProvider classProvider) {
        this.resultObserver = resultObserver;
        this.classProvider = classProvider;
    }

    public void checkQueued() {
        for (Map.Entry<Class<?>, JDiffClassDescription> entry : class2Description.entrySet()) {
            Class<?> runtimeClass = entry.getKey();
            JDiffClassDescription classDescription = entry.getValue();
            List<Method> methods = checkInterfaceMethodCompliance(classDescription, runtimeClass);
            if (methods.size() > 0) {
                resultObserver.notifyFailure(FailureType.MISMATCH_INTERFACE_METHOD,
                        classDescription.getAbsoluteClassName(), "Interfaces cannot be modified: "
                                + classDescription.getAbsoluteClassName() + ": " + methods);
            }
        }
    }

    private static <T> Predicate<T> not(Predicate<T> predicate) {
        return predicate.negate();
    }

    /**
     * Validate that an interfaces method count is as expected.
     *
     * @param classDescription the class's API description.
     * @param runtimeClass the runtime class corresponding to {@code classDescription}.
     */
    private List<Method> checkInterfaceMethodCompliance(
            JDiffClassDescription classDescription, Class<?> runtimeClass) {

        return Stream.of(runtimeClass.getDeclaredMethods())
                .filter(not(Method::isDefault))
                .filter(not(Method::isSynthetic))
                .filter(not(Method::isBridge))
                .filter(m -> !Modifier.isStatic(m.getModifiers()))
                .filter(m -> !HIDDEN_INTERFACE_METHOD_WHITELIST.contains(m.toGenericString()))
                .filter(m -> !findMethod(classDescription, m))
                .collect(Collectors.toCollection(ArrayList::new));
    }

    private boolean findMethod(JDiffClassDescription classDescription, Method method) {
        for (JDiffClassDescription.JDiffMethod jdiffMethod : classDescription.getMethods()) {
            if (ReflectionHelper.matches(jdiffMethod, method)) {
                return true;
            }
        }
        for (String interfaceName : classDescription.getImplInterfaces()) {
            Class<?> interfaceClass = null;
            try {
                interfaceClass = ReflectionHelper.findMatchingClass(interfaceName, classProvider);
            } catch (ClassNotFoundException e) {
                LogHelper.loge("ClassNotFoundException for " + classDescription.getAbsoluteClassName(), e);
            }

            JDiffClassDescription implInterface = class2Description.get(interfaceClass);
            if (implInterface == null) {
                // Class definition is not in the scope of the API definitions.
                continue;
            }

            if (findMethod(implInterface, method)) {
                return true;
            }
        }
        return false;
    }


    void queueForDeferredCheck(JDiffClassDescription classDescription, Class<?> runtimeClass) {

        JDiffClassDescription existingDescription = class2Description.get(runtimeClass);
        if (existingDescription != null) {
            for (JDiffClassDescription.JDiffMethod method : classDescription.getMethods()) {
                existingDescription.addMethod(method);
            }
        } else {
            class2Description.put(runtimeClass, classDescription);
        }
    }
}
