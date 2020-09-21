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
package com.android.icu4j.srcgen;

import static com.google.currysrc.api.process.ast.PackageMatcher.getPackageName;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Sets;
import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;

import java.util.List;
import java.util.Set;

import org.eclipse.jdt.core.dom.Annotation;
import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.ImportDeclaration;
import org.eclipse.jdt.core.dom.MarkerAnnotation;
import org.eclipse.jdt.core.dom.MethodDeclaration;
import org.eclipse.jdt.core.dom.Modifier;
import org.eclipse.jdt.core.dom.TypeDeclaration;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;
import org.eclipse.text.edits.TextEditGroup;

/**
 * A {@link Processor} which applies annotations test classes to indicate how they ought to be
 * sharded. It may also apply annotations to non-test classes in the test package, which will be
 * ignored (this avoids reproducing the logic to identify test classes).
 */
class ShardingAnnotator implements Processor {

    /**
     * The package in which the annotations to apply live.
     */
    private static final String ANNOTATION_PACKAGE = "android.icu.testsharding";

    /**
     * The simple name of the annotation to apply for test classes not listed in {@link
     * #CLASS_NAME_TO_ANNOTATION_NAME}.
     */
    private static final String DEFAULT_ANNOTATION_NAME = "MainTestShard";

    /**
     * A map from the fully-qualified names of test classes which need specific annotations to the
     * simple names of those annotations.
     */
    private static final ImmutableMap<String, String> CLASS_NAME_TO_ANNOTATION_NAME =
            // Keys are in alphabetical order.
            // All annotations must be included in cts/tests/tests/icu/AndroidTest.xml.
            ImmutableMap.of(
                    "android.icu.dev.test.format.NumberRegressionTests", "HiMemTestShard",
                    "android.icu.dev.test.format.TimeUnitTest", "HiMemTestShard"
            );

    @Override
    public void process(Context context, CompilationUnit cu) {
        List types = cu.types();
        ASTRewrite rewrite = context.rewrite();
        Set<String> imports = Sets.newHashSet();
        for (Object type : types) {
            if (type instanceof TypeDeclaration) {
                TypeDeclaration declaration = (TypeDeclaration) type;
                if (needsAnnotation(declaration)) {
                    annotateTestType(cu, rewrite, declaration, imports);
                }
            }
        }
    }

    private boolean needsAnnotation(TypeDeclaration declaration) {
        int modifiers = declaration.getModifiers();
        return !declaration.isInterface()
                && !Modifier.isAbstract(modifiers)
                && Modifier.isPublic(modifiers);
    }

    private void annotateTestType(
            CompilationUnit cu, ASTRewrite rewrite, TypeDeclaration type, Set<String> imports) {
        AST ast = cu.getAST();
        String className = getPackageName(type)  + '.' + type.getName().getIdentifier();
        String annotationName = getAnnotationName(className);

        if (!imports.contains(annotationName)) {
            appendImport(cu, rewrite, annotationName);
            imports.add(annotationName);
        }

        MarkerAnnotation annotation = ast.newMarkerAnnotation();
        annotation.setTypeName(ast.newSimpleName(annotationName));
        TextEditGroup editGroup = null;
        rewrite.getListRewrite(type, type.getModifiersProperty())
                .insertFirst(annotation, editGroup);
    }

    private String getAnnotationName(String className) {
        if (CLASS_NAME_TO_ANNOTATION_NAME.containsKey(className)) {
            return CLASS_NAME_TO_ANNOTATION_NAME.get(className);
        } else {
            return DEFAULT_ANNOTATION_NAME;
        }
    }

    private void appendImport(CompilationUnit cu, ASTRewrite rewrite, String annotationName) {
        AST ast = cu.getAST();
        ImportDeclaration importDeclaration = ast.newImportDeclaration();
        importDeclaration.setName(ast.newName(ANNOTATION_PACKAGE + '.' + annotationName));
        TextEditGroup editGroup = null;
        rewrite.getListRewrite(cu, CompilationUnit.IMPORTS_PROPERTY)
                .insertLast(importDeclaration, editGroup);
    }

    @Override
    public String toString() {
        return "ShardingAnnotator{}";
    }
}
