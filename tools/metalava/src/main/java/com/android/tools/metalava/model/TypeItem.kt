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

package com.android.tools.metalava.model

import com.android.tools.lint.detector.api.ClassContext
import com.android.tools.metalava.JAVA_LANG_OBJECT
import com.android.tools.metalava.JAVA_LANG_PREFIX
import com.android.tools.metalava.compatibility
import com.android.tools.metalava.options
import java.util.function.Predicate

/** Represents a type */
interface TypeItem {
    /**
     * Generates a string for this type.
     *
     * For a type like this: @Nullable java.util.List<@NonNull java.lang.String>,
     * [outerAnnotations] controls whether the top level annotation like @Nullable
     * is included, [innerAnnotations] controls whether annotations like @NonNull
     * are included, and [erased] controls whether we return the string for
     * the raw type, e.g. just "java.util.List"
     *
     * (The combination [outerAnnotations] = true and [innerAnnotations] = false
     * is not allowed.)
     */
    fun toTypeString(
        outerAnnotations: Boolean = false,
        innerAnnotations: Boolean = outerAnnotations,
        erased: Boolean = false
    ): String

    /** Alias for [toTypeString] with erased=true */
    fun toErasedTypeString(): String

    /** Returns the internal name of the type, as seen in bytecode */
    fun internalName(): String {
        // Default implementation; PSI subclass is more accurate
        return toSlashFormat(toErasedTypeString())
    }

    /** Array dimensions of this type; for example, for String it's 0 and for String[][] it's 2. */
    fun arrayDimensions(): Int

    fun asClass(): ClassItem?

    fun toSimpleType(): String {
        return stripJavaLangPrefix(toTypeString())
    }

    /**
     * Helper methods to compare types, especially types from signature files with types
     * from parsing, which may have slightly different formats, e.g. varargs ("...") versus
     * arrays ("[]"), java.lang. prefixes removed in wildcard signatures, etc.
     */
    fun toCanonicalType(): String {
        var s = toTypeString()
        while (s.contains(JAVA_LANG_PREFIX)) {
            s = s.replace(JAVA_LANG_PREFIX, "")
        }
        if (s.contains("...")) {
            s = s.replace("...", "[]")
        }

        return s
    }

    val primitive: Boolean

    fun typeArgumentClasses(): List<ClassItem>

    fun convertType(from: ClassItem, to: ClassItem): TypeItem {
        val map = from.mapTypeVariables(to)
        if (!map.isEmpty()) {
            return convertType(map)
        }

        return this
    }

    fun convertType(replacementMap: Map<String, String>?, owner: Item? = null): TypeItem

    fun convertTypeString(replacementMap: Map<String, String>?): String {
        return convertTypeString(toTypeString(outerAnnotations = true, innerAnnotations = true), replacementMap)
    }

    fun isJavaLangObject(): Boolean {
        return toTypeString() == JAVA_LANG_OBJECT
    }

    fun defaultValue(): Any? {
        return when (toTypeString()) {
            "boolean" -> false
            "byte" -> 0.toByte()
            "char" -> '\u0000'
            "short" -> 0.toShort()
            "int" -> 0
            "long" -> 0L
            "float" -> 0f
            "double" -> 0.0
            else -> null
        }
    }

    /** Returns true if this type references a type not matched by the given predicate */
    fun referencesExcludedType(filter: Predicate<Item>): Boolean {
        if (primitive) {
            return false
        }

        for (item in typeArgumentClasses()) {
            if (!filter.test(item)) {
                return true
            }
        }

        return false
    }

    fun defaultValueString(): String = defaultValue()?.toString() ?: "null"

    fun hasTypeArguments(): Boolean = toTypeString().contains("<")

    /**
     * If this type is a type parameter, then return the corresponding [TypeParameterItem].
     * The optional [context] provides the method or class where this type parameter
     * appears, and can be used for example to resolve the bounds for a type variable
     * used in a method that was specified on the class.
     */
    fun asTypeParameter(context: MemberItem? = null): TypeParameterItem?

    /**
     * Mark nullness annotations in the type as recent.
     * TODO: This isn't very clean; we should model individual annotations.
     */
    fun markRecent()

