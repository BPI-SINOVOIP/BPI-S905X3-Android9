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

package com.android.cts.api;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.StringJoiner;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.DexFileFactory.DexFileNotFoundException;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.ReferenceType;
import org.jf.dexlib2.dexbacked.DexBackedClassDef;
import org.jf.dexlib2.dexbacked.DexBackedDexFile;
import org.jf.dexlib2.dexbacked.DexBackedField;
import org.jf.dexlib2.dexbacked.DexBackedMethod;
import org.jf.dexlib2.dexbacked.reference.DexBackedFieldReference;
import org.jf.dexlib2.dexbacked.reference.DexBackedMethodReference;
import org.jf.dexlib2.dexbacked.reference.DexBackedTypeReference;

public class DexAnalyzer {
    private static Map<Character, String> primitiveTypes = new HashMap<>();
    static {
        primitiveTypes.put('Z', "boolean");
        primitiveTypes.put('B', "byte");
        primitiveTypes.put('C', "char");
        primitiveTypes.put('S', "short");
        primitiveTypes.put('I', "int");
        primitiveTypes.put('J', "long");
        primitiveTypes.put('F', "float");
        primitiveTypes.put('D', "double");
        primitiveTypes.put('V', "void");
    }

    // [[Lcom/foo/bar/MyClass$Inner; becomes
    // com.foo.bar.MyClass.Inner[][]
    // and [[I becomes int[][]
    private static String toCanonicalName(String name) {
        int arrayDepth = 0;
        for (int i = 0; i < name.length(); i++) {
            if (name.charAt(i) == '[') {
                arrayDepth++;
            } else {
                break;
            }
        }

        // test the first character.
        final char firstChar = name.charAt(arrayDepth);
        if (primitiveTypes.containsKey(firstChar)) {
            name = primitiveTypes.get(firstChar);
        } else if (firstChar == 'L') {
            // omit the leading 'L' and the trailing ';'
            name = name.substring(arrayDepth + 1, name.length() - 1);

            // replace '/' and '$' to '.'
            name = name.replace('/', '.').replace('$', '.');
        } else {
            throw new RuntimeException("Invalid type name " + name);
        }

        // add []'s, if any
        if (arrayDepth > 0) {
            for (int i = 0; i < arrayDepth; i++) {
                name += "[]";
            }
        }
        return name;
    }

    public static abstract class Ref {
        private final List<String> fromList; // list of files that this reference was found

        protected Ref(String from) {
            fromList = new ArrayList<>();
            fromList.add(from);
        }

        void merge(Ref other) {
            this.fromList.addAll(other.fromList);
        }

        String printReferencedFrom() {
            StringJoiner joiner = new StringJoiner(", ");
            fromList.stream().forEach(name -> joiner.add(name));
            return joiner.toString();
        }
    }

    public static class TypeRef extends Ref implements Comparable<TypeRef> {
        private final DexBackedTypeReference ref;
        private final String name;

        TypeRef(DexBackedTypeReference ref, String from) {
            super(from);
            this.ref = ref;
            name = toCanonicalName(ref.getType());
        }

        String getName() {
            return name;
        }

        boolean isArray() {
            return ref.getType().charAt(0) == '[';
        }

        boolean isPrimitiveType() {
            String name = ref.getType();
            if (name.length() == 1) {
                return primitiveTypes.containsKey(name.charAt(0));
            } else if (name.charAt(0) == '[') {
                return primitiveTypes.containsKey(name.replaceAll("\\[+", "").charAt(0));
            }
            return false;
        }

        @Override
        public int compareTo(TypeRef o) {
            if (o != null) {
                return getName().compareTo(o.getName());
            }
            return 0;
        }
    }

    // common parent class for MethodRef and FieldRef
    public static abstract class MemberRef extends Ref implements Comparable<MemberRef> {
        private final String signature;
        private String definingClass;

        protected MemberRef(String signature, String initialDefiningClass, String from) {
            super(from);
            this.signature = signature;
            // This might be incorrect since DexBacked[Method|Field]Reference.getDefiningClass()
            // only returns the type name of the target object. For example,
            //
            // class Super { void foo() {...} }
            // class Child extends Super { }
            // Child obj = new Child();
            // obj.foo();
            //
            // then method reference for `obj.foo()` is `Child.foo()` since obj is declared as type
            // Child. The true defining class is somewhere in the type hierarchy, which is Super
            // in this case.
            definingClass = initialDefiningClass;
        }

        String getSignature() {
            return signature;
        }

        String getDefiningClass() {
            return definingClass;
        }

        void setDefiningClass(String name) {
            definingClass = name;
        }

