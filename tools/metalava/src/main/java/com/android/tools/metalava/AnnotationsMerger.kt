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

import com.android.SdkConstants.AMP_ENTITY
import com.android.SdkConstants.APOS_ENTITY
import com.android.SdkConstants.ATTR_NAME
import com.android.SdkConstants.DOT_CLASS
import com.android.SdkConstants.DOT_JAR
import com.android.SdkConstants.DOT_XML
import com.android.SdkConstants.DOT_ZIP
import com.android.SdkConstants.GT_ENTITY
import com.android.SdkConstants.INT_DEF_ANNOTATION
import com.android.SdkConstants.LT_ENTITY
import com.android.SdkConstants.QUOT_ENTITY
import com.android.SdkConstants.STRING_DEF_ANNOTATION
import com.android.SdkConstants.TYPE_DEF_FLAG_ATTRIBUTE
import com.android.SdkConstants.TYPE_DEF_VALUE_ATTRIBUTE
import com.android.SdkConstants.VALUE_TRUE
import com.android.tools.lint.annotations.Extractor.ANDROID_INT_DEF
import com.android.tools.lint.annotations.Extractor.ANDROID_NOTNULL
import com.android.tools.lint.annotations.Extractor.ANDROID_NULLABLE
import com.android.tools.lint.annotations.Extractor.ANDROID_STRING_DEF
import com.android.tools.lint.annotations.Extractor.ATTR_PURE
import com.android.tools.lint.annotations.Extractor.ATTR_VAL
import com.android.tools.lint.annotations.Extractor.IDEA_CONTRACT
import com.android.tools.lint.annotations.Extractor.IDEA_MAGIC
import com.android.tools.lint.annotations.Extractor.IDEA_NOTNULL
import com.android.tools.lint.annotations.Extractor.IDEA_NULLABLE
import com.android.tools.lint.annotations.Extractor.SUPPORT_NOTNULL
import com.android.tools.lint.annotations.Extractor.SUPPORT_NULLABLE
import com.android.tools.lint.checks.AnnotationDetector
import com.android.tools.lint.detector.api.getChildren
import com.android.tools.metalava.doclava1.ApiFile
import com.android.tools.metalava.doclava1.ApiParseException
import com.android.tools.metalava.doclava1.ApiPredicate
import com.android.tools.metalava.model.AnnotationAttribute
import com.android.tools.metalava.model.AnnotationAttributeValue
import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.DefaultAnnotationValue
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.psi.PsiAnnotationItem
import com.android.tools.metalava.model.visitors.ApiVisitor
import com.android.utils.XmlUtils
import com.google.common.base.Charsets
import com.google.common.io.ByteStreams
import com.google.common.io.Closeables
import com.google.common.io.Files
import org.w3c.dom.Document
import org.w3c.dom.Element
import org.xml.sax.SAXParseException
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.lang.reflect.Field
import java.util.jar.JarInputStream
import java.util.regex.Pattern
import java.util.zip.ZipEntry

