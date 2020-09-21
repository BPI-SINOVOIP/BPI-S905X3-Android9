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

package com.android.tools.metalava.model.psi

import com.android.tools.lint.detector.api.getInternalName
import com.android.tools.metalava.compatibility
import com.android.tools.metalava.doclava1.ApiPredicate
import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MemberItem
import com.android.tools.metalava.model.TypeItem
import com.android.tools.metalava.model.TypeParameterItem
import com.android.tools.metalava.model.text.TextTypeItem
import com.intellij.psi.JavaTokenType
import com.intellij.psi.PsiArrayType
import com.intellij.psi.PsiCapturedWildcardType
import com.intellij.psi.PsiClass
import com.intellij.psi.PsiClassType
import com.intellij.psi.PsiCompiledElement
import com.intellij.psi.PsiDisjunctionType
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiEllipsisType
import com.intellij.psi.PsiIntersectionType
import com.intellij.psi.PsiJavaCodeReferenceElement
import com.intellij.psi.PsiJavaToken
import com.intellij.psi.PsiLambdaExpressionType
import com.intellij.psi.PsiPrimitiveType
import com.intellij.psi.PsiRecursiveElementVisitor
import com.intellij.psi.PsiReferenceList
import com.intellij.psi.PsiType
import com.intellij.psi.PsiTypeElement
import com.intellij.psi.PsiTypeParameter
import com.intellij.psi.PsiTypeParameterList
import com.intellij.psi.PsiTypeVisitor
import com.intellij.psi.PsiWhiteSpace
import com.intellij.psi.PsiWildcardType
import com.intellij.psi.util.PsiTypesUtil
import com.intellij.psi.util.TypeConversionUtil

/** Represents a type backed by PSI */
class PsiTypeItem private constructor(private val codebase: PsiBasedCodebase, private val psiType: PsiType) : TypeItem {
    private var toString: String? = null
    private var toAnnotatedString: String? = null
    private var toInnerAnnotatedString: String? = null
    private var toErasedString: String? = null
    private var asClass: PsiClassItem? = null

    override fun toString(): String {
        return toTypeString()
    }

    override fun toTypeString(
        outerAnnotations: Boolean,
        innerAnnotations: Boolean,
        erased: Boolean
    ): String {
        assert(innerAnnotations || !outerAnnotations) // Can't supply outer=true,inner=false

        return if (erased) {
            if (innerAnnotations || outerAnnotations) {
                // Not cached: Not common
                toTypeString(codebase, psiType, outerAnnotations, innerAnnotations, erased)
            } else {
                if (toErasedString == null) {
                    toErasedString = toTypeString(codebase, psiType, outerAnnotations, innerAnnotations, erased)
                }
                toErasedString!!
            }
        } else {
            when {
                outerAnnotations -> {
                    if (toAnnotatedString == null) {
                        toAnnotatedString = TypeItem.formatType(
                            toTypeString(
                                codebase,
                                psiType,
                                outerAnnotations,
                                innerAnnotations,
                                erased
                            )
                        )
                    }
                    toAnnotatedString!!
                }
                innerAnnotations -> {
                    if (toInnerAnnotatedString == null) {
                        toInnerAnnotatedString = TypeItem.formatType(
                            toTypeString(
                                codebase,
                                psiType,
                                outerAnnotations,
                                innerAnnotations,
                                erased
                            )
                        )
                    }
                    toInnerAnnotatedString!!
                }
                else -> {
                    if (toString == null) {
                        toString = TypeItem.formatType(getCanonicalText(psiType, annotated = false))
                    }
                    toString!!
                }
            }
        }
    }

    override fun toErasedTypeString(): String {
        return toTypeString(outerAnnotations = false, innerAnnotations = false, erased = true)
    }

    override fun arrayDimensions(): Int {
        return psiType.arrayDimensions
    }

    override fun internalName(): String {
        if (primitive) {
            val signature = getPrimitiveSignature(toString())
            if (signature != null) {
                return signature
            }
        }
        val sb = StringBuilder()
        appendJvmSignature(sb, psiType)
        return sb.toString()
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true

        return when (other) {
            is TypeItem -> toTypeString().replace(" ", "") == other.toTypeString().replace(" ", "")
            else -> false
        }
    }

    override fun asClass(): PsiClassItem? {
        if (asClass == null) {
            asClass = codebase.findClass(psiType)
        }
        return asClass
    }

    override fun asTypeParameter(context: MemberItem?): TypeParameterItem? {
        val cls = asClass() ?: return null
        return cls as? PsiTypeParameterItem
    }

