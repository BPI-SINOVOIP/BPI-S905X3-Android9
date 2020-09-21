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

package com.android.tools.metalava

import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.ConstructorItem
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.ModifierList
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.TypeItem
import com.android.tools.metalava.model.TypeParameterList
import com.android.tools.metalava.model.javaEscapeString
import com.android.tools.metalava.model.visitors.ApiVisitor
import java.io.PrintWriter
import java.util.function.Predicate

class SignatureWriter(
    private val writer: PrintWriter,
    filterEmit: Predicate<Item>,
    filterReference: Predicate<Item>,
    private val preFiltered: Boolean
) : ApiVisitor(
    visitConstructorsAsMethods = false,
    nestInnerClasses = false,
    inlineInheritedFields = true,
    methodComparator = MethodItem.comparator,
    fieldComparator = FieldItem.comparator,
    filterEmit = filterEmit,
    filterReference = filterReference
) {

    override fun visitPackage(pkg: PackageItem) {
        writer.print("package ${pkg.qualifiedName()} {\n\n")
    }

    override fun afterVisitPackage(pkg: PackageItem) {
        writer.print("}\n\n")
    }

    override fun visitConstructor(constructor: ConstructorItem) {
        writer.print("    ctor ")
        writeModifiers(constructor)
        // Note - we don't write out the type parameter list (constructor.typeParameterList()) in signature files!
        // writeTypeParameterList(constructor.typeParameterList(), addSpace = true)
        writer.print(constructor.containingClass().fullName())
        writeParameterList(constructor)
        writeThrowsList(constructor)
        writer.print(";\n")
    }

    override fun visitField(field: FieldItem) {
        val name = if (field.isEnumConstant()) "enum_constant" else "field"
        writer.print("    ")
        writer.print(name)
        writer.print(" ")
        writeModifiers(field)
        writeType(field, field.type(), field.modifiers)
        writer.print(' ')
        writer.print(field.name())
        field.writeValueWithSemicolon(writer, allowDefaultValue = false, requireInitialValue = false)
        writer.print("\n")
    }

    override fun visitMethod(method: MethodItem) {
        if (compatibility.skipAnnotationInstanceMethods && method.containingClass().isAnnotationType() &&
            !method.modifiers.isStatic()
        ) {
            return
        }

        if (compatibility.skipInheritedMethods && method.inheritedMethod) {
            return
        }

        writer.print("    method ")
        writeModifiers(method)
        writeTypeParameterList(method.typeParameterList(), addSpace = true)

        writeType(method, method.returnType(), method.modifiers)
        writer.print(' ')
        writer.print(method.name())
        writeParameterList(method)
        writeThrowsList(method)
        writer.print(";\n")
    }

    override fun visitClass(cls: ClassItem) {
        writer.print("  ")

        if (compatibility.extraSpaceForEmptyModifiers && cls.isPackagePrivate && cls.isPackagePrivate) {
            writer.print(" ")
        }

        writeModifiers(cls)

        if (cls.isAnnotationType()) {
            if (compatibility.classForAnnotations) {
                // doclava incorrectly treats annotations (such as TargetApi) as an abstract class instead
                // of an @interface!
                //
                // Example:
                //   public abstract class SuppressLint implements java.lang.annotation.Annotation { }
                writer.print("class")
            } else {
                writer.print("@interface")
            }
        } else if (cls.isInterface()) {
            writer.print("interface")
        } else if (!compatibility.classForEnums && cls.isEnum()) { // compat mode calls enums "class" instead
            writer.print("enum")
        } else {
            writer.print("class")
        }
        writer.print(" ")
        writer.print(cls.fullName())
        writeTypeParameterList(cls.typeParameterList(), addSpace = false)
        writeSuperClassStatement(cls)
        writeInterfaceList(cls)

        writer.print(" {\n")
    }

    override fun afterVisitClass(cls: ClassItem) {
        writer.print("  }\n\n")
    }

    private fun writeModifiers(item: Item) {
        ModifierList.write(
            writer = writer,
            modifiers = item.modifiers,
            item = item,
            includeDeprecated = true,
            includeAnnotations = compatibility.annotationsInSignatures,
            skipNullnessAnnotations = options.outputKotlinStyleNulls,
            omitCommonPackages = options.omitCommonPackages,
            onlyIncludeSignatureAnnotations = true
        )
    }

    private fun writeSuperClassStatement(cls: ClassItem) {
        if (!compatibility.classForEnums && cls.isEnum() || cls.isAnnotationType()) {
            return
        }

        if (cls.isInterface() && compatibility.extendsForInterfaceSuperClass) {
            // Written in the interface section instead
            return
        }

        val superClass = if (preFiltered)
            cls.superClassType()
        else cls.filteredSuperClassType(filterReference)
        if (superClass != null && !superClass.isJavaLangObject()) {
            val superClassString =
                superClass.toTypeString(erased = compatibility.omitTypeParametersInInterfaces)
            writer.print(" extends ")
            writer.print(superClassString)
        }
    }

    private fun writeInterfaceList(cls: ClassItem) {
        if (cls.isAnnotationType()) {
            if (compatibility.classForAnnotations) {
                writer.print(" implements java.lang.annotation.Annotation")
            }
            return
        }
        val isInterface = cls.isInterface()

        val interfaces = if (preFiltered)
            cls.interfaceTypes().asSequence()
        else cls.filteredInterfaceTypes(filterReference).asSequence()
        val all: Sequence<TypeItem> = if (isInterface && compatibility.extendsForInterfaceSuperClass) {
            val superClassType = cls.superClassType()
            if (superClassType != null && !superClassType.isJavaLangObject()) {
                interfaces.plus(sequenceOf(superClassType))
            } else {
                interfaces
            }
        } else {
            interfaces
        }

        if (all.any()) {
            val label = if (isInterface && !compatibility.extendsForInterfaceSuperClass) " extends" else " implements"
            writer.print(label)

            all.sortedWith(TypeItem.comparator).forEach { item ->
                writer.print(" ")
                writer.print(item.toTypeString(erased = compatibility.omitTypeParametersInInterfaces))
            }
        }
    }

    private fun writeTypeParameterList(typeList: TypeParameterList, addSpace: Boolean) {
        val typeListString = typeList.toString()
        if (typeListString.isNotEmpty()) {
            writer.print(typeListString)
            if (addSpace) {
                writer.print(' ')
            }
        }
    }

    private fun writeParameterList(method: MethodItem) {
        writer.print("(")
        val emitParameterNames = compatibility.parameterNames
        method.parameters().asSequence().forEachIndexed { i, parameter ->
            if (i > 0) {
                writer.print(", ")
            }
            writeModifiers(parameter)
            writeType(parameter, parameter.type(), parameter.modifiers)
            if (emitParameterNames) {
                val name = parameter.publicName()
                if (name != null) {
                    writer.print(" ")
                    writer.print(name)
                }
            }
            if (options.outputDefaultValues && parameter.hasDefaultValue()) {
                writer.print(" = \"")
                val defaultValue = parameter.defaultValue()
                if (defaultValue != null) {
                    writer.print(javaEscapeString(defaultValue))
                } else {
                    // null is a valid default value!
                    writer.print("null")
                }
                writer.print("\"")
            }
        }
        writer.print(")")
    }

    private fun writeType(
        item: Item,
        type: TypeItem?,
        modifiers: ModifierList
    ) {
        type ?: return

        var typeString = type.toTypeString(
            erased = false,
            outerAnnotations = false,
            innerAnnotations = compatibility.annotationsInSignatures
        )

        // Strip java.lang. prefix?
        if (options.omitCommonPackages) {
            typeString = TypeItem.shortenTypes(typeString)
        }

        if (typeString.endsWith(", ?>") && compatibility.includeExtendsObjectInWildcard && item is ParameterItem) {
            // This wasn't done universally; just in a few places, so replicate it for those exact places
            val methodName = item.containingMethod().name()
            when (methodName) {
                "computeIfAbsent" -> {
                    if (typeString == "java.util.function.Function<? super java.lang.Object, ?>") {
                        typeString = "java.util.function.Function<? super java.lang.Object, ? extends java.lang.Object>"
                    }
                }
                "computeIfPresent", "merge", "replaceAll", "compute" -> {
                    if (typeString == "java.util.function.BiFunction<? super java.lang.Object, ? super java.lang.Object, ?>") {
                        typeString =
                            "java.util.function.BiFunction<? super java.lang.Object, ? super java.lang.Object, ? extends java.lang.Object>"
                    }
                }
            }
        }

        writer.print(typeString)

        if (options.outputKotlinStyleNulls && !type.primitive) {
            var nullable: Boolean? = null
            for (annotation in modifiers.annotations()) {
                if (annotation.isNullable()) {
                    nullable = true
                } else if (annotation.isNonNull()) {
                    nullable = false
                }
            }
            when (nullable) {
                null -> writer.write("!")
                true -> writer.write("?")
            // else: non-null: nothing to write
            }
        }
    }

    private fun writeThrowsList(method: MethodItem) {
        val throws = when {
            preFiltered -> method.throwsTypes().asSequence()
            compatibility.filterThrowsClasses -> method.filteredThrowsTypes(filterReference).asSequence()
            else -> method.throwsTypes().asSequence()
        }
        if (throws.any()) {
            writer.print(" throws ")
            throws.asSequence().sortedWith(ClassItem.fullNameComparator).forEachIndexed { i, type ->
                if (i > 0) {
                    writer.print(", ")
                }
                writer.print(type.qualifiedName())
            }
        }
    }
}