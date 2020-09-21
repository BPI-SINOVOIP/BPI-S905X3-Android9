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

import com.android.SdkConstants
import com.android.SdkConstants.ATTR_VALUE
import com.android.SdkConstants.INT_DEF_ANNOTATION
import com.android.SdkConstants.LONG_DEF_ANNOTATION
import com.android.SdkConstants.STRING_DEF_ANNOTATION
import com.android.tools.lint.annotations.Extractor.ANDROID_INT_DEF
import com.android.tools.lint.annotations.Extractor.ANDROID_LONG_DEF
import com.android.tools.lint.annotations.Extractor.ANDROID_STRING_DEF
import com.android.tools.metalava.ANDROIDX_ANNOTATION_PREFIX
import com.android.tools.metalava.ANDROID_SUPPORT_ANNOTATION_PREFIX
import com.android.tools.metalava.JAVA_LANG_PREFIX
import com.android.tools.metalava.Options
import com.android.tools.metalava.RECENTLY_NONNULL
import com.android.tools.metalava.RECENTLY_NULLABLE
import com.android.tools.metalava.options
import java.util.function.Predicate

fun isNullableAnnotation(qualifiedName: String): Boolean {
    return qualifiedName.endsWith("Nullable")
}

fun isNonNullAnnotation(qualifiedName: String): Boolean {
    return qualifiedName.endsWith("NonNull") ||
        qualifiedName.endsWith("NotNull") ||
        qualifiedName.endsWith("Nonnull")
}

interface AnnotationItem {
    val codebase: Codebase

    /** Fully qualified name of the annotation */
    fun qualifiedName(): String?

    /** Generates source code for this annotation (using fully qualified names) */
    fun toSource(): String

    /** Whether this annotation is significant and should be included in signature files, stubs, etc */
    fun isSignificant(): Boolean {
        return includeInSignatures(qualifiedName() ?: return false)
    }

    /** Attributes of the annotation (may be empty) */
    fun attributes(): List<AnnotationAttribute>

    /** True if this annotation represents @Nullable or @NonNull (or some synonymous annotation) */
    fun isNullnessAnnotation(): Boolean {
        return isNullable() || isNonNull()
    }

    /** True if this annotation represents @Nullable (or some synonymous annotation) */
    fun isNullable(): Boolean {
        return isNullableAnnotation(qualifiedName() ?: return false)
    }

    /** True if this annotation represents @NonNull (or some synonymous annotation) */
    fun isNonNull(): Boolean {
        return isNonNullAnnotation(qualifiedName() ?: return false)
    }

    /** True if this annotation represents @IntDef, @LongDef or @StringDef */
    fun isTypeDefAnnotation(): Boolean {
        val name = qualifiedName() ?: return false
        return (INT_DEF_ANNOTATION.isEquals(name) ||
            STRING_DEF_ANNOTATION.isEquals(name) ||
            LONG_DEF_ANNOTATION.isEquals(name) ||
            ANDROID_INT_DEF == name ||
            ANDROID_STRING_DEF == name ||
            ANDROID_LONG_DEF == name)
    }

    /**
     * True if this annotation represents a @ParameterName annotation (or some synonymous annotation).
     * The parameter name should be the default attribute or "value".
     */
    fun isParameterName(): Boolean {
        return qualifiedName()?.endsWith(".ParameterName") ?: return false
    }

    /**
     * True if this annotation represents a @DefaultValue annotation (or some synonymous annotation).
     * The default value should be the default attribute or "value".
     */
    fun isDefaultValue(): Boolean {
        return qualifiedName()?.endsWith(".DefaultValue") ?: return false
    }

    /** Returns the given named attribute if specified */
    fun findAttribute(name: String?): AnnotationAttribute? {
        val actualName = name ?: ATTR_VALUE
        return attributes().firstOrNull { it.name == actualName }
    }

    /** Find the class declaration for the given annotation */
    fun resolve(): ClassItem? {
        return codebase.findClass(qualifiedName() ?: return null)
    }