        String getFullSignature() {
            return getDefiningClass() + "." + getSignature();
        }

        @Override
        public int compareTo(MemberRef o) {
            if (o != null) {
                return getFullSignature().compareTo(o.getFullSignature());
            }
            return 0;
        }
    }

    public static class MethodRef extends MemberRef {
        private final boolean isConstructor;

        MethodRef(DexBackedMethodReference ref, String from) {
            super(makeSignature(ref), toCanonicalName(ref.getDefiningClass()), from);
            isConstructor = ref.getName().equals("<init>");
        }

        private static String makeSignature(DexBackedMethodReference ref) {
            StringBuffer sb = new StringBuffer();
            sb.append(ref.getName());
            sb.append('(');
            StringJoiner joiner = new StringJoiner(",");
            for (String param : ref.getParameterTypes()) {
                joiner.add(toCanonicalName(param));
            }
            sb.append(joiner.toString());
            sb.append(')');
            if (!ref.getName().equals("<init>")) {
                sb.append(toCanonicalName(ref.getReturnType()));
            }
            return sb.toString();
        }

        boolean isConstructor() {
            return isConstructor;
        }
    }

    public static class FieldRef extends MemberRef {
        FieldRef(DexBackedFieldReference ref, String from) {
            super(makeSignature(ref), toCanonicalName(ref.getDefiningClass()), from);
        }

        private static String makeSignature(DexBackedFieldReference ref) {
            return ref.getName() + ":" + toCanonicalName(ref.getType());
        }
    }

    private final Map<String, DefinedClass> definedClassesInDex = new HashMap<>();
    private final Map<String, DefinedMethod> definedMethodsInDex = new HashMap<>();
    private final Map<String, DefinedField> definedFieldsInDex = new HashMap<>();

    private final Map<String, TypeRef> typeReferences = new HashMap<>();
    private final Map<String, MethodRef> methodReferences = new HashMap<>();
    private final Map<String, FieldRef> fieldReferences = new HashMap<>();

    private final ApprovedApis approvedApis;

    public DexAnalyzer(Stream<PulledFile> files, ApprovedApis approvedApis) {
        this.approvedApis = approvedApis;

        files.forEach(file -> parse(file));

        // Maps for methods and fields are constructed AFTER all files are parsed.
        // This is because different dex files can have the same class with different sets of
        // members - if they are statically linking to the same class which are built
        // with source code at different times. In that case, members of the classes are
        // merged together.
        definedMethodsInDex.putAll(definedClassesInDex.values().stream()
                .map(DefinedClass::getMethods)
                .flatMap(methods -> methods.stream())
                .collect(Collectors.toMap(DefinedMethod::getFullSignature,
                        Function.identity())));

        definedFieldsInDex.putAll(definedClassesInDex.values().stream()
                .map(DefinedClass::getFields)
                .flatMap(fields -> fields.stream())
                .collect(
                        Collectors.toMap(DefinedField::getFullSignature, Function.identity())));

        if (UnofficialApisUsageTest.DEBUG) {
            definedClassesInDex.values().stream().sorted()
                    .forEach(def -> System.err.println(" Defined class: " + def.getName()));

            definedMethodsInDex.values().stream().sorted()
                    .forEach(def -> System.err
                            .println(" Defined method: " + def.getFullSignature()));

            definedFieldsInDex.values().stream().sorted()
                    .forEach(def -> System.err
                            .println(" Defined field: " + def.getFullSignature()));

            typeReferences.values().stream().sorted().forEach(
                    ref -> System.err.println(" type ref: " + ref.getName()));
            methodReferences.values().stream().sorted().forEach(
                    ref -> System.err.println(" method ref: " + ref.getFullSignature()));
            fieldReferences.values().stream().sorted().forEach(
                    ref -> System.err.println(" field ref: " + ref.getFullSignature()));
        }

        updateDefiningClassInReferences();
    }

    /**
     * Parse a dex file to extract symbols defined in the file and symbols referenced from the file
     */
    private void parse(PulledFile file) {
        if (UnofficialApisUsageTest.DEBUG) {
            System.err.println("Analyzing file: " + file.pathInDevice);
        }

        try {
            parseInner(file, "classes.dex");
        } catch (DexFileNotFoundException e) {
            // classes.dex must exist
            throw new RuntimeException("classes.dex" + " not found in " + file, e);
        }

        int i = 2;
        while (true) {
            try {
                parseInner(file, String.format("classes%d.dex", i++));
            } catch (DexFileNotFoundException e) {
                // failing to find additional dex files is okay. we just stop trying.
                break;
            }
        }
    }