    companion object {
        /** Shortens types, if configured */
        fun shortenTypes(type: String): String {
            if (options.omitCommonPackages) {
                var cleaned = type
                if (cleaned.contains("@androidx.annotation.")) {
                    cleaned = cleaned.replace("@androidx.annotation.", "@")
                }
                if (cleaned.contains("@android.support.annotation.")) {
                    cleaned = cleaned.replace("@android.support.annotation.", "@")
                }

                return stripJavaLangPrefix(cleaned)
            }

            return type
        }

        /**
         * Removes java.lang. prefixes from types, unless it's in a subpackage such
         * as java.lang.reflect. For simplicity we may also leave inner classes
         * in the java.lang package untouched.
         *
         * NOTE: We only remove this from the front of the type; e.g. we'll replace
         * java.lang.Class<java.lang.String> with Class<java.lang.String>.
         * This is because the signature parsing of types is not 100% accurate
         * and we don't want to run into trouble with more complicated generic
         * type signatures where we end up not mapping the simplified types back
         * to the real fully qualified type names.
         */
        fun stripJavaLangPrefix(type: String): String {
            if (type.startsWith(JAVA_LANG_PREFIX)) {
                // Replacing java.lang is harder, since we don't want to operate in sub packages,
                // e.g. java.lang.String -> String, but java.lang.reflect.Method -> unchanged
                val start = JAVA_LANG_PREFIX.length
                val end = type.length
                for (index in start until end) {
                    if (type[index] == '<') {
                        return type.substring(start)
                    } else if (type[index] == '.') {
                        return type
                    }
                }

                return type.substring(start)
            }

            return type
        }

        fun formatType(type: String?): String {
            if (type == null) {
                return ""
            }

            var cleaned = type

            if (compatibility.spacesAfterCommas && cleaned.indexOf(',') != -1) {
                // The compat files have spaces after commas where we normally don't
                cleaned = cleaned.replace(",", ", ").replace(",  ", ", ")
            }

            cleaned = cleanupGenerics(cleaned)
            return cleaned
        }

        fun cleanupGenerics(signature: String): String {
            // <T extends java.lang.Object> is the same as <T>
            //  but NOT for <T extends Object & java.lang.Comparable> -- you can't
            //  shorten this to <T & java.lang.Comparable
            // return type.replace(" extends java.lang.Object", "")
            return signature.replace(" extends java.lang.Object>", ">")
        }

        val comparator: Comparator<TypeItem> = Comparator { type1, type2 ->
            val cls1 = type1.asClass()
            val cls2 = type2.asClass()
            if (cls1 != null && cls2 != null) {
                ClassItem.fullNameComparator.compare(cls1, cls2)
            } else {
                type1.toTypeString().compareTo(type2.toTypeString())
            }
        }

        fun convertTypeString(typeString: String, replacementMap: Map<String, String>?): String {
            var string = typeString
            if (replacementMap != null && replacementMap.isNotEmpty()) {
                // This is a moved method (typically an implementation of an interface
                // method provided in a hidden superclass), with generics signatures.
                // We need to rewrite the generics variables in case they differ
                // between the classes.
                if (!replacementMap.isEmpty()) {
                    replacementMap.forEach { from, to ->
                        // We can't just replace one string at a time:
                        // what if I have a map of {"A"->"B", "B"->"C"} and I tried to convert A,B,C?
                        // If I do the replacements one letter at a time I end up with C,C,C; if I do the substitutions
                        // simultaneously I get B,C,C. Therefore, we insert "___" as a magical prefix to prevent
                        // scenarios like this, and then we'll drop them afterwards.
                        string = string.replace(Regex(pattern = """\b$from\b"""), replacement = "___$to")
                    }
                }
                string = string.replace("___", "")
                return string
            } else {
                return string
            }
        }

        // Copied from doclava1
        fun toSlashFormat(typeName: String): String {
            var name = typeName
            var dimension = ""
            while (name.endsWith("[]")) {
                dimension += "["
                name = name.substring(0, name.length - 2)
            }

            val base: String
            base = when (name) {
                "void" -> "V"
                "byte" -> "B"
                "boolean" -> "Z"
                "char" -> "C"
                "short" -> "S"
                "int" -> "I"
                "long" -> "L"
                "float" -> "F"
                "double" -> "D"
                else -> "L" + ClassContext.getInternalName(name) + ";"
            }

            return dimension + base
        }
    }
}