    companion object {
        /** Whether the given annotation name is "significant", e.g. should be included in signature files */
        fun includeInSignatures(qualifiedName: String?): Boolean {
            qualifiedName ?: return false
            if (qualifiedName.startsWith(ANDROID_SUPPORT_ANNOTATION_PREFIX) ||
                qualifiedName.startsWith(ANDROIDX_ANNOTATION_PREFIX)
            ) {

                // Don't include typedefs in the stub files.
                if (qualifiedName.endsWith("IntDef") || qualifiedName.endsWith("StringDef")) {
                    return false
                }

                return true
            }
            return false
        }

        /** The simple name of an annotation, which is the annotation name (not qualified name) prefixed by @ */
        fun simpleName(item: AnnotationItem): String {
            val qualifiedName = item.qualifiedName() ?: return ""
            return "@${qualifiedName.substring(qualifiedName.lastIndexOf('.') + 1)}"
        }

        /**
         * Maps an annotation name to the name to be used in signatures/stubs/external annotation files.
         * Annotations that should not be exported are mapped to null.
         */
        fun mapName(codebase: Codebase, qualifiedName: String?, filter: Predicate<Item>? = null): String? {
            qualifiedName ?: return null

            when (qualifiedName) {
            // Resource annotations
                "android.support.annotation.AnimRes",
                "android.annotation.AnimRes" -> return "androidx.annotation.AnimRes"
                "android.support.annotation.AnimatorRes",
                "android.annotation.AnimatorRes" -> return "androidx.annotation.AnimatorRes"
                "android.support.annotation.AnyRes",
                "android.annotation.AnyRes" -> return "androidx.annotation.AnyRes"
                "android.support.annotation.ArrayRes",
                "android.annotation.ArrayRes" -> return "androidx.annotation.ArrayRes"
                "android.support.annotation.AttrRes",
                "android.annotation.AttrRes" -> return "androidx.annotation.AttrRes"
                "android.support.annotation.BoolRes",
                "android.annotation.BoolRes" -> return "androidx.annotation.BoolRes"
                "android.support.annotation.ColorRes",
                "android.annotation.ColorRes" -> return "androidx.annotation.ColorRes"
                "android.support.annotation.DimenRes",
                "android.annotation.DimenRes" -> return "androidx.annotation.DimenRes"
                "android.support.annotation.DrawableRes",
                "android.annotation.DrawableRes" -> return "androidx.annotation.DrawableRes"
                "android.support.annotation.FontRes",
                "android.annotation.FontRes" -> return "androidx.annotation.FontRes"
                "android.support.annotation.FractionRes",
                "android.annotation.FractionRes" -> return "androidx.annotation.FractionRes"
                "android.support.annotation.IdRes",
                "android.annotation.IdRes" -> return "androidx.annotation.IdRes"
                "android.support.annotation.IntegerRes",
                "android.annotation.IntegerRes" -> return "androidx.annotation.IntegerRes"
                "android.support.annotation.InterpolatorRes",
                "android.annotation.InterpolatorRes" -> return "androidx.annotation.InterpolatorRes"
                "android.support.annotation.LayoutRes",
                "android.annotation.LayoutRes" -> return "androidx.annotation.LayoutRes"
                "android.support.annotation.MenuRes",
                "android.annotation.MenuRes" -> return "androidx.annotation.MenuRes"
                "android.support.annotation.PluralsRes",
                "android.annotation.PluralsRes" -> return "androidx.annotation.PluralsRes"
                "android.support.annotation.RawRes",
                "android.annotation.RawRes" -> return "androidx.annotation.RawRes"
                "android.support.annotation.StringRes",
                "android.annotation.StringRes" -> return "androidx.annotation.StringRes"
                "android.support.annotation.StyleRes",
                "android.annotation.StyleRes" -> return "androidx.annotation.StyleRes"
                "android.support.annotation.StyleableRes",
                "android.annotation.StyleableRes" -> return "androidx.annotation.StyleableRes"
                "android.support.annotation.TransitionRes",
                "android.annotation.TransitionRes" -> return "androidx.annotation.TransitionRes"
                "android.support.annotation.XmlRes",
                "android.annotation.XmlRes" -> return "androidx.annotation.XmlRes"

            // Threading
                "android.support.annotation.AnyThread",
                "android.annotation.AnyThread" -> return "androidx.annotation.AnyThread"
                "android.support.annotation.BinderThread",
                "android.annotation.BinderThread" -> return "androidx.annotation.BinderThread"
                "android.support.annotation.MainThread",
                "android.annotation.MainThread" -> return "androidx.annotation.MainThread"
                "android.support.annotation.UiThread",
                "android.annotation.UiThread" -> return "androidx.annotation.UiThread"
                "android.support.annotation.WorkerThread",
                "android.annotation.WorkerThread" -> return "androidx.annotation.WorkerThread"

            // Colors
                "android.support.annotation.ColorInt",
                "android.annotation.ColorInt" -> return "androidx.annotation.ColorInt"
                "android.support.annotation.ColorLong",
                "android.annotation.ColorLong" -> return "androidx.annotation.ColorLong"
                "android.support.annotation.HalfFloat",
                "android.annotation.HalfFloat" -> return "androidx.annotation.HalfFloat"

            // Ranges and sizes
                "android.support.annotation.FloatRange",
                "android.annotation.FloatRange" -> return "androidx.annotation.FloatRange"
                "android.support.annotation.IntRange",
                "android.annotation.IntRange" -> return "androidx.annotation.IntRange"
                "android.support.annotation.Size",
                "android.annotation.Size" -> return "androidx.annotation.Size"
                "android.support.annotation.Px",
                "android.annotation.Px" -> return "androidx.annotation.Px"
                "android.support.annotation.Dimension",
                "android.annotation.Dimension" -> return "androidx.annotation.Dimension"

            // Null
                "android.support.annotation.NonNull",
                "android.annotation.NonNull" -> return "androidx.annotation.NonNull"
                "android.support.annotation.Nullable",
                "android.annotation.Nullable" -> return "androidx.annotation.Nullable"
                "libcore.util.NonNull" -> return "androidx.annotation.NonNull"
                "libcore.util.Nullable" -> return "androidx.annotation.Nullable"
                "org.jetbrains.annotations.NotNull" -> return "androidx.annotation.NonNull"
                "org.jetbrains.annotations.Nullable" -> return "androidx.annotation.Nullable"

            // Typedefs
                "android.support.annotation.IntDef",
                "android.annotation.IntDef" -> return "androidx.annotation.IntDef"
                "android.support.annotation.StringDef",
                "android.annotation.StringDef" -> return "androidx.annotation.StringDef"
                "android.support.annotation.LongDef",
                "android.annotation.LongDef" -> return "androidx.annotation.LongDef"

            // Misc
                "android.support.annotation.CallSuper",
                "android.annotation.CallSuper" -> return "androidx.annotation.CallSuper"
                "android.support.annotation.CheckResult",
                "android.annotation.CheckResult" -> return "androidx.annotation.CheckResult"
                "android.support.annotation.RequiresPermission",
                "android.annotation.RequiresPermission" -> return "androidx.annotation.RequiresPermission"

            // These aren't support annotations, but could/should be:
                "android.annotation.CurrentTimeMillisLong",
                "android.annotation.DurationMillisLong",
                "android.annotation.ElapsedRealtimeLong",
                "android.annotation.UserIdInt",
                "android.annotation.BytesLong",

                    // These aren't support annotations
                "android.annotation.AppIdInt",
                "android.annotation.SuppressAutoDoc",
                "android.annotation.SystemApi",
                "android.annotation.TestApi",
                "android.annotation.CallbackExecutor",
                "android.annotation.Condemned",

                "android.annotation.Widget" -> {
                    // Remove, unless (a) public or (b) specifically included in --showAnnotations
                    return if (options.showAnnotations.contains(qualifiedName)) {
                        qualifiedName
                    } else if (filter != null) {
                        val cls = codebase.findClass(qualifiedName)
                        if (cls != null && filter.test(cls)) {
                            qualifiedName
                        } else {
                            null
                        }
                    } else {
                        qualifiedName
                    }
                }

            // Included for analysis, but should not be exported:
                "android.annotation.BroadcastBehavior",
                "android.annotation.SdkConstant",
                "android.annotation.RequiresFeature",
                "android.annotation.SystemService" -> return qualifiedName

            // Should not be mapped to a different package name:
                "android.annotation.TargetApi",
                "android.annotation.SuppressLint" -> return qualifiedName

            // We only change recently/newly nullable annotation if the codebase supports it
                RECENTLY_NULLABLE -> return if (codebase.supportsStagedNullability) qualifiedName else "androidx.annotation.Nullable"
                RECENTLY_NONNULL -> return if (codebase.supportsStagedNullability) qualifiedName else "androidx.annotation.NonNull"

                else -> {
                    // Some new annotations added to the platform: assume they are support annotations?
                    return when {
                    // Special Kotlin annotations recognized by the compiler: map to supported package name
                        qualifiedName.endsWith(".ParameterName") || qualifiedName.endsWith(".DefaultValue") ->
                            "kotlin.annotations.jvm.internal${qualifiedName.substring(qualifiedName.lastIndexOf('.'))}"

                    // Other third party nullness annotations?
                        isNullableAnnotation(qualifiedName) -> "androidx.annotation.Nullable"
                        isNonNullAnnotation(qualifiedName) -> "androidx.annotation.NonNull"

                    // Support library annotations are all included, as is the built-in stuff like @Retention
                        qualifiedName.startsWith(ANDROIDX_ANNOTATION_PREFIX) -> return qualifiedName
                        qualifiedName.startsWith(JAVA_LANG_PREFIX) -> return qualifiedName

                    // Unknown Android platform annotations
                        qualifiedName.startsWith("android.annotation.") -> {
                            // Remove, unless specifically included in --showAnnotations
                            return if (options.showAnnotations.contains(qualifiedName)) {
                                qualifiedName
                            } else {
                                null
                            }
                        }

                        qualifiedName.startsWith(ANDROID_SUPPORT_ANNOTATION_PREFIX) -> {
                            return mapName(
                                codebase,
                                ANDROIDX_ANNOTATION_PREFIX + qualifiedName.substring(ANDROID_SUPPORT_ANNOTATION_PREFIX.length),
                                filter
                            )
                        }

                        else -> {
                            // Remove, unless (a) public or (b) specifically included in --showAnnotations
                            return if (options.showAnnotations.contains(qualifiedName)) {
                                qualifiedName
                            } else if (filter != null) {
                                val cls = codebase.findClass(qualifiedName)
                                if (cls != null && filter.test(cls)) {
                                    qualifiedName
                                } else {
                                    null
                                }
                            } else {
                                qualifiedName
                            }
                        }
                    }
                }
            }
        }

        /**
         * Given a "full" annotation name, shortens it by removing redundant package names.
         * This is intended to be used by the [Options.omitCommonPackages] flag
         * to reduce clutter in signature files.
         *
         * For example, this method will convert `@androidx.annotation.Nullable` to just
         * `@Nullable`, and `@androidx.annotation.IntRange(from=20)` to `IntRange(from=20)`.
         */
        fun shortenAnnotation(source: String): String {
            return when {
                source.startsWith("android.annotation.", 1) -> {
                    "@" + source.substring("@android.annotation.".length)
                }
                source.startsWith(ANDROID_SUPPORT_ANNOTATION_PREFIX, 1) -> {
                    "@" + source.substring("@android.support.annotation.".length)
                }
                source.startsWith(ANDROIDX_ANNOTATION_PREFIX, 1) -> {
                    "@" + source.substring("@androidx.annotation.".length)
                }
                else -> source
            }
        }

        /**
         * Reverses the [shortenAnnotation] method. Intended for use when reading in signature files
         * that contain shortened type references.
         */
        fun unshortenAnnotation(source: String): String {
            return when {
            // These 3 annotations are in the android.annotation. package, not android.support.annotation
                source.startsWith("@SystemService") ||
                    source.startsWith("@TargetApi") ||
                    source.startsWith("@SuppressLint") ->
                    "@android.annotation." + source.substring(1)
                else -> {
                    "@androidx.annotation." + source.substring(1)
                }
            }
        }
    }
}