/** Merges annotations into classes already registered in the given [Codebase] */
class AnnotationsMerger(
    private val codebase: Codebase
) {
    fun merge(mergeAnnotations: List<File>) {
        mergeAnnotations.forEach { mergeExisting(it) }
    }

    private fun mergeExisting(file: File) {
        if (file.isDirectory) {
            val files = file.listFiles()
            if (files != null) {
                for (child in files) {
                    mergeExisting(child)
                }
            }
        } else if (file.isFile) {
            if (file.path.endsWith(DOT_JAR) || file.path.endsWith(DOT_ZIP)) {
                mergeFromJar(file)
            } else if (file.path.endsWith(DOT_XML)) {
                try {
                    val xml = Files.asCharSource(file, Charsets.UTF_8).read()
                    mergeAnnotationsXml(file.path, xml)
                } catch (e: IOException) {
                    error("Aborting: I/O problem during transform: " + e.toString())
                }
            } else if (file.path.endsWith(".jaif")) {
                try {
                    val jaif = Files.asCharSource(file, Charsets.UTF_8).read()
                    mergeAnnotationsJaif(file.path, jaif)
                } catch (e: IOException) {
                    error("Aborting: I/O problem during transform: " + e.toString())
                }
            } else if (file.path.endsWith(".txt") ||
                file.path.endsWith(".signatures") ||
                file.path.endsWith(".api")
            ) {
                try {
                    // .txt: Old style signature files
                    // Others: new signature files (e.g. kotlin-style nullness info)
                    mergeAnnotationsSignatureFile(file.path)
                } catch (e: IOException) {
                    error("Aborting: I/O problem during transform: " + e.toString())
                }
            }
        }
    }

    private fun mergeFromJar(jar: File) {
        // Reads in an existing annotations jar and merges in entries found there
        // with the annotations analyzed from source.
        var zis: JarInputStream? = null
        try {
            val fis = FileInputStream(jar)
            zis = JarInputStream(fis)
            var entry: ZipEntry? = zis.nextEntry
            while (entry != null) {
                if (entry.name.endsWith(".xml")) {
                    val bytes = ByteStreams.toByteArray(zis)
                    val xml = String(bytes, Charsets.UTF_8)
                    mergeAnnotationsXml(jar.path + ": " + entry, xml)
                }
                entry = zis.nextEntry
            }
        } catch (e: IOException) {
            error("Aborting: I/O problem during transform: " + e.toString())
        } finally {
            try {
                Closeables.close(zis, true /* swallowIOException */)
            } catch (e: IOException) {
                // cannot happen
            }
        }
    }

    private fun mergeAnnotationsXml(path: String, xml: String) {
        try {
            val document = XmlUtils.parseDocument(xml, false)
            mergeDocument(document)
        } catch (e: Exception) {
            var message = "Failed to merge " + path + ": " + e.toString()
            if (e is SAXParseException) {
                message = "Line " + e.lineNumber + ":" + e.columnNumber + ": " + message
            }
            error(message)
            if (e !is IOException) {
                e.printStackTrace()
            }
        }
    }

    private fun mergeAnnotationsSignatureFile(path: String) {
        try {
            // Old style signature files don't support annotations anyway, so we might as well
            // accept
            val kotlinStyleNulls = true
            val supportsStagedNullability = true
            val signatureCodebase = ApiFile.parseApi(File(path), kotlinStyleNulls, supportsStagedNullability)
            signatureCodebase.description = "Signature files for annotation merger: loaded from $path"
            val visitor = object : ComparisonVisitor() {
                override fun compare(old: Item, new: Item) {
                    val newModifiers = new.modifiers
                    for (annotation in old.modifiers.annotations()) {
                        var addAnnotation = false
                        if (annotation.isNullnessAnnotation()) {
                            if (!newModifiers.hasNullnessInfo()) {
                                addAnnotation = true
                            }
                        } else {
                            // TODO: Check for other incompatibilities than nullness?
                            val qualifiedName = annotation.qualifiedName() ?: continue
                            if (newModifiers.findAnnotation(qualifiedName) == null) {
                                addAnnotation = true
                            }
                        }

                        if (addAnnotation) {
                            // Don't map annotation names - this would turn newly non null back into non null
                            new.mutableModifiers().addAnnotation(
                                new.codebase.createAnnotation(
                                    annotation.toSource(),
                                    new,
                                    mapName = false
                                )
                            )
                        }
                    }
                }
            }
            CodebaseComparator().compare(visitor, signatureCodebase, codebase, ApiPredicate(signatureCodebase))
        } catch (ex: ApiParseException) {
            val message = "Unable to parse signature file $path: ${ex.message}"
            throw DriverException(message)
        }
    }

    private fun mergeAnnotationsJaif(path: String, jaif: String) {
        var pkgItem: PackageItem? = null
        var clsItem: ClassItem? = null
        var methodItem: MethodItem? = null
        var curr: Item? = null

        for (rawLine in jaif.split("\n")) {
            val line = rawLine.trim()
            if (line.isEmpty()) {
                continue
            }
            if (line.startsWith("//")) {
                continue
            }
            if (line.startsWith("package ")) {
                val pkg = line.substring("package ".length, line.length - 1)
                pkgItem = codebase.findPackage(pkg)
                curr = pkgItem
            } else if (line.startsWith("class ")) {
                val cls = line.substring("class ".length, line.length - 1)
                clsItem = if (pkgItem != null)
                    codebase.findClass(pkgItem.qualifiedName() + "." + cls)
                else
                    null
                curr = clsItem
            } else if (line.startsWith("annotation ")) {
                val cls = line.substring("annotation ".length, line.length - 1)
                clsItem = if (pkgItem != null)
                    codebase.findClass(pkgItem.qualifiedName() + "." + cls)
                else
                    null
                curr = clsItem
            } else if (line.startsWith("method ")) {
                val method = line.substring("method ".length, line.length - 1)
                methodItem = null
                if (clsItem != null) {
                    val index = method.indexOf('(')
                    if (index != -1) {
                        val name = method.substring(0, index)
                        val desc = method.substring(index)
                        methodItem = clsItem.findMethodByDesc(name, desc, true, true)
                    }
                }
                curr = methodItem
            } else if (line.startsWith("field ")) {
                val field = line.substring("field ".length, line.length - 1)
                val fieldItem = clsItem?.findField(field, true, true)
                curr = fieldItem
            } else if (line.startsWith("parameter #")) {
                val parameterIndex = line.substring("parameter #".length, line.length - 1).toInt()
                val parameterItem = if (methodItem != null) {
                    methodItem.parameters()[parameterIndex]
                } else {
                    null
                }
                curr = parameterItem
            } else if (line.startsWith("type: ")) {
                val typeAnnotation = line.substring("type: ".length)
                if (curr != null) {
                    mergeJaifAnnotation(path, curr, typeAnnotation)
                }
            } else if (line.startsWith("return: ")) {
                val annotation = line.substring("return: ".length)
                if (methodItem != null) {
                    mergeJaifAnnotation(path, methodItem, annotation)
                }
            } else if (line.startsWith("inner-type")) {
                warning("$path: Skipping inner-type annotations for now ($line)")
            } else if (line.startsWith("int ")) {
                // warning("Skipping int attribute definitions for annotations now ($line)")
            }
        }
    }

    private fun mergeJaifAnnotation(
        path: String,
        item: Item,
        annotationSource: String
    ) {
        if (annotationSource.isEmpty()) {
            return
        }

        if (annotationSource.contains("(")) {
            warning("$path: Can't merge complex annotations from jaif yet: $annotationSource")
            return
        }
        val originalName = annotationSource.substring(1) // remove "@"
        val qualifiedName = AnnotationItem.mapName(codebase, originalName) ?: originalName
        if (hasNullnessConflicts(item, qualifiedName)) {
            return
        }

        val annotationItem = codebase.createAnnotation("@$qualifiedName")
        item.mutableModifiers().addAnnotation(annotationItem)
    }

    internal fun error(message: String) {
        // TODO: Integrate with metalava error facility
        options.stderr.println("Error: $message")
    }

    internal fun warning(message: String) {
        if (options.verbose) {
            options.stdout.println("Warning: $message")
        }
    }

    @Suppress("PrivatePropertyName")
    private val XML_SIGNATURE: Pattern = Pattern.compile(
        // Class (FieldName | Type? Name(ArgList) Argnum?)
        // "(\\S+) (\\S+|(.*)\\s+(\\S+)\\((.*)\\)( \\d+)?)");
        "(\\S+) (\\S+|((.*)\\s+)?(\\S+)\\((.*)\\)( \\d+)?)"
    )

    private fun mergeDocument(document: Document) {

        val root = document.documentElement
        val rootTag = root.tagName
        assert(rootTag == "root") { rootTag }

        for (item in getChildren(root)) {
            var signature: String? = item.getAttribute(ATTR_NAME)
            if (signature == null || signature == "null") {
                continue // malformed item
            }

            signature = unescapeXml(signature)
            if (signature == "java.util.Calendar int get(int)") {
                // https://youtrack.jetbrains.com/issue/IDEA-137385
                continue
            } else if (signature == "java.util.Calendar void set(int, int, int) 1" ||
                signature == "java.util.Calendar void set(int, int, int, int, int) 1" ||
                signature == "java.util.Calendar void set(int, int, int, int, int, int) 1"
            ) {
                // http://b.android.com/76090
                continue
            }

            val matcher = XML_SIGNATURE.matcher(signature)
            if (matcher.matches()) {
                val containingClass = matcher.group(1)
                if (containingClass == null) {
                    warning("Could not find class for $signature")
                    continue
                }

                val classItem = codebase.findClass(containingClass)
                if (classItem == null) {
                    // Well known exceptions from IntelliJ's external annotations
                    // we won't complain loudly about
                    if (wellKnownIgnoredImport(containingClass)) {
                        continue
                    }

                    warning("Could not find class $containingClass; omitting annotation from merge")
                    continue
                }

                val methodName = matcher.group(5)
                if (methodName != null) {
                    val parameters = matcher.group(6)
                    val parameterIndex =
                        if (matcher.group(7) != null) {
                            Integer.parseInt(matcher.group(7).trim())
                        } else {
                            -1
                        }
                    mergeMethodOrParameter(item, containingClass, classItem, methodName, parameterIndex, parameters)
                } else {
                    val fieldName = matcher.group(2)
                    mergeField(item, containingClass, classItem, fieldName)
                }
            } else if (signature.indexOf(' ') == -1 && signature.indexOf('.') != -1) {
                // Must be just a class
                val containingClass = signature
                val classItem = codebase.findClass(containingClass)
                if (classItem == null) {
                    if (wellKnownIgnoredImport(containingClass)) {
                        continue
                    }

                    warning("Could not find class $containingClass; omitting annotation from merge")
                    continue
                }

                mergeAnnotations(item, classItem)
            } else {
                warning("No merge match for signature $signature")
            }
        }
    }

    private fun wellKnownIgnoredImport(containingClass: String): Boolean {
        if (containingClass.startsWith("javax.swing.") ||
            containingClass.startsWith("javax.naming.") ||
            containingClass.startsWith("java.awt.") ||
            containingClass.startsWith("org.jdom.")
        ) {
            return true
        }
        return false
    }

    // The parameter declaration used in XML files should not have duplicated spaces,
    // and there should be no space after commas (we can't however strip out all spaces,
    // since for example the spaces around the "extends" keyword needs to be there in
    // types like Map<String,? extends Number>
    private fun fixParameterString(parameters: String): String {
        return parameters.replace("  ", " ").replace(", ", ",").replace("?super", "? super ")
            .replace("?extends", "? extends ")
    }

    private fun mergeMethodOrParameter(
        item: Element,
        containingClass: String,
        classItem: ClassItem,
        methodName: String,
        parameterIndex: Int,
        parameters: String
    ) {
        @Suppress("NAME_SHADOWING")
        val parameters = fixParameterString(parameters)

        val methodItem: MethodItem? = classItem.findMethod(methodName, parameters)
        if (methodItem == null) {
            if (wellKnownIgnoredImport(containingClass)) {
                return
            }

            warning("Could not find method $methodName($parameters) in $containingClass; omitting annotation from merge")
            return
        }

        if (parameterIndex != -1) {
            val parameterItem = methodItem.parameters()[parameterIndex]

            if ("java.util.Calendar" == containingClass && "set" == methodName &&
                parameterIndex > 0
            ) {
                // Skip the metadata for Calendar.set(int, int, int+); see
                // https://code.google.com/p/android/issues/detail?id=73982
                return
            }

            mergeAnnotations(item, parameterItem)
        } else {
            // Annotation on the method itself
            mergeAnnotations(item, methodItem)
        }
    }

    private fun mergeField(item: Element, containingClass: String, classItem: ClassItem, fieldName: String) {

        val fieldItem = classItem.findField(fieldName)
        if (fieldItem == null) {
            if (wellKnownIgnoredImport(containingClass)) {
                return
            }

            warning("Could not find field $fieldName in $containingClass; omitting annotation from merge")
            return
        }

        mergeAnnotations(item, fieldItem)
    }

    private fun getAnnotationName(element: Element): String {
        val tagName = element.tagName
        assert(tagName == "annotation") { tagName }

        val qualifiedName = element.getAttribute(ATTR_NAME)
        assert(qualifiedName != null && !qualifiedName.isEmpty())
        return qualifiedName
    }

    private fun mergeAnnotations(xmlElement: Element, item: Item) {
        loop@ for (annotationElement in getChildren(xmlElement)) {
            val originalName = getAnnotationName(annotationElement)
            val qualifiedName = AnnotationItem.mapName(codebase, originalName) ?: originalName
            if (hasNullnessConflicts(item, qualifiedName)) {
                continue@loop
            }

            val annotationItem = createAnnotation(annotationElement) ?: continue
            item.mutableModifiers().addAnnotation(annotationItem)
        }
    }

    private fun hasNullnessConflicts(
        item: Item,
        qualifiedName: String
    ): Boolean {
        var haveNullable = false
        var haveNotNull = false
        for (existing in item.modifiers.annotations()) {
            val name = existing.qualifiedName() ?: continue
            if (isNonNull(name)) {
                haveNotNull = true
            }
            if (isNullable(name)) {
                haveNullable = true
            }
            if (name == qualifiedName) {
                return true
            }
        }

        // Make sure we don't have a conflict between nullable and not nullable
        if (isNonNull(qualifiedName) && haveNullable || isNullable(qualifiedName) && haveNotNull) {
            warning("Found both @Nullable and @NonNull after import for $item")
            return true
        }
        return false
    }

    /** Reads in annotation data from an XML item (using IntelliJ IDE's external annotations XML format) and
     * creates a corresponding [AnnotationItem], performing some "translations" in the process (e.g. mapping
     * from IntelliJ annotations like `org.jetbrains.annotations.Nullable` to `android.support.annotation.Nullable`. */
    private fun createAnnotation(annotationElement: Element): AnnotationItem? {
        val tagName = annotationElement.tagName
        assert(tagName == "annotation") { tagName }
        val name = annotationElement.getAttribute(ATTR_NAME)
        assert(name != null && !name.isEmpty())
        when {
            name == "org.jetbrains.annotations.Range" -> {
                val children = getChildren(annotationElement)
                assert(children.size == 2) { children.size }
                val valueElement1 = children[0]
                val valueElement2 = children[1]
                val valName1 = valueElement1.getAttribute(ATTR_NAME)
                val value1 = valueElement1.getAttribute(ATTR_VAL)
                val valName2 = valueElement2.getAttribute(ATTR_NAME)
                val value2 = valueElement2.getAttribute(ATTR_VAL)
                return PsiAnnotationItem.create(
                    codebase, XmlBackedAnnotationItem(
                        codebase, AnnotationDetector.INT_RANGE_ANNOTATION.newName(),
                        listOf(
                            // Add "L" suffix to ensure that we don't for example interpret "-1" as
                            // an integer -1 and then end up recording it as "ffffffff" instead of -1L
                            XmlBackedAnnotationAttribute(
                                valName1,
                                value1 + (if (value1.last().isDigit()) "L" else "")
                            ),
                            XmlBackedAnnotationAttribute(
                                valName2,
                                value2 + (if (value2.last().isDigit()) "L" else "")
                            )
                        )
                    )
                )
            }
            name == IDEA_MAGIC -> {
                val children = getChildren(annotationElement)
                assert(children.size == 1) { children.size }
                val valueElement = children[0]
                val valName = valueElement.getAttribute(ATTR_NAME)
                var value = valueElement.getAttribute(ATTR_VAL)
                val flagsFromClass = valName == "flagsFromClass"
                val flag = valName == "flags" || flagsFromClass
                if (valName == "valuesFromClass" || flagsFromClass) {
                    // Not supported
                    var found = false
                    if (value.endsWith(DOT_CLASS)) {
                        val clsName = value.substring(0, value.length - DOT_CLASS.length)
                        val sb = StringBuilder()
                        sb.append('{')

                        var reflectionFields: Array<Field>? = null
                        try {
                            val cls = Class.forName(clsName)
                            reflectionFields = cls.declaredFields
                        } catch (ignore: Exception) {
                            // Class not available: not a problem. We'll rely on API filter.
                            // It's mainly used for sorting anyway.
                        }

                        // Attempt to sort in reflection order
                        if (!found && reflectionFields != null) {
                            val filterEmit = ApiVisitor(codebase).filterEmit

                            // Attempt with reflection
                            var first = true
                            for (field in reflectionFields) {
                                if (field.type == Integer.TYPE || field.type == Int::class.javaPrimitiveType) {
                                    // Make sure this field is included in our API too
                                    val fieldItem = codebase.findClass(clsName)?.findField(field.name)
                                    if (fieldItem == null || !filterEmit.test(fieldItem)) {
                                        continue
                                    }

                                    if (first) {
                                        first = false
                                    } else {
                                        sb.append(',').append(' ')
                                    }
                                    sb.append(clsName).append('.').append(field.name)
                                }
                            }
                        }
                        sb.append('}')
                        value = sb.toString()
                        if (sb.length > 2) { // 2: { }
                            found = true
                        }
                    }

                    if (!found) {
                        return null
                    }
                }

                val attributes = mutableListOf<XmlBackedAnnotationAttribute>()
                attributes.add(XmlBackedAnnotationAttribute(TYPE_DEF_VALUE_ATTRIBUTE, value))
                if (flag) {
                    attributes.add(XmlBackedAnnotationAttribute(TYPE_DEF_FLAG_ATTRIBUTE, VALUE_TRUE))
                }
                return PsiAnnotationItem.create(
                    codebase, XmlBackedAnnotationItem(
                        codebase,
                        if (valName == "stringValues") STRING_DEF_ANNOTATION.newName() else INT_DEF_ANNOTATION.newName(),
                        attributes
                    )
                )
            }

            name == STRING_DEF_ANNOTATION.oldName() ||
                name == STRING_DEF_ANNOTATION.newName() ||
                name == ANDROID_STRING_DEF ||
                name == INT_DEF_ANNOTATION.oldName() ||
                name == INT_DEF_ANNOTATION.newName() ||
                name == ANDROID_INT_DEF -> {
                val children = getChildren(annotationElement)
                var valueElement = children[0]
                val valName = valueElement.getAttribute(ATTR_NAME)
                assert(TYPE_DEF_VALUE_ATTRIBUTE == valName)
                val value = valueElement.getAttribute(ATTR_VAL)
                var flag = false
                if (children.size == 2) {
                    valueElement = children[1]
                    assert(TYPE_DEF_FLAG_ATTRIBUTE == valueElement.getAttribute(ATTR_NAME))
                    flag = VALUE_TRUE == valueElement.getAttribute(ATTR_VAL)
                }
                val intDef = INT_DEF_ANNOTATION.oldName() == name ||
                    INT_DEF_ANNOTATION.newName() == name ||
                    ANDROID_INT_DEF == name

                val attributes = mutableListOf<XmlBackedAnnotationAttribute>()
                attributes.add(XmlBackedAnnotationAttribute(TYPE_DEF_VALUE_ATTRIBUTE, value))
                if (flag) {
                    attributes.add(XmlBackedAnnotationAttribute(TYPE_DEF_FLAG_ATTRIBUTE, VALUE_TRUE))
                }
                return PsiAnnotationItem.create(
                    codebase, XmlBackedAnnotationItem(
                        codebase,
                        if (intDef) INT_DEF_ANNOTATION.newName() else STRING_DEF_ANNOTATION.newName(), attributes
                    )
                )
            }

            name == IDEA_CONTRACT -> {
                val children = getChildren(annotationElement)
                val valueElement = children[0]
                val value = valueElement.getAttribute(ATTR_VAL)
                val pure = valueElement.getAttribute(ATTR_PURE)
                return if (pure != null && !pure.isEmpty()) {
                    PsiAnnotationItem.create(
                        codebase, XmlBackedAnnotationItem(
                            codebase, name,
                            listOf(
                                XmlBackedAnnotationAttribute(TYPE_DEF_VALUE_ATTRIBUTE, value),
                                XmlBackedAnnotationAttribute(ATTR_PURE, pure)
                            )
                        )
                    )
                } else {
                    PsiAnnotationItem.create(
                        codebase, XmlBackedAnnotationItem(
                            codebase, name,
                            listOf(XmlBackedAnnotationAttribute(TYPE_DEF_VALUE_ATTRIBUTE, value))
                        )
                    )
                }
            }

            isNonNull(name) -> return codebase.createAnnotation("@$ANDROIDX_NOTNULL")

            isNullable(name) -> return codebase.createAnnotation("@$ANDROIDX_NULLABLE")

            else -> {
                val children = getChildren(annotationElement)
                if (children.isEmpty()) {
                    return codebase.createAnnotation("@$name")
                }
                val attributes = mutableListOf<XmlBackedAnnotationAttribute>()
                for (valueElement in children) {
                    attributes.add(
                        XmlBackedAnnotationAttribute(
                            valueElement.getAttribute(ATTR_NAME) ?: continue,
                            valueElement.getAttribute(ATTR_VAL) ?: continue
                        )
                    )
                }
                return PsiAnnotationItem.create(codebase, XmlBackedAnnotationItem(codebase, name, attributes))
            }
        }
    }

    private fun isNonNull(name: String): Boolean {
        return name == IDEA_NOTNULL ||
            name == ANDROID_NOTNULL ||
            name == ANDROIDX_NOTNULL ||
            name == SUPPORT_NOTNULL
    }

    private fun isNullable(name: String): Boolean {
        return name == IDEA_NULLABLE ||
            name == ANDROID_NULLABLE ||
            name == ANDROIDX_NULLABLE ||
            name == SUPPORT_NULLABLE
    }

    private fun unescapeXml(escaped: String): String {
        var workingString = escaped.replace(QUOT_ENTITY, "\"")
        workingString = workingString.replace(LT_ENTITY, "<")
        workingString = workingString.replace(GT_ENTITY, ">")
        workingString = workingString.replace(APOS_ENTITY, "'")
        workingString = workingString.replace(AMP_ENTITY, "&")

        return workingString
    }
}

// TODO: Replace with usage of DefaultAnnotationValue?
data class XmlBackedAnnotationAttribute(
    override val name: String,
    private val valueLiteral: String
) : AnnotationAttribute {
    override val value: AnnotationAttributeValue = DefaultAnnotationValue.create(valueLiteral)

    override fun toString(): String {
        return "$name=$valueLiteral"
    }
}

// TODO: Replace with usage of DefaultAnnotationAttribute?
class XmlBackedAnnotationItem(
    override var codebase: Codebase,
    private val qualifiedName: String,
    private val attributes: List<XmlBackedAnnotationAttribute> = emptyList()
) : AnnotationItem {
    override fun qualifiedName(): String? = AnnotationItem.mapName(codebase, qualifiedName)

    override fun attributes() = attributes

    override fun toSource(): String {
        val qualifiedName = qualifiedName() ?: return ""

        if (attributes.isEmpty()) {
            return "@$qualifiedName"
        }

        val sb = StringBuilder(30)
        sb.append("@")
        sb.append(qualifiedName)
        sb.append("(")
        attributes.joinTo(sb)
        sb.append(")")

        return sb.toString()
    }
}
