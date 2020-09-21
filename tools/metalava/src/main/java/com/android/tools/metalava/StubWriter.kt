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

import com.android.tools.metalava.doclava1.ApiPredicate
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.doclava1.FilterPredicate
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.ConstructorItem
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MemberItem
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.ModifierList
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.TypeParameterList
import com.android.tools.metalava.model.psi.PsiClassItem
import com.android.tools.metalava.model.psi.trimDocIndent
import com.android.tools.metalava.model.visitors.ApiVisitor
import com.google.common.io.Files
import java.io.BufferedWriter
import java.io.File
import java.io.FileWriter
import java.io.IOException
import java.io.PrintWriter
import kotlin.text.Charsets.UTF_8

class StubWriter(
    private val codebase: Codebase,
    private val stubsDir: File,
    private val generateAnnotations: Boolean = false,
    private val preFiltered: Boolean = true,
    docStubs: Boolean
) : ApiVisitor(
    visitConstructorsAsMethods = false,
    nestInnerClasses = true,
    inlineInheritedFields = true,
    fieldComparator = FieldItem.comparator,
    // Methods are by default sorted in source order in stubs, to encourage methods
    // that are near each other in the source to show up near each other in the documentation
    methodComparator = MethodItem.sourceOrderComparator,
    filterEmit = FilterPredicate(ApiPredicate(codebase, ignoreShown = true, includeDocOnly = docStubs)),
    filterReference = ApiPredicate(codebase, ignoreShown = true, includeDocOnly = docStubs),
    includeEmptyOuterClasses = true
) {

    private val sourceList = StringBuilder(20000)

    override fun include(cls: ClassItem): Boolean {
        val filter = options.stubPackages
        if (filter != null && !filter.matches(cls.containingPackage())) {
            return false
        }
        return super.include(cls)
    }

    /** Writes a source file list of the generated stubs */
    fun writeSourceList(target: File, root: File?) {
        target.parentFile?.mkdirs()
        val contents = if (root != null) {
            val path = root.path.replace('\\', '/')
            sourceList.toString().replace(path, "")
        } else {
            sourceList.toString()
        }
        Files.asCharSink(target, UTF_8).write(contents)
    }

    private fun startFile(sourceFile: File) {
        if (sourceList.isNotEmpty()) {
            sourceList.append(' ')
        }
        sourceList.append(sourceFile.path.replace('\\', '/'))
    }

    override fun visitPackage(pkg: PackageItem) {
        getPackageDir(pkg, create = true)

        writePackageInfo(pkg)

        codebase.getPackageDocs()?.getDocs(pkg)?.let { writeDocOverview(pkg, it) }
    }

    private fun writeDocOverview(pkg: PackageItem, content: String) {
        if (content.isBlank()) {
            return
        }

        val sourceFile = File(getPackageDir(pkg), "overview.html")
        val writer = try {
            PrintWriter(BufferedWriter(FileWriter(sourceFile)))
        } catch (e: IOException) {
            reporter.report(Errors.IO_ERROR, sourceFile, "Cannot open file for write.")
            return
        }

        // Should we include this in our stub list?
        //     startFile(sourceFile)

        writer.println(content)
        writer.flush()
        writer.close()
    }

    private fun writePackageInfo(pkg: PackageItem) {
        if (!generateAnnotations) {
            // package-info,java is only needed to record annotations
            return
        }

        val annotations = pkg.modifiers.annotations()
        if (annotations.isNotEmpty()) {
            val sourceFile = File(getPackageDir(pkg), "package-info.java")
            val writer = try {
                PrintWriter(BufferedWriter(FileWriter(sourceFile)))
            } catch (e: IOException) {
                reporter.report(Errors.IO_ERROR, sourceFile, "Cannot open file for write.")
                return
            }
            startFile(sourceFile)

            ModifierList.writeAnnotations(
                list = pkg.modifiers,
                separateLines = true,
                // Some bug in UAST triggers duplicate nullability annotations
                // here; make sure the are filtered out
                filterDuplicates = true,
                onlyIncludeSignatureAnnotations = true,
                writer = writer
            )
            writer.println("package ${pkg.qualifiedName()};")

            writer.flush()
            writer.close()
        }
    }

    private fun getPackageDir(packageItem: PackageItem, create: Boolean = true): File {
        val relative = packageItem.qualifiedName().replace('.', File.separatorChar)
        val dir = File(stubsDir, relative)
        if (create && !dir.isDirectory) {
            val ok = dir.mkdirs()
            if (!ok) {
                throw IOException("Could not create $dir")
            }
        }

        return dir
    }

    private fun getClassFile(classItem: ClassItem): File {
        assert(classItem.containingClass() == null) { "Should only be called on top level classes" }
        // TODO: Look up compilation unit language
        return File(getPackageDir(classItem.containingPackage()), "${classItem.simpleName()}.java")
    }

    /**
     * Between top level class files the [writer] field doesn't point to a real file; it
     * points to this writer, which redirects to the error output. Nothing should be written
     * to the writer at that time.
     */
    private var errorWriter = PrintWriter(options.stderr)

    /** The writer to write the stubs file to */
    private var writer: PrintWriter = errorWriter

    override fun visitClass(cls: ClassItem) {
        if (cls.isTopLevelClass()) {
            val sourceFile = getClassFile(cls)
            writer = try {
                PrintWriter(BufferedWriter(FileWriter(sourceFile)))
            } catch (e: IOException) {
                reporter.report(Errors.IO_ERROR, sourceFile, "Cannot open file for write.")
                errorWriter
            }

            startFile(sourceFile)

            // Copyright statements from the original file?
            val compilationUnit = cls.getCompilationUnit()
            compilationUnit?.getHeaderComments()?.let { writer.println(it) }

            val qualifiedName = cls.containingPackage().qualifiedName()
            if (qualifiedName.isNotBlank()) {
                writer.println("package $qualifiedName;")
                writer.println()
            }

            compilationUnit?.getImportStatements(filterReference)?.let {
                for (item in it) {
                    when (item) {
                        is ClassItem ->
                            writer.println("import ${item.qualifiedName()};")
                        is MemberItem ->
                            writer.println("import static ${item.containingClass().qualifiedName()}.${item.name()};")
                    }
                }
                writer.println()
            }
        }

        appendDocumentation(cls, writer)

        // "ALL" doesn't do it; compiler still warns unless you actually explicitly list "unchecked"
        writer.println("@SuppressWarnings({\"unchecked\", \"deprecation\", \"all\"})")

        // Need to filter out abstract from the modifiers list and turn it
        // into a concrete method to make the stub compile
        val removeAbstract = cls.modifiers.isAbstract() && (cls.isEnum() || cls.isAnnotationType())

        appendModifiers(cls, removeAbstract)

        when {
            cls.isAnnotationType() -> writer.print("@interface")
            cls.isInterface() -> writer.print("interface")
            cls.isEnum() -> writer.print("enum")
            else -> writer.print("class")
        }

        writer.print(" ")
        writer.print(cls.simpleName())

        generateTypeParameterList(typeList = cls.typeParameterList(), addSpace = false)
        generateSuperClassStatement(cls)
        generateInterfaceList(cls)

        writer.print(" {\n")

        if (cls.isEnum()) {
            var first = true
            // Enums should preserve the original source order, not alphabetical etc sort
            for (field in cls.fields().sortedBy { it.sortingRank }) {
                if (field.isEnumConstant()) {
                    if (first) {
                        first = false
                    } else {
                        writer.write(", ")
                    }
                    writer.write(field.name())
                }
            }
            writer.println(";")
        }

        generateMissingConstructors(cls)
    }

    private fun appendDocumentation(item: Item, writer: PrintWriter) {
        val documentation = item.documentation
        if (documentation.isNotBlank()) {
            val trimmed = trimDocIndent(documentation)
            writer.println(trimmed)
            writer.println()
        }
    }

    override fun afterVisitClass(cls: ClassItem) {
        writer.print("}\n\n")

        if (cls.isTopLevelClass()) {
            writer.flush()
            writer.close()
            writer = errorWriter
        }
    }

    private fun appendModifiers(
        item: Item,
        removeAbstract: Boolean = false,
        removeFinal: Boolean = false,
        addPublic: Boolean = false
    ) {
        appendModifiers(item, item.modifiers, removeAbstract, removeFinal, addPublic)
    }

    private fun appendModifiers(
        item: Item,
        modifiers: ModifierList,
        removeAbstract: Boolean,
        removeFinal: Boolean = false,
        addPublic: Boolean = false
    ) {
        if (item.deprecated && generateAnnotations) {
            writer.write("@Deprecated ")
        }

        ModifierList.write(
            writer, modifiers, item, removeAbstract = removeAbstract, removeFinal = removeFinal,
            addPublic = addPublic, includeAnnotations = generateAnnotations,
            onlyIncludeSignatureAnnotations = true
        )
    }

    private fun generateSuperClassStatement(cls: ClassItem) {
        if (cls.isEnum() || cls.isAnnotationType()) {
            // No extends statement for enums and annotations; it's implied by the "enum" and "@interface" keywords
            return
        }

        val superClass = if (preFiltered)
            cls.superClassType()
        else cls.filteredSuperClassType(filterReference)

        if (superClass != null && !superClass.isJavaLangObject()) {
            val qualifiedName = superClass.toTypeString()
            writer.print(" extends ")

            if (qualifiedName.contains("<")) {
                // TODO: I need to push this into the model at filter-time such that clients don't need
                // to remember to do this!!
                val s = superClass.asClass()
                if (s != null) {
                    val map = cls.mapTypeVariables(s)
                    val replaced = superClass.convertTypeString(map)
                    writer.print(replaced)
                    return
                }
            }
            (cls as PsiClassItem).psiClass.superClassType
            writer.print(qualifiedName)
        }
    }

    private fun generateInterfaceList(cls: ClassItem) {
        if (cls.isAnnotationType()) {
            // No extends statement for annotations; it's implied by the "@interface" keyword
            return
        }

        val interfaces = if (preFiltered)
            cls.interfaceTypes().asSequence()
        else cls.filteredInterfaceTypes(filterReference).asSequence()

        if (interfaces.any()) {
            if (cls.isInterface() && cls.superClassType() != null)
                writer.print(", ")
            else writer.print(" implements")
            interfaces.forEachIndexed { index, type ->
                if (index > 0) {
                    writer.print(",")
                }
                writer.print(" ")
                writer.print(type.toTypeString())
            }
        } else if (compatibility.classForAnnotations && cls.isAnnotationType()) {
            writer.print(" implements java.lang.annotation.Annotation")
        }
    }

    private fun generateTypeParameterList(
        typeList: TypeParameterList,
        addSpace: Boolean
    ) {
        // TODO: Do I need to map type variables?

        val typeListString = typeList.toString()
        if (typeListString.isNotEmpty()) {
            writer.print(typeListString)

            if (addSpace) {
                writer.print(' ')
            }
        }
    }

    override fun visitConstructor(constructor: ConstructorItem) {
        writeConstructor(constructor, constructor.superConstructor)
    }

    private fun writeConstructor(
        constructor: MethodItem,
        superConstructor: MethodItem?
    ) {
        writer.println()
        appendDocumentation(constructor, writer)
        appendModifiers(constructor, false)
        generateTypeParameterList(
            typeList = constructor.typeParameterList(),
            addSpace = true
        )
        writer.print(constructor.containingClass().simpleName())

        generateParameterList(constructor)
        generateThrowsList(constructor)

        writer.print(" { ")

        writeConstructorBody(constructor, superConstructor)
        writer.println(" }")
    }

    private fun writeConstructorBody(constructor: MethodItem?, superConstructor: MethodItem?) {
        // Find any constructor in parent that we can compile against
        superConstructor?.let { it ->
            val parameters = it.parameters()
            val invokeOnThis = constructor != null && constructor.containingClass() == it.containingClass()
            if (invokeOnThis || parameters.isNotEmpty()) {
                val includeCasts = parameters.isNotEmpty() &&
                    it.containingClass().constructors().filter { filterReference.test(it) }.size > 1
                if (invokeOnThis) {
                    writer.print("this(")
                } else {
                    writer.print("super(")
                }
                parameters.forEachIndexed { index, parameter ->
                    if (index > 0) {
                        writer.write(", ")
                    }
                    val type = parameter.type()
                    val typeString = type.toErasedTypeString()
                    if (!type.primitive) {
                        if (includeCasts) {
                            writer.write("(")

                            // Types with varargs can't appear as varargs when used as an argument
                            if (typeString.contains("...")) {
                                writer.write(typeString.replace("...", "[]"))
                            } else {
                                writer.write(typeString)
                            }
                            writer.write(")")
                        }
                        writer.write("null")
                    } else {
                        if (typeString != "boolean" && typeString != "int" && typeString != "long") {
                            writer.write("(")
                            writer.write(typeString)
                            writer.write(")")
                        }
                        writer.write(type.defaultValueString())
                    }
                }
                writer.print("); ")
            }
        }

        writeThrowStub()
    }

    private fun generateMissingConstructors(cls: ClassItem) {
        val clsDefaultConstructor = cls.defaultConstructor
        val constructors = cls.filteredConstructors(filterEmit)
        if (clsDefaultConstructor != null && !constructors.contains(clsDefaultConstructor)) {
            clsDefaultConstructor.mutableModifiers().setPackagePrivate(true)
            visitConstructor(clsDefaultConstructor)
            return
        }
    }

    override fun visitMethod(method: MethodItem) {
        writeMethod(method.containingClass(), method, false)
    }

    private fun writeMethod(containingClass: ClassItem, method: MethodItem, movedFromInterface: Boolean) {
        val modifiers = method.modifiers
        val isEnum = containingClass.isEnum()
        val isAnnotation = containingClass.isAnnotationType()

        if (isEnum && (method.name() == "values" ||
                method.name() == "valueOf" && method.parameters().size == 1 &&
                method.parameters()[0].type().toTypeString() == JAVA_LANG_STRING)
        ) {
            // Skip the values() and valueOf(String) methods in enums: these are added by
            // the compiler for enums anyway, but was part of the doclava1 signature files
            // so inserted in compat mode.
            return
        }

        writer.println()
        appendDocumentation(method, writer)

        // Need to filter out abstract from the modifiers list and turn it
        // into a concrete method to make the stub compile
        val removeAbstract = modifiers.isAbstract() && (isEnum || isAnnotation) || movedFromInterface

        appendModifiers(method, modifiers, removeAbstract, movedFromInterface)
        generateTypeParameterList(typeList = method.typeParameterList(), addSpace = true)

        val returnType = method.returnType()
        writer.print(returnType?.toTypeString(outerAnnotations = false, innerAnnotations = generateAnnotations))

        writer.print(' ')
        writer.print(method.name())
        generateParameterList(method)
        generateThrowsList(method)

        if (modifiers.isAbstract() && !removeAbstract && !isEnum || isAnnotation || modifiers.isNative()) {
            writer.println(";")
        } else {
            writer.print(" { ")
            writeThrowStub()
            writer.println(" }")
        }
    }

    override fun visitField(field: FieldItem) {
        // Handled earlier in visitClass
        if (field.isEnumConstant()) {
            return
        }

        writer.println()

        appendDocumentation(field, writer)
        appendModifiers(field, false, false)
        writer.print(field.type().toTypeString(outerAnnotations = false, innerAnnotations = generateAnnotations))
        writer.print(' ')
        writer.print(field.name())
        val needsInitialization =
            field.modifiers.isFinal() && field.initialValue(true) == null && field.containingClass().isClass()
        field.writeValueWithSemicolon(
            writer,
            allowDefaultValue = !needsInitialization,
            requireInitialValue = !needsInitialization
        )
        writer.print("\n")

        if (needsInitialization) {
            if (field.modifiers.isStatic()) {
                writer.print("static ")
            }
            writer.print("{ ${field.name()} = ${field.type().defaultValueString()}; }\n")
        }
    }

    private fun writeThrowStub() {
        writer.write("throw new RuntimeException(\"Stub!\");")
    }

    private fun generateParameterList(method: MethodItem) {
        writer.print("(")
        method.parameters().asSequence().forEachIndexed { i, parameter ->
            if (i > 0) {
                writer.print(", ")
            }
            appendModifiers(parameter, false)
            writer.print(
                parameter.type().toTypeString(
                    outerAnnotations = false,
                    innerAnnotations = generateAnnotations
                )
            )
            writer.print(' ')
            val name = parameter.publicName() ?: parameter.name()
            writer.print(name)
        }
        writer.print(")")
    }

    private fun generateThrowsList(method: MethodItem) {
        // Note that throws types are already sorted internally to help comparison matching
        val throws = if (preFiltered) {
            method.throwsTypes().asSequence()
        } else {
            method.filteredThrowsTypes(filterReference).asSequence()
        }
        if (throws.any()) {
            writer.print(" throws ")
            throws.asSequence().sortedWith(ClassItem.fullNameComparator).forEachIndexed { i, type ->
                if (i > 0) {
                    writer.print(", ")
                }
                // TODO: Shouldn't declare raw types here!
                writer.print(type.qualifiedName())
            }
        }
    }
}