/** An attribute of an annotation, such as "value" */
interface AnnotationAttribute {
    /** The name of the annotation */
    val name: String
    /** The annotation value */
    val value: AnnotationAttributeValue

    /**
     * Return all leaf values; this flattens the complication of handling
     * {@code @SuppressLint("warning")} and {@code @SuppressLint({"warning1","warning2"})
     */
    fun leafValues(): List<AnnotationAttributeValue> {
        val result = mutableListOf<AnnotationAttributeValue>()
        AnnotationAttributeValue.addValues(value, result)
        return result
    }
}

/** An annotation value */
interface AnnotationAttributeValue {
    /** Generates source code for this annotation value */
    fun toSource(): String

    /** The value of the annotation */
    fun value(): Any?

    /** If the annotation declaration references a field (or class etc), return the resolved class */
    fun resolve(): Item?

    companion object {
        fun addValues(value: AnnotationAttributeValue, into: MutableList<AnnotationAttributeValue>) {
            if (value is AnnotationArrayAttributeValue) {
                for (v in value.values) {
                    addValues(v, into)
                }
            } else if (value is AnnotationSingleAttributeValue) {
                into.add(value)
            }
        }
    }
}

/** An annotation value (for a single item, not an array) */
interface AnnotationSingleAttributeValue : AnnotationAttributeValue {
    /** The annotation value, expressed as source code */
    val valueSource: String
    /** The annotation value */
    val value: Any?

