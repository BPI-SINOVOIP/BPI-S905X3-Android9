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

import com.android.SdkConstants
import com.android.tools.lint.annotations.Extractor
import com.android.tools.lint.client.api.AnnotationLookup
import com.android.tools.lint.detector.api.ConstantEvaluator
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MemberItem
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.psi.PsiAnnotationItem
import com.android.tools.metalava.model.psi.PsiClassItem
import com.android.tools.metalava.model.visitors.ApiVisitor
import com.android.utils.XmlUtils
import com.google.common.base.Charsets
import com.google.common.xml.XmlEscapers
import com.intellij.psi.PsiAnnotation
import com.intellij.psi.PsiClass
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiField
import com.intellij.psi.PsiNameValuePair
import org.jetbrains.uast.UAnnotation
import org.jetbrains.uast.UBinaryExpressionWithType
import org.jetbrains.uast.UCallExpression
import org.jetbrains.uast.UExpression
import org.jetbrains.uast.ULiteralExpression
import org.jetbrains.uast.UNamedExpression
import org.jetbrains.uast.UReferenceExpression
import org.jetbrains.uast.UastEmptyExpression
import org.jetbrains.uast.java.JavaUAnnotation
import org.jetbrains.uast.java.expressions.JavaUAnnotationCallExpression
import org.jetbrains.uast.util.isArrayInitializer
import org.jetbrains.uast.util.isTypeCast
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.io.PrintWriter
import java.io.StringWriter
import java.util.ArrayList
import java.util.jar.JarEntry
import java.util.jar.JarOutputStream