    private void parseInner(PulledFile file, String dexEntry) throws DexFileNotFoundException {
        try {
            DexBackedDexFile dexFile = DexFileFactory.loadDexEntry(file.fileInHost, dexEntry, true,
                    Opcodes.getDefault());

            // 1. extract defined symbols and add them to the maps
            dexFile.getClasses().stream().forEach(classDef -> {
                // If the same class is found (defined from one of the previous files), then
                // merge the members of this class to the old class.
                DefinedClass c = definedClassFrom(classDef);
                if (definedClassesInDex.containsKey(c.getName())) {
                    definedClassesInDex.get(c.getName()).addNewMembers(c.getMethods(),
                            c.getFields());
                } else {
                    definedClassesInDex.put(c.getName(), c);
                }
            });

            // 2. extract referenced symbols and add then to the sets
            // Note that these *Ref classes are identified by their names or full signatures.
            // This is required since a same reference can be created by different dex files.

            // array types and primitive types are filtered-out
            dexFile.getReferences(ReferenceType.TYPE).stream()
                    .map(t -> new TypeRef((DexBackedTypeReference) t, file.pathInDevice))
                    .filter(((Predicate<TypeRef>) TypeRef::isArray).negate())
                    .filter(((Predicate<TypeRef>) TypeRef::isPrimitiveType).negate())
                    .forEach(ref -> {
                        if (typeReferences.containsKey(ref.getName())) {
                            typeReferences.get(ref.getName()).merge(ref);
                        } else {
                            typeReferences.put(ref.getName(), ref);
                        }
                    });

            dexFile.getReferences(ReferenceType.METHOD).stream()
                    .map(m -> new MethodRef((DexBackedMethodReference) m, file.pathInDevice))
                    .forEach(ref -> {
                        if (methodReferences.containsKey(ref.getFullSignature())) {
                            methodReferences.get(ref.getFullSignature()).merge(ref);
                        } else {
                            methodReferences.put(ref.getFullSignature(), ref);
                        }
                    });

            dexFile.getReferences(ReferenceType.FIELD).stream()
                    .map(f -> new FieldRef((DexBackedFieldReference) f, file.pathInDevice))
                    .forEach(ref -> {
                        if (fieldReferences.containsKey(ref.getFullSignature())) {
                            fieldReferences.get(ref.getFullSignature()).merge(ref);
                        } else {
                            fieldReferences.put(ref.getFullSignature(), ref);
                        }
                    });

        } catch (IOException e) {
            throw new RuntimeException("Cannot parse dex file in " + file, e);
        }
    }

    /**
     * For For each method/field reference, try to find a class defining it. If found, update the
     * reference in the set since its full signature has changed.
     */
    private void updateDefiningClassInReferences() {
        Stream.concat(methodReferences.values().stream(), fieldReferences.values().stream())
                .forEach(ref -> {
                    DefinedClass c = findDefiningClass(ref);
                    if (c != null && !ref.getDefiningClass().equals(c.getName())) {
                        ref.setDefiningClass(c.getName());
                    }
                });
    }

    /**
     * Try to find a true class defining the given member.
     */
    private DefinedClass findDefiningClass(MemberRef ref) {
        return findMemberInner(ref, ref.getDefiningClass());
    }

    /**
     * Try to find a class defining a member by from the given class up to the parent classes
     */
    private DefinedClass findMemberInner(MemberRef ref, String className) {
        final Function<DefinedClass, Collection<String>> getMethods = (DefinedClass c) -> c
                .getMethodSignatures();
        final Function<DefinedClass, Collection<String>> getFields = (DefinedClass c) -> c
                .getFieldSignatures();

        Function<DefinedClass, Collection<String>> getMembers = ref instanceof MethodRef
                ? getMethods
                : getFields;

        final boolean classFoundInDex = definedClassesInDex.containsKey(className);
        final boolean classFoundInApi = approvedApis.getDefinedClasses().containsKey(className);
        if (!classFoundInDex && !classFoundInApi) {
            // unknown class.
            return null;
        }

        if (classFoundInDex) {
            DefinedClass c = definedClassesInDex.get(className);
            if (getMembers.apply(c).contains(ref.getSignature())) {
                // method was found in the class
                return c;
            }
        }

        if (classFoundInApi) {
            DefinedClass c = approvedApis.getDefinedClasses().get(className);
            if (getMembers.apply(c).contains(ref.getSignature())) {
                // method was found in the class
                return c;
            }
        }

        // member was not found in the class. try finding in parent classes.
        // first gather the name of parent classes both from dex and api
        Set<String> parentClasses = new HashSet<>();
        if (classFoundInDex) {
            DefinedClass c = definedClassesInDex.get(className);
            parentClasses.add(c.getSuperClass());
            parentClasses.addAll(c.getInterfaces());
        }
        if (classFoundInApi) {
            DefinedClass c = approvedApis.getDefinedClasses().get(className);
            parentClasses.add(c.getSuperClass());
            parentClasses.addAll(c.getInterfaces());
        }
        // null can be in parentClasses, because null might have been added as getSuperClass() is
        // null for java.lang.Object
        parentClasses.remove(null);

        for (String pc : parentClasses) {
            DefinedClass foundClass = findMemberInner(ref, pc);
            if (foundClass != null) {
                return foundClass;
            }
        }
        return null;
    }

