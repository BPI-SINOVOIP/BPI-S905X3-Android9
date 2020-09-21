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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringJoiner;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.doclava.ClassInfo;
import com.google.doclava.FieldInfo;
import com.google.doclava.MethodInfo;
import com.google.doclava.ParameterInfo;
import com.google.doclava.TypeInfo;
import com.google.doclava.apicheck.ApiFile;
import com.google.doclava.apicheck.ApiInfo;
import com.google.doclava.apicheck.ApiParseException;

/**
 * Parses API text files (e.g. current.txt) into classes, methods, and fields.
 */
public class ApprovedApis {
    private final Map<String, DefinedClass> definedApiClasses = new HashMap<>();
    private final Map<String, DefinedMethod> definedApiMethods = new HashMap<>();
    private final Map<String, DefinedField> definedApiFields = new HashMap<>();

    public ApprovedApis(Stream<File> apiFiles) {
        apiFiles.forEach(file -> parse(file));

        // build the maps for methods and fields
        definedApiMethods.putAll(definedApiClasses.values().stream()
                .flatMap(c -> c.getMethods().stream()) // all methods from all classes
                .collect(Collectors.toMap(DefinedMethod::getFullSignature,
                        Function.identity()))); // map by their full signatures

        definedApiFields.putAll(definedApiClasses.values().stream()
                .flatMap(c -> c.getFields().stream()) // all fields from all classes
                .collect(Collectors.toMap(DefinedField::getFullSignature,
                        Function.identity()))); // map by their full signatures

        if (UnofficialApisUsageTest.DEBUG) {
            System.err.println("-------------------API------------------");

            definedApiClasses.values().stream().sorted()
                    .forEach(def -> System.err.println(" Defined class: " + def.getName()));

            definedApiMethods.values().stream().sorted()
                    .forEach(def -> System.err
                            .println(" Defined method: " + def.getFullSignature()));

            definedApiFields.values().stream().sorted()
                    .forEach(def -> System.err
                            .println(" Defined field: " + def.getFullSignature()));
        }
    }

    public Map<String, DefinedClass> getDefinedClasses() {
        return definedApiClasses;
    }

    public Map<String, DefinedMethod> getDefinedMethods() {
        return definedApiMethods;
    }

    public Map<String, DefinedField> getDefinedFields() {
        return definedApiFields;
    }

    private void parse(File apiFile) {
        try {
            InputStream is = null;
            try {
                is = this.getClass().getResourceAsStream(apiFile.toString());
                ApiInfo apiInfo = ApiFile.parseApi(apiFile.toString(), is);
                apiInfo.getPackages().values().stream()
                        .flatMap(packageInfo -> packageInfo.allClasses().values().stream())
                        .forEach(classInfo -> {
                            DefinedClass c = definedApiClasses.get(classInfo.qualifiedName());
                            if (c != null) {
                                // if the same class has already been defined by other api file,
                                // then update it
                                DefinedClass newClass = definedClassFrom(classInfo);
                                c.addNewMembers(newClass.getMethods(), newClass.getFields());
                            } else {
                                c = definedClassFrom(classInfo);
                                definedApiClasses.put(c.getName(), c);
                            }
                        });

            } finally {
                if (is != null) {
                    is.close();
                }
            }
        } catch (ApiParseException | IOException e) {
            throw new RuntimeException("Failed to parse " + apiFile, e);
        }
    }

    private static DefinedClass definedClassFrom(ClassInfo def) {
        String name = def.qualifiedName();
        String superClassName = def.superclassName();
        Collection<String> interfaces = def.interfaces().stream().map(ClassInfo::qualifiedName)
                .collect(Collectors.toList());

        Collection<DefinedMethod> methods = new ArrayList<>(
                def.allConstructors().size() + def.selfMethods().size());
        Collection<DefinedField> fields = new ArrayList<>(
                def.enumConstants().size() + def.selfFields().size());

        methods = Stream.concat(
                def.allConstructors().stream().map(ApprovedApis::definedMethodFromConstructor),
                def.selfMethods().stream().map(ApprovedApis::definedMethodFromMethod))
                .collect(Collectors.toSet());

        fields = Stream.concat(def.enumConstants().stream(), def.selfFields().stream())
                .map(ApprovedApis::definedFieldFrom).collect(Collectors.toSet());

        return new DefinedClass(name, superClassName, interfaces, methods, fields);
    }

    private static DefinedMethod definedMethodFromConstructor(MethodInfo def) {
        return definedMethodFrom(def, true);
    }

    private static DefinedMethod definedMethodFromMethod(MethodInfo def) {
        return definedMethodFrom(def, false);
    }

    private static DefinedMethod definedMethodFrom(MethodInfo def, boolean isCtor) {
        StringBuffer sb = new StringBuffer();
        if (isCtor) {
            sb.append("<init>");
        } else {
            sb.append(def.name());
        }
        sb.append('(');
        StringJoiner joiner = new StringJoiner(",");
        for (int i = 0; i < def.parameters().size(); i++) {
            ParameterInfo param = def.parameters().get(i);
            TypeInfo type = param.type();
            TypeInfo typeParameterType = def.getTypeParameter(type.qualifiedTypeName());
            String typeName;
            if (typeParameterType != null) {
                List<TypeInfo> bounds = typeParameterType.extendsBounds();
                if (bounds == null || bounds.size() == 0) {
                    typeName = "java.lang.Object" + type.dimension();
                } else {
                    typeName = bounds.get(0).qualifiedTypeName() + type.dimension();
                }
            } else {
                typeName = type.qualifiedTypeName() + type.dimension();
            }
            if (i == def.parameters().size() - 1 && def.isVarArgs()) {
                typeName += "[]";
            }
            joiner.add(typeName);
        }
        sb.append(joiner.toString());
        sb.append(')');
        if (!isCtor) {
            TypeInfo type = def.returnType();
            TypeInfo typeParameterType = def.getTypeParameter(type.qualifiedTypeName());
            if (typeParameterType != null) {
                List<TypeInfo> bounds = typeParameterType.extendsBounds();
                if (bounds == null || bounds.size() != 1) {
                    sb.append("java.lang.Object" + type.dimension());
                } else {
                    sb.append(bounds.get(0).qualifiedTypeName() + type.dimension());
                }
            } else {
                sb.append(type.qualifiedTypeName() + type.dimension());
            }
        }

        String signature = sb.toString();
        String containingClass = def.containingClass().qualifiedName();
        return new DefinedMethod(signature, containingClass);
    }

    private static DefinedField definedFieldFrom(FieldInfo def) {
        StringBuffer sb = new StringBuffer(def.name());
        sb.append(":");

        TypeInfo type = def.type();
        TypeInfo typeParameterType = def.containingClass()
                .getTypeParameter(type.qualifiedTypeName());
        if (typeParameterType != null) {
            List<TypeInfo> bounds = typeParameterType.extendsBounds();
            if (bounds == null || bounds.size() != 1) {
                sb.append("java.lang.Object" + type.dimension());
            } else {
                sb.append(bounds.get(0).qualifiedTypeName() + type.dimension());
            }
        } else {
            sb.append(type.qualifiedTypeName() + type.dimension());
        }

        String signature = sb.toString();
        String containingClass = def.containingClass().qualifiedName();
        return new DefinedField(signature, containingClass);
    }
}