    override fun hashCode(): Int {
        return psiType.hashCode()
    }

    override val primitive: Boolean
        get() = psiType is PsiPrimitiveType

    override fun defaultValue(): Any? {
        return PsiTypesUtil.getDefaultValue(psiType)
    }

    override fun defaultValueString(): String {
        return PsiTypesUtil.getDefaultValueOfType(psiType)
    }

    override fun typeArgumentClasses(): List<ClassItem> {
        if (primitive) {
            return emptyList()
        }

        val classes = mutableListOf<ClassItem>()
        psiType.accept(object : PsiTypeVisitor<PsiType>() {
            override fun visitType(type: PsiType?): PsiType? {
                return type
            }

            override fun visitClassType(classType: PsiClassType): PsiType? {
                codebase.findClass(classType)?.let {
                    if (!it.isTypeParameter && !classes.contains(it)) {
                        classes.add(it)
                    }
                }
                for (type in classType.parameters) {
                    type.accept(this)
                }
                return classType
            }

            override fun visitWildcardType(wildcardType: PsiWildcardType): PsiType? {
                if (wildcardType.isExtends) {
                    wildcardType.extendsBound.accept(this)
                }
                if (wildcardType.isSuper) {
                    wildcardType.superBound.accept(this)
                }
                if (wildcardType.isBounded) {
                    wildcardType.bound?.accept(this)
                }
                return wildcardType
            }

            override fun visitPrimitiveType(primitiveType: PsiPrimitiveType): PsiType? {
                return primitiveType
            }

            override fun visitEllipsisType(ellipsisType: PsiEllipsisType): PsiType? {
                ellipsisType.componentType.accept(this)
                return ellipsisType
            }

            override fun visitArrayType(arrayType: PsiArrayType): PsiType? {
                arrayType.componentType.accept(this)
                return arrayType
            }

            override fun visitLambdaExpressionType(lambdaExpressionType: PsiLambdaExpressionType): PsiType? {
                for (superType in lambdaExpressionType.superTypes) {
                    superType.accept(this)
                }
                return lambdaExpressionType
            }

            override fun visitCapturedWildcardType(capturedWildcardType: PsiCapturedWildcardType): PsiType? {
                capturedWildcardType.upperBound.accept(this)
                return capturedWildcardType
            }

            override fun visitDisjunctionType(disjunctionType: PsiDisjunctionType): PsiType? {
                for (type in disjunctionType.disjunctions) {
                    type.accept(this)
                }
                return disjunctionType
            }

            override fun visitIntersectionType(intersectionType: PsiIntersectionType): PsiType? {
                for (type in intersectionType.conjuncts) {
                    type.accept(this)
                }
                return intersectionType
            }
        })

        return classes
    }

    override fun convertType(replacementMap: Map<String, String>?, owner: Item?): TypeItem {
        val s = convertTypeString(replacementMap)
        return create(codebase, codebase.createPsiType(s, owner?.psi()))
    }

    override fun hasTypeArguments(): Boolean = psiType is PsiClassType && psiType.hasParameters()

    override fun markRecent() {
        toAnnotatedString = toTypeString(false, true, false).replace(".NonNull", ".RecentlyNonNull")
        toInnerAnnotatedString = toTypeString(true, true, false).replace(".NonNull", ".RecentlyNonNull")
    }

