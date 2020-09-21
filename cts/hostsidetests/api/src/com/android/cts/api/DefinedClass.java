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

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.stream.Collectors;

/**
 * A class defined in a DEX or in an API file
 */
public final class DefinedClass implements Comparable<DefinedClass> {
    private final String name;
    private final String superClassName;
    private final Collection<String> interfaceNames;
    private final Collection<DefinedMethod> methods;
    private final Collection<DefinedField> fields;
    private final Collection<String> methodSignatures;
    private final Collection<String> fieldSignatures;

    DefinedClass(String name, String superClassName, Collection<String> interfaceNames,
            Collection<DefinedMethod> methods, Collection<DefinedField> fields) {
        this.name = name;
        this.superClassName = superClassName;
        this.interfaceNames = interfaceNames;
        // these are Set instead of List to eliminate duplication from multiple api files
        this.methods = new HashSet<>();
        this.fields = new HashSet<>();

        // signatures is okay to be list because they are always re-generated
        // in addMembersFrom
        this.methodSignatures = new ArrayList<>();
        this.fieldSignatures = new ArrayList<>();

        addNewMembers(methods, fields);
    }

    void addNewMembers(Collection<DefinedMethod> newMethods, Collection<DefinedField> newFields) {
        methods.addAll(newMethods);
        fields.addAll(newFields);

        // re-generate the signatures list
        methodSignatures.clear();
        methodSignatures.addAll(methods.stream()
                .map(DefinedMethod::getSignature)
                .collect(Collectors.toList()));

        fieldSignatures.clear();
        fieldSignatures.addAll(fields.stream()
                .map(DefinedField::getSignature)
                .collect(Collectors.toList()));
    }

    /**
     * Returns canonical name of a class
     */
    public String getName() {
        return name;
    }

    String getSuperClass() {
        return superClassName;
    }

    Collection<String> getInterfaces() {
        return interfaceNames;
    }

    Collection<DefinedMethod> getMethods() {
        return methods;
    }

    Collection<DefinedField> getFields() {
        return fields;
    }

    Collection<String> getMethodSignatures() {
        return methodSignatures;
    }

    Collection<String> getFieldSignatures() {
        return fieldSignatures;
    }

    boolean isEnum() {
        return getSuperClass().equals("java.lang.Enum");
    }

    @Override
    public int compareTo(DefinedClass o) {
        if (o != null) {
            return getName().compareTo(o.getName());
        }
        return 0;
    }
}