// Like the tools/base Extractor class, but limited to our own (mapped) AnnotationItems,
// and only those with source retention (and in particular right now that just means the
// typedef annotations.)
class ExtractAnnotations(
    private val codebase: Codebase,
    private val outputFile: File
) : ApiVisitor(codebase) {
    // Used linked hash map for order such that we always emit parameters after their surrounding method etc
    private val packageToAnnotationPairs = LinkedHashMap<PackageItem, MutableList<Pair<Item, AnnotationHolder>>>()

    private val annotationLookup = AnnotationLookup()

    private data class AnnotationHolder(
        val annotationClass: ClassItem?,
        val annotationItem: AnnotationItem,
        val uAnnotation: UAnnotation?
    )

    private val classToAnnotationHolder = mutableMapOf<String, AnnotationHolder>()

    fun extractAnnotations() {
        codebase.accept(this)

        // Write external annotations
        FileOutputStream(outputFile).use { fileOutputStream ->
            JarOutputStream(BufferedOutputStream(fileOutputStream)).use { zos ->
                val sortedPackages =
                    packageToAnnotationPairs.keys.asSequence().sortedBy { it.qualifiedName() }.toList()

                for (pkg in sortedPackages) {
                    // Note: Using / rather than File.separator: jar lib requires it
                    val name = pkg.qualifiedName().replace('.', '/') + "/annotations.xml"

                    val outEntry = JarEntry(name)
                    outEntry.time = 0
                    zos.putNextEntry(outEntry)

                    val pairs = packageToAnnotationPairs[pkg] ?: continue

                    StringPrintWriter.create().use { writer ->
                        writer.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>")

                        var prev: Item? = null
                        for ((item, annotation) in pairs) {
                            // that we only do analysis for IntDef/LongDef
                            assert(item != prev) // should be only one annotation per element now
                            prev = item

                            writer.print("  <item name=\"")
                            writer.print(item.getExternalAnnotationSignature())
                            writer.println("\">")

                            writeAnnotation(writer, item, annotation)

                            writer.print("  </item>")
                            writer.println()
                        }

                        writer.println("</root>\n")
                        writer.close()
                        val bytes = writer.contents.toByteArray(Charsets.UTF_8)
                        zos.write(bytes)
                        zos.closeEntry()
                    }
                }
            }
        }
    }

    /** For a given item, extract the relevant annotations for that item.
     *
     * Currently, we're only extracting typedef annotations. Everything else
     * has class retention.
     */
    private fun checkItem(item: Item) {
        // field, method or parameter
        val typedef = findTypeDef(item) ?: return

        val pkg = when (item) {
            is MemberItem -> item.containingClass().containingPackage()
            is ParameterItem -> item.containingMethod().containingClass().containingPackage()
            else -> return
        }

        val list = packageToAnnotationPairs[pkg] ?: run {
            val new =
                mutableListOf<Pair<Item, AnnotationHolder>>()
            packageToAnnotationPairs[pkg] = new
            new
        }
        list.add(Pair(item, typedef))
    }

    override fun visitField(field: FieldItem) {
        checkItem(field)
    }

    override fun visitMethod(method: MethodItem) {
        checkItem(method)
    }

    override fun visitParameter(parameter: ParameterItem) {
        checkItem(parameter)
    }

    private fun findTypeDef(item: Item): AnnotationHolder? {
        for (annotation in item.modifiers.annotations()) {
            val qualifiedName = annotation.qualifiedName() ?: continue
            if (qualifiedName.startsWith(JAVA_LANG_PREFIX) ||
                qualifiedName.startsWith(ANDROIDX_ANNOTATION_PREFIX) ||
                qualifiedName.startsWith(ANDROID_ANNOTATION_PREFIX) ||
                qualifiedName.startsWith(ANDROID_SUPPORT_ANNOTATION_PREFIX)
            ) {
                if (annotation.isTypeDefAnnotation()) {
                    // Imported typedef
                    return AnnotationHolder(null, annotation, null)
                }

                continue
            }

            val typeDefClass = annotation.resolve() ?: continue
            val className = typeDefClass.qualifiedName()
            if (typeDefClass.isAnnotationType()) {
                val cached = classToAnnotationHolder[className]
                if (cached != null) {
                    return cached
                }

                val typeDefAnnotation = typeDefClass.modifiers.annotations().firstOrNull {
                    it.isTypeDefAnnotation()
                }
                if (typeDefAnnotation != null) {
                    // Make sure it has the right retention
                    if (!hasSourceRetention(typeDefClass)) {
                        reporter.report(
                            Errors.ANNOTATION_EXTRACTION, typeDefClass,
                            "This typedef annotation class should have @Retention(RetentionPolicy.SOURCE)"
                        )
                    }

                    if (filterEmit.test(typeDefClass)) {
                        reporter.report(
                            Errors.ANNOTATION_EXTRACTION, typeDefClass,
                            "This typedef annotation class should be marked @hide or should not be marked public"
                        )
                    }

                    if (typeDefAnnotation is PsiAnnotationItem && typeDefClass is PsiClassItem) {
                        val result = AnnotationHolder(
                            typeDefClass, typeDefAnnotation,
                            annotationLookup.findRealAnnotation(
                                typeDefAnnotation.psiAnnotation,
                                typeDefClass.psiClass,
                                null
                            )
                        )
                        classToAnnotationHolder[className] = result
                        return result
                    }
                }
            }
        }
        return null
    }

    private fun hasSourceRetention(annotationClass: ClassItem): Boolean {
        if (annotationClass is PsiClassItem) {
            return hasSourceRetention(annotationClass.psiClass)
        }
        return false
    }

    private fun hasSourceRetention(cls: PsiClass): Boolean {
        val modifierList = cls.modifierList
        if (modifierList != null) {
            for (psiAnnotation in modifierList.annotations) {
                val annotation = JavaUAnnotation.wrap(psiAnnotation)
                if (hasSourceRetention(annotation)) {
                    return true
                }
            }
        }

        return false
    }

    private fun hasSourceRetention(annotation: UAnnotation): Boolean {
        val qualifiedName = annotation.qualifiedName
        if ("java.lang.annotation.Retention" == qualifiedName || "kotlin.annotation.Retention" == qualifiedName) {
            val attributes = annotation.attributeValues
            if (attributes.size != 1) {
                error("Expected exactly one parameter passed to @Retention")
                return false
            }
            val value = attributes[0].expression
            if (value is UReferenceExpression) {
                try {
                    val element = value.resolve()
                    if (element is PsiField) {
                        val field = element as PsiField?
                        if ("SOURCE" == field!!.name) {
                            return true
                        }
                    }
                } catch (t: Throwable) {
                    val s = value.asSourceString()
                    return s.contains("SOURCE")
                }
            }
        }

        return false
    }

    /**
     * A writer which stores all its contents into a string and has the ability to mark a certain
     * freeze point and then reset back to it
     */
    private class StringPrintWriter constructor(private val stringWriter: StringWriter) :
        PrintWriter(stringWriter) {
        private var mark: Int = 0

        val contents: String get() = stringWriter.toString()

        fun mark() {
            flush()
            mark = stringWriter.buffer.length
        }

        fun reset() {
            stringWriter.buffer.setLength(mark)
        }

        override fun toString(): String {
            return contents
        }

        companion object {
            fun create(): StringPrintWriter {
                return StringPrintWriter(StringWriter(1000))
            }
        }
    }

    private fun escapeXml(unescaped: String): String {
        return XmlEscapers.xmlAttributeEscaper().escape(unescaped)
    }

    private fun Item.getExternalAnnotationSignature(): String? {
        when (this) {
            is PackageItem -> {
                return escapeXml(qualifiedName())
            }

            is ClassItem -> {
                return escapeXml(qualifiedName())
            }

            is MethodItem -> {
                val sb = StringBuilder(100)
                sb.append(escapeXml(containingClass().qualifiedName()))
                sb.append(' ')

                if (isConstructor()) {
                    sb.append(escapeXml(containingClass().simpleName()))
                } else if (returnType() != null) {
                    sb.append(escapeXml(returnType()!!.toTypeString()))
                    sb.append(' ')
                    sb.append(escapeXml(name()))
                }

                sb.append('(')

                // The signature must match *exactly* the formatting used by IDEA,
                // since it looks up external annotations in a map by this key.
                // Therefore, it is vital that the parameter list uses exactly one
                // space after each comma between parameters, and *no* spaces between
                // generics variables, e.g. foo(Map<A,B>, int)
                var i = 0
                val parameterList = parameters()
                val n = parameterList.size
                while (i < n) {
                    if (i > 0) {
                        sb.append(',').append(' ')
                    }
                    val type = parameterList[i].type().toTypeString().replace(" ", "")
                    sb.append(type)
                    i++
                }
                sb.append(')')
                return sb.toString()
            }

            is FieldItem -> {
                return escapeXml(containingClass().qualifiedName()) + ' '.toString() + name()
            }

            is ParameterItem -> {
                return containingMethod().getExternalAnnotationSignature() + ' '.toString() + this.parameterIndex
            }
        }

        return null
    }

    private fun writeAnnotation(
        writer: StringPrintWriter,
        item: Item,
        annotationHolder: AnnotationHolder
    ) {
        val annotationItem = annotationHolder.annotationItem
        val qualifiedName = annotationItem.qualifiedName()

        writer.mark()
        writer.print("    <annotation name=\"")
        writer.print(qualifiedName)

        writer.print("\">")
        writer.println()

        val uAnnotation = annotationHolder.uAnnotation
            ?: if (annotationItem is PsiAnnotationItem) {
                // Imported annotation
                JavaUAnnotation.wrap(annotationItem.psiAnnotation)
            } else {
                null
            }
        if (uAnnotation != null) {
            var attributes = uAnnotation.attributeValues

            // noinspection PointlessBooleanExpression,ConstantConditions
            if (attributes.size > 1 && sortAnnotations) {
                // Ensure mutable
                attributes = ArrayList(attributes)

                // Ensure that the value attribute is written first
                attributes.sortedWith(object : Comparator<UNamedExpression> {
                    private fun getName(pair: UNamedExpression): String {
                        val name = pair.name
                        return name ?: SdkConstants.ATTR_VALUE
                    }

                    private fun rank(pair: UNamedExpression): Int {
                        return if (SdkConstants.ATTR_VALUE == getName(pair)) -1 else 0
                    }

                    override fun compare(o1: UNamedExpression, o2: UNamedExpression): Int {
                        val r1 = rank(o1)
                        val r2 = rank(o2)
                        val delta = r1 - r2
                        return if (delta != 0) {
                            delta
                        } else getName(o1).compareTo(getName(o2))
                    }
                })
            }

            if (attributes.size == 1 && Extractor.REQUIRES_PERMISSION.isPrefix(qualifiedName, true)) {
                val expression = attributes[0].expression
                if (expression is UAnnotation) {
                    // The external annotations format does not allow for nested/complex annotations.
                    // However, these special annotations (@RequiresPermission.Read,
                    // @RequiresPermission.Write, etc) are known to only be simple containers with a
                    // single permission child, so instead we "inline" the content:
                    //  @Read(@RequiresPermission(allOf={P1,P2},conditional=true)
                    //     =>
                    //      @RequiresPermission.Read(allOf({P1,P2},conditional=true)
                    // That's setting attributes that don't actually exist on the container permission,
                    // but we'll counteract that on the read-annotations side.
                    val annotation = expression as UAnnotation
                    attributes = annotation.attributeValues
                } else if (expression is JavaUAnnotationCallExpression) {
                    val annotation = expression.uAnnotation
                    attributes = annotation.attributeValues
                } else if (expression is UastEmptyExpression && attributes[0].sourcePsi is PsiNameValuePair) {
                    val memberValue = (attributes[0].sourcePsi as PsiNameValuePair).value
                    if (memberValue is PsiAnnotation) {
                        val annotation = JavaUAnnotation.wrap(memberValue)
                        attributes = annotation.attributeValues
                    }
                }
            }

            val inlineConstants = isInlinedConstant(annotationItem)
            var empty = true
            for (pair in attributes) {
                val expression = pair.expression
                val value = attributeString(expression, inlineConstants) ?: continue
                empty = false
                var name = pair.name
                if (name == null) {
                    name = SdkConstants.ATTR_VALUE // default name
                }

                // Platform typedef annotations now declare a prefix attribute for
                // documentation generation purposes; this should not be part of the
                // extracted metadata.
                if (("prefix" == name || "suffix" == name) && annotationItem.isTypeDefAnnotation()) {
                    reporter.report(
                        Errors.SUPERFLUOUS_PREFIX, item,
                        "Superfluous $name attribute on typedef"
                    )
                    continue
                }

                writer.print("      <val name=\"")
                writer.print(name)
                writer.print("\" val=\"")
                writer.print(escapeXml(value))
                writer.println("\" />")
            }

            if (empty) {
                // All items were filtered out: don't write the annotation at all
                writer.reset()
                return
            }
        }

        writer.println("    </annotation>")
    }

    private fun attributeString(value: UExpression?, inlineConstants: Boolean): String? {
        value ?: return null
        val sb = StringBuilder()
        return if (appendExpression(sb, value, inlineConstants)) {
            sb.toString()
        } else {
            null
        }
    }

    private fun appendExpression(
        sb: StringBuilder,
        expression: UExpression,
        inlineConstants: Boolean
    ): Boolean {
        if (expression.isArrayInitializer()) {
            val call = expression as UCallExpression
            val initializers = call.valueArguments
            sb.append('{')
            var first = true
            val initialLength = sb.length
            for (e in initializers) {
                val length = sb.length
                if (first) {
                    first = false
                } else {
                    sb.append(", ")
                }
                val appended = appendExpression(sb, e, inlineConstants)
                if (!appended) {
                    // trunk off comma if it bailed for some reason (e.g. constant
                    // filtered out by API etc)
                    sb.setLength(length)
                    if (length == initialLength) {
                        first = true
                    }
                }
            }
            sb.append('}')
            return sb.length != 2
        } else if (expression is UReferenceExpression) {
            val resolved = expression.resolve()
            if (resolved is PsiField) {
                val field = resolved as PsiField?
                if (!inlineConstants) {
                    // Inline constants
                    val value = field!!.computeConstantValue()
                    if (appendLiteralValue(sb, value)) {
                        return true
                    }
                }

                val declaringClass = field!!.containingClass
                if (declaringClass == null) {
                    error("No containing class found for " + field.name)
                    return false
                }
                val qualifiedName = declaringClass.qualifiedName
                val fieldName = field.name

                if (qualifiedName != null) {
                    val cls = codebase.findClass(qualifiedName)
                    val fld = cls?.findField(fieldName, true)
                    if (fld == null || !filterReference.test(fld)) {
                        // This field is not visible: remove from typedef
                        if (fld != null) {
                            reporter.report(
                                Errors.HIDDEN_TYPEDEF_CONSTANT, fld,
                                "Typedef class references hidden field $fld: removed from typedef metadata"
                            )
                        }
                        return false
                    }
                    sb.append(qualifiedName)
                    sb.append('.')
                    sb.append(fieldName)
                    return true
                }
                return false
            } else {
                warning("Unexpected reference to $expression")
                return false
            }
        } else if (expression is ULiteralExpression) {
            val literalValue = expression.value
            if (appendLiteralValue(sb, literalValue)) {
                return true
            }
        } else if (expression is UBinaryExpressionWithType) {
            if ((expression).isTypeCast()) {
                val operand = expression.operand
                return appendExpression(sb, operand, inlineConstants)
            }
            return false
        }

        // For example, binary expressions like 3 + 4
        val literalValue = ConstantEvaluator.evaluate(null, expression)
        if (literalValue != null) {
            if (appendLiteralValue(sb, literalValue)) {
                return true
            }
        }

        warning("Unexpected annotation expression of type ${expression.javaClass} and is $expression")

        return false
    }

    private fun appendLiteralValue(sb: StringBuilder, literalValue: Any?): Boolean {
        if (literalValue is Number || literalValue is Boolean) {
            sb.append(literalValue.toString())
            return true
        } else if (literalValue is String || literalValue is Char) {
            sb.append('"')
            XmlUtils.appendXmlAttributeValue(sb, literalValue.toString())
            sb.append('"')
            return true
        }
        return false
    }

    private fun isInlinedConstant(annotationItem: AnnotationItem): Boolean {
        return annotationItem.isTypeDefAnnotation()
    }

    /** Whether to sort annotation attributes (otherwise their declaration order is used)  */
    private val sortAnnotations: Boolean = true

    private fun warning(string: String) {
        reporter.report(Severity.ERROR, null as PsiElement?, string, Errors.ANNOTATION_EXTRACTION)
    }

    private fun error(string: String) {
        reporter.report(Severity.WARNING, null as PsiElement?, string, Errors.ANNOTATION_EXTRACTION)
    }
}