    companion object {
        private fun getPrimitiveSignature(typeName: String): String? = when (typeName) {
            "boolean" -> "Z"
            "byte" -> "B"
            "char" -> "C"
            "short" -> "S"
            "int" -> "I"
            "long" -> "J"
            "float" -> "F"
            "double" -> "D"
            "void" -> "V"
            else -> null
        }

        private fun appendJvmSignature(
            buffer: StringBuilder,
            type: PsiType?
        ): Boolean {
            if (type == null) {
                return false
            }

            val psiType = TypeConversionUtil.erasure(type)

            when (psiType) {
                is PsiArrayType -> {
                    buffer.append('[')
                    appendJvmSignature(buffer, psiType.componentType)
                }
                is PsiClassType -> {
                    val resolved = psiType.resolve() ?: return false
                    if (!appendJvmTypeName(buffer, resolved)) {
                        return false
                    }
                }
                is PsiPrimitiveType -> buffer.append(getPrimitiveSignature(psiType.canonicalText))
                else -> return false
            }
            return true
        }

        private fun appendJvmTypeName(
            signature: StringBuilder,
            outerClass: PsiClass
        ): Boolean {
            val className = getInternalName(outerClass) ?: return false
            signature.append('L').append(className).append(';')
            return true
        }

        fun toTypeString(
            codebase: Codebase,
            type: PsiType,
            outerAnnotations: Boolean,
            innerAnnotations: Boolean,
            erased: Boolean
        ): String {

            if (erased) {
                // Recurse with raw type and erase=false
                return toTypeString(
                    codebase,
                    TypeConversionUtil.erasure(type),
                    outerAnnotations,
                    innerAnnotations,
                    false
                )
            }

            if (outerAnnotations || innerAnnotations) {
                val typeString = mapAnnotations(codebase, getCanonicalText(type, true))
                if (!outerAnnotations && typeString.contains("@")) {
                    // Temporary hack: should use PSI type visitor instead
                    return TextTypeItem.eraseAnnotations(typeString, false, true)
                }
                return typeString
            } else {
                return type.canonicalText
            }
        }

        /**
         * Replace annotations in the given type string with the mapped qualified names
         * to [AnnotationItem.mapName]
         */
        private fun mapAnnotations(codebase: Codebase, string: String): String {
            var s = string
            var offset = s.length
            while (true) {
                val start = s.lastIndexOf('@', offset)
                if (start == -1) {
                    return s
                }
                var index = start + 1
                val length = string.length
                while (index < length) {
                    val c = string[index]
                    if (c != '.' && !Character.isJavaIdentifierPart(c)) {
                        break
                    }
                    index++
                }
                val annotation = string.substring(start + 1, index)

                val mapped = AnnotationItem.mapName(codebase, annotation, ApiPredicate(codebase))
                if (mapped != null) {
                    if (mapped != annotation) {
                        s = string.substring(0, start + 1) + mapped + s.substring(index)
                    }
                } else {
                    var balance = 0
                    // Find annotation end
                    while (index < length) {
                        val c = string[index]
                        if (c == '(') {
                            balance++
                        } else if (c == ')') {
                            balance--
                            if (balance == 0) {
                                index++
                                break
                            }
                        } else if (c != ' ' && balance == 0) {
                            break
                        }
                        index++
                    }
                    s = string.substring(0, start) + s.substring(index)
                }
                offset = start - 1
            }
        }

        private fun getCanonicalText(type: PsiType, annotated: Boolean): String {
            val typeString = type.getCanonicalText(annotated)
            if (!annotated) {
                return typeString
            }

            val index = typeString.indexOf(".@")
            if (index != -1) {
                // Work around type bugs in PSI: when you have a type like this:
                //    @android.support.annotation.NonNull java.lang.Float)
                // PSI returns
                //    @android.support.annotation.NonNull java.lang.@android.support.annotation.NonNull Float)
                //
                //
                // ...but sadly it's less predictable; e.g. it can be
                //    java.util.List<@android.support.annotation.Nullable java.lang.String>
                // PSI returns
                //    java.util.List<java.lang.@android.support.annotation.Nullable String>

                // Here we try to reverse this:
                val end = typeString.indexOf(' ', index)
                if (end != -1) {
                    val annotation = typeString.substring(index + 1, end)
                    if (typeString.lastIndexOf(annotation, index) == -1) {
                        // Find out where to place it
                        var ci = index
                        while (ci > 0) {
                            val c = typeString[ci]
                            if (c != '.' && !Character.isJavaIdentifierPart(c)) {
                                ci++
                                break
                            }
                            ci--
                        }
                        return typeString.substring(0, ci) +
                            annotation + " " +
                            typeString.substring(ci, index + 1) +
                            typeString.substring(end + 1)
                    } else {
                        return typeString.substring(0, index + 1) + typeString.substring(end + 1)
                    }
                }
            }

            return typeString
        }

        fun create(codebase: PsiBasedCodebase, psiType: PsiType): PsiTypeItem {
            return PsiTypeItem(codebase, psiType)
        }

        fun create(codebase: PsiBasedCodebase, original: PsiTypeItem): PsiTypeItem {
            return PsiTypeItem(codebase, original.psiType)
        }

        fun typeParameterList(typeList: PsiTypeParameterList?): String? {
            if (typeList != null && typeList.typeParameters.isNotEmpty()) {
                // TODO: Filter the type list classes? Try to construct a typelist of a private API!
                // We can't just use typeList.text here, because that just
                // uses the declaration from the source, which may not be
                // fully qualified - e.g. we might get
                //    <T extends View> instead of <T extends android.view.View>
                // Therefore, we'll need to compute it ourselves; I can't find
                // a utility for this
                val sb = StringBuilder()
                typeList.accept(object : PsiRecursiveElementVisitor() {
                    override fun visitElement(element: PsiElement) {
                        if (element is PsiTypeParameterList) {
                            val typeParameters = element.typeParameters
                            if (typeParameters.isEmpty()) {
                                return
                            }
                            sb.append("<")
                            var first = true
                            for (parameter in typeParameters) {
                                if (!first) {
                                    sb.append(", ")
                                }
                                first = false
                                visitElement(parameter)
                            }
                            sb.append(">")
                            return
                        } else if (element is PsiTypeParameter) {
                            sb.append(element.name)
                            // TODO: How do I get super -- e.g. "Comparable<? super T>"
                            val extendsList = element.extendsList
                            val refList = extendsList.referenceElements
                            if (refList.isNotEmpty()) {
                                sb.append(" extends ")
                                var first = true
                                for (refElement in refList) {
                                    if (!first) {
                                        sb.append(" & ")
                                    } else {
                                        first = false
                                    }

                                    if (refElement is PsiJavaCodeReferenceElement) {
                                        visitElement(refElement)
                                        continue
                                    }
                                    val resolved = refElement.resolve()
                                    if (resolved is PsiClass) {
                                        sb.append(resolved.qualifiedName ?: resolved.name)
                                        resolved.typeParameterList?.accept(this)
                                    } else {
                                        sb.append(refElement.referenceName)
                                    }
                                }
                            } else {
                                val extendsListTypes = element.extendsListTypes
                                if (extendsListTypes.isNotEmpty()) {
                                    sb.append(" extends ")
                                    var first = true
                                    for (type in extendsListTypes) {
                                        if (!first) {
                                            sb.append(" & ")
                                        } else {
                                            first = false
                                        }
                                        val resolved = type.resolve()
                                        if (resolved == null) {
                                            sb.append(type.className)
                                        } else {
                                            sb.append(resolved.qualifiedName ?: resolved.name)
                                            resolved.typeParameterList?.accept(this)
                                        }
                                    }
                                }
                            }
                            return
                        } else if (element is PsiJavaCodeReferenceElement) {
                            val resolved = element.resolve()
                            if (resolved is PsiClass) {
                                if (resolved.qualifiedName == null) {
                                    sb.append(resolved.name)
                                } else {
                                    sb.append(resolved.qualifiedName)
                                }
                                val typeParameters = element.parameterList
                                if (typeParameters != null) {
                                    val typeParameterElements = typeParameters.typeParameterElements
                                    if (typeParameterElements.isEmpty()) {
                                        return
                                    }

                                    // When reading in this from bytecode, the order is sometimes wrong
                                    // (for example, for
                                    //    public interface BaseStream<T, S extends BaseStream<T, S>>
                                    // the extends type BaseStream<T, S> will return the typeParameterElements
                                    // as [S,T] instead of [T,S]. However, the typeParameters.typeArguments
                                    // list is correct, so order the elements by the typeArguments array instead

                                    // Special case: just one type argument: no sorting issue
                                    if (typeParameterElements.size == 1) {
                                        sb.append("<")
                                        var first = true
                                        for (parameter in typeParameterElements) {
                                            if (!first) {
                                                sb.append(", ")
                                            }
                                            first = false
                                            visitElement(parameter)
                                        }
                                        sb.append(">")
                                        return
                                    }

                                    // More than one type argument

                                    val typeArguments = typeParameters.typeArguments
                                    if (typeArguments.isNotEmpty()) {
                                        sb.append("<")
                                        var first = true
                                        for (parameter in typeArguments) {
                                            if (!first) {
                                                sb.append(", ")
                                            }
                                            first = false
                                            // Try to match up a type parameter element
                                            var found = false
                                            for (typeElement in typeParameterElements) {
                                                if (parameter == typeElement.type) {
                                                    found = true
                                                    visitElement(typeElement)
                                                    break
                                                }
                                            }
                                            if (!found) {
                                                // No type element matched: use type instead
                                                val classType = PsiTypesUtil.getPsiClass(parameter)
                                                if (classType != null) {
                                                    visitElement(classType)
                                                } else {
                                                    sb.append(parameter.canonicalText)
                                                }
                                            }
                                        }
                                        sb.append(">")
                                    }
                                }
                                return
                            }
                        } else if (element is PsiTypeElement) {
                            val type = element.type
                            if (type is PsiWildcardType) {
                                sb.append("?")
                                if (type.isBounded) {
                                    if (type.isExtends) {
                                        sb.append(" extends ")
                                        sb.append(type.extendsBound.canonicalText)
                                    }
                                    if (type.isSuper) {
                                        sb.append(" super ")
                                        sb.append(type.superBound.canonicalText)
                                    }
                                }
                                return
                            }
                            sb.append(type.canonicalText)
                            return
                        } else if (element is PsiJavaToken && element.tokenType == JavaTokenType.COMMA) {
                            sb.append(",")
                            if (compatibility.spaceAfterCommaInTypes) {
                                if (element.nextSibling == null || element.nextSibling !is PsiWhiteSpace) {
                                    sb.append(" ")
                                }
                            }
                            return
                        }
                        if (element.firstChild == null) { // leaf nodes only
                            if (element is PsiCompiledElement) {
                                if (element is PsiReferenceList) {
                                    val referencedTypes = element.referencedTypes
                                    var first = true
                                    for (referenceType in referencedTypes) {
                                        if (first) {
                                            first = false
                                        } else {
                                            sb.append(", ")
                                        }
                                        sb.append(referenceType.canonicalText)
                                    }
                                }
                            } else {
                                sb.append(element.text)
                            }
                        }
                        super.visitElement(element)
                    }
                })

                val typeString = sb.toString()
                return TypeItem.cleanupGenerics(typeString)
            }

            return null
        }

        fun typeParameterClasses(codebase: PsiBasedCodebase, typeList: PsiTypeParameterList?): List<ClassItem> {
            if (typeList != null && typeList.typeParameters.isNotEmpty()) {
                val list = mutableListOf<ClassItem>()
                typeList.accept(object : PsiRecursiveElementVisitor() {
                    override fun visitElement(element: PsiElement) {
                        if (element is PsiTypeParameterList) {
                            val typeParameters = element.typeParameters
                            for (parameter in typeParameters) {
                                visitElement(parameter)
                            }
                            return
                        } else if (element is PsiTypeParameter) {
                            val extendsList = element.extendsList
                            val refList = extendsList.referenceElements
                            if (refList.isNotEmpty()) {
                                for (refElement in refList) {
                                    if (refElement is PsiJavaCodeReferenceElement) {
                                        visitElement(refElement)
                                        continue
                                    }
                                    val resolved = refElement.resolve()
                                    if (resolved is PsiClass) {
                                        addRealClass(
                                            list,
                                            codebase.findOrCreateClass(resolved)
                                        )
                                        resolved.typeParameterList?.accept(this)
                                    }
                                }
                            } else {
                                val extendsListTypes = element.extendsListTypes
                                if (extendsListTypes.isNotEmpty()) {
                                    for (type in extendsListTypes) {
                                        val resolved = type.resolve()
                                        if (resolved != null) {
                                            addRealClass(
                                                list, codebase.findOrCreateClass(resolved)
                                            )
                                            resolved.typeParameterList?.accept(this)
                                        }
                                    }
                                }
                            }
                            return
                        } else if (element is PsiJavaCodeReferenceElement) {
                            val resolved = element.resolve()
                            if (resolved is PsiClass) {
                                addRealClass(
                                    list,
                                    codebase.findOrCreateClass(resolved)
                                )
                                element.parameterList?.accept(this)
                                return
                            }
                        } else if (element is PsiTypeElement) {
                            val type = element.type
                            if (type is PsiWildcardType) {
                                if (type.isBounded) {
                                    addRealClass(
                                        codebase,
                                        list, type.bound
                                    )
                                }
                                if (type.isExtends) {
                                    addRealClass(
                                        codebase,
                                        list, type.extendsBound
                                    )
                                }
                                if (type.isSuper) {
                                    addRealClass(
                                        codebase,
                                        list, type.superBound
                                    )
                                }
                                return
                            }
                            return
                        }
                        super.visitElement(element)
                    }
                })

                return list
            } else {
                return emptyList()
            }
        }

        private fun addRealClass(codebase: PsiBasedCodebase, classes: MutableList<ClassItem>, type: PsiType?) {
            codebase.findClass(type ?: return)?.let {
                addRealClass(classes, it)
            }
        }

        private fun addRealClass(classes: MutableList<ClassItem>, cls: ClassItem) {
            if (!cls.isTypeParameter && !classes.contains(cls)) { // typically small number of items, don't need Set
                classes.add(cls)
            }
        }
    }
}