    override fun value() = value
}

/** An annotation value for an array of items */
interface AnnotationArrayAttributeValue : AnnotationAttributeValue {
    /** The annotation values */
    val values: List<AnnotationAttributeValue>

    override fun resolve(): Item? {
        error("resolve() should not be called on an array value")
    }

    override fun value() = values.mapNotNull { it.value() }.toTypedArray()
}

class DefaultAnnotationAttribute(
    override val name: String,
    override val value: DefaultAnnotationValue
) : AnnotationAttribute {
    companion object {
        fun create(name: String, value: String): DefaultAnnotationAttribute {
            return DefaultAnnotationAttribute(name, DefaultAnnotationValue.create(value))
        }

        fun createList(source: String): List<AnnotationAttribute> {
            val list = mutableListOf<AnnotationAttribute>()
            if (source.contains("{")) {
                assert(
                    source.indexOf('{', source.indexOf('{', source.indexOf('{') + 1) + 1) != -1
                ) { "Multiple arrays not supported: $source" }
                val split = source.indexOf('=')
                val name: String
                val value: String
                if (split == -1) {
                    name = "value"
                    value = source.substring(source.indexOf('{'))
                } else {
                    name = source.substring(0, split).trim()
                    value = source.substring(split + 1).trim()
                }
                list.add(DefaultAnnotationAttribute.create(name, value))
                return list
            }

            source.split(",").forEach { declaration ->
                val split = declaration.indexOf('=')
                val name: String
                val value: String
                if (split == -1) {
                    name = "value"
                    value = declaration.trim()
                } else {
                    name = declaration.substring(0, split).trim()
                    value = declaration.substring(split + 1).trim()
                }
                list.add(DefaultAnnotationAttribute.create(name, value))
            }
            return list
        }
    }
}