    private static class SkipIfNeeded implements Predicate<Ref> {
        @Override
        public boolean test(Ref ref) {
            String className = (ref instanceof TypeRef) ?
                    ((TypeRef)ref).getName() : ((MemberRef)ref).getDefiningClass();
            if (className.endsWith("[]")) {
                // Reference to array type is skipped
                return true;
            }
            if (className.startsWith("dalvik.annotation.")
                    || className.startsWith("javax.annotation.")) {
                // These annotation classes are not part of API but they are not explicitly used.
                return true;
            }
            if (className.startsWith("java.lang.")) {
                // core java libraries are exempted.
                return true;
            }
            if (ref instanceof MemberRef) {
                MemberRef memberRef = (MemberRef)ref;
                if (memberRef.getFullSignature().equals(
                        "android.os.SystemProperties.set(java.lang.String,java.lang.String)void")) {
                    // SystemProperties.set is exceptionally allowed.
                    // TODO(b/73750660): remove this when sysprops are publicized via IDs.
                    return true;
                }
            }
            return false;
        }
    }

    // for each type, method and field references, find the ones that are found neither in dex
    // nor in api
    public Stream<TypeRef> collectUndefinedTypeReferences() {
        return typeReferences.values().stream()
                .filter(ref -> !definedClassesInDex.containsKey(ref.getName())
                        && !approvedApis.getDefinedClasses().containsKey(ref.getName()))
                .filter((new SkipIfNeeded()).negate());
    }

    public Stream<MethodRef> collectUndefinedMethodReferences() {
        return methodReferences.values().stream()
                .filter(ref -> !definedMethodsInDex.containsKey(ref.getFullSignature())
                        && !approvedApis.getDefinedMethods().containsKey(ref.getFullSignature()))
                .filter((new SkipIfNeeded()).negate());
    }

    public Stream<FieldRef> collectUndefinedFieldReferences() {
        return fieldReferences.values().stream()
                .filter(ref -> !definedFieldsInDex.containsKey(ref.getFullSignature())
                        && !approvedApis.getDefinedFields().containsKey(ref.getFullSignature()))
                .filter((new SkipIfNeeded()).negate());
    }

    private static DefinedClass definedClassFrom(DexBackedClassDef def) {
        String name = toCanonicalName(def.getType());
        String superClassName = toCanonicalName(def.getSuperclass());
        Collection<String> interfaceNames = def.getInterfaces().stream()
                .map(n -> toCanonicalName(n))
                .collect(Collectors.toList());

        Collection<DefinedMethod> methods = StreamSupport
                .stream(def.getMethods().spliterator(), false /* parallel */)
                .map(DexAnalyzer::definedMethodFrom).collect(Collectors.toList());

        Collection<DefinedField> fields = StreamSupport
                .stream(def.getFields().spliterator(), false /* parallel */)
                .map(DexAnalyzer::definedFieldFrom).collect(Collectors.toList());

        return new DefinedClass(name, superClassName, interfaceNames, methods, fields);
    }

    private static DefinedMethod definedMethodFrom(DexBackedMethod def) {
        StringBuffer sb = new StringBuffer();
        sb.append(def.getName());
        sb.append('(');
        StringJoiner joiner = new StringJoiner(",");
        for (String param : def.getParameterTypes()) {
            joiner.add(toCanonicalName(param));
        }
        sb.append(joiner.toString());
        sb.append(')');
        final boolean isConstructor = def.getName().equals("<init>");
        if (!isConstructor) {
            sb.append(toCanonicalName(def.getReturnType()));
        }

        String signature = sb.toString();
        String definingClass = toCanonicalName(def.getDefiningClass());
        return new DefinedMethod(signature, definingClass);
    }

    private static DefinedField definedFieldFrom(DexBackedField def) {
        String signature = def.getName() + ":" + toCanonicalName(def.getType());
        String definingClass = toCanonicalName(def.getDefiningClass());
        return new DefinedField(signature, definingClass);
    }
}