abstract class DefaultAnnotationValue : AnnotationAttributeValue {
    companion object {
        fun create(value: String): DefaultAnnotationValue {
            return if (value.startsWith("{")) { // Array
                DefaultAnnotationArrayAttributeValue(value)
            } else {
                DefaultAnnotationSingleAttributeValue(value)
            }
        }
    }

    override fun toString(): String = toSource()
}

class DefaultAnnotationSingleAttributeValue(override val valueSource: String) : DefaultAnnotationValue(),
    AnnotationSingleAttributeValue {
    @Suppress("IMPLICIT_CAST_TO_ANY")
    override val value = when {
        valueSource == SdkConstants.VALUE_TRUE -> true
        valueSource == SdkConstants.VALUE_FALSE -> false
        valueSource.startsWith("\"") -> valueSource.removeSurrounding("\"")
        valueSource.startsWith('\'') -> valueSource.removeSurrounding("'")[0]
        else -> try {
            if (valueSource.contains(".")) {
                valueSource.toDouble()
            } else {
                valueSource.toLong()
            }
        } catch (e: NumberFormatException) {
            valueSource
        }
    }

    override fun resolve(): Item? = null

    override fun toSource() = valueSource
}

class DefaultAnnotationArrayAttributeValue(val value: String) : DefaultAnnotationValue(),
    AnnotationArrayAttributeValue {
    init {
        assert(value.startsWith("{") && value.endsWith("}")) { value }
    }

    override val values = value.substring(1, value.length - 1).split(",").map {
        DefaultAnnotationValue.create(it.trim())
    }.toList()

    override fun toSource() = value
}
