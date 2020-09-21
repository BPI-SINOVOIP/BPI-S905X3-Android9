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

package com.android.tools.metalava

import com.android.SdkConstants
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.model.AnnotationAttributeValue
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.TypeItem
import com.android.tools.metalava.model.visitors.ApiVisitor
import java.util.regex.Pattern

/** Misc API suggestions */
class AndroidApiChecks {
    fun check(codebase: Codebase) {
        codebase.accept(object : ApiVisitor(
            codebase,
            // Sort by source order such that warnings follow source line number order
            fieldComparator = FieldItem.comparator,
            methodComparator = MethodItem.sourceOrderComparator
        ) {
            override fun skip(item: Item): Boolean {
                // Limit the checks to the android.* namespace (except for ICU)
                if (item is ClassItem) {
                    val name = item.qualifiedName()
                    return !(name.startsWith("android.") && !name.startsWith("android.icu."))
                }
                return super.skip(item)
            }

            override fun visitItem(item: Item) {
                checkTodos(item)
            }

            override fun visitMethod(method: MethodItem) {
                checkRequiresPermission(method)
                if (!method.isConstructor()) {
                    checkVariable(method, "@return", "Return value of '" + method.name() + "'", method.returnType())
                }
            }

            override fun visitField(field: FieldItem) {
                if (field.name().contains("ACTION")) {
                    checkIntentAction(field)
                }
                checkVariable(field, null, "Field '" + field.name() + "'", field.type())
            }

            override fun visitParameter(parameter: ParameterItem) {
                checkVariable(
                    parameter,
                    parameter.name(),
                    "Parameter '" + parameter.name() + "' of '" + parameter.containingMethod().name() + "'",
                    parameter.type()
                )
            }
        })
    }

    private var cachedDocumentation: String = ""
    private var cachedDocumentationItem: Item? = null
    private var cachedDocumentationTag: String? = null

    // Cache around findDocumentation
    private fun getDocumentation(item: Item, tag: String?): String {
        return if (item === cachedDocumentationItem && cachedDocumentationTag == tag) {
            cachedDocumentation
        } else {
            cachedDocumentationItem = item
            cachedDocumentationTag = tag
            cachedDocumentation = findDocumentation(item, tag)
            cachedDocumentation
        }
    }

    private fun findDocumentation(item: Item, tag: String?): String {
        if (item is ParameterItem) {
            return findDocumentation(item.containingMethod(), item.name())
        }

        val doc = item.documentation
        if (doc.isBlank()) {
            return ""
        }

        if (tag == null) {
            return doc
        }

        var begin: Int
        if (tag == "@return") {
            // return tag
            begin = doc.indexOf("@return")
        } else {
            begin = 0
            while (true) {
                begin = doc.indexOf(tag, begin)
                if (begin == -1) {
                    return ""
                } else {
                    // See if it's prefixed by @param
                    // Scan backwards and allow whitespace and *
                    var ok = false
                    for (i in begin - 1 downTo 0) {
                        val c = doc[i]
                        if (c != '*' && !Character.isWhitespace(c)) {
                            if (c == 'm' && doc.startsWith("@param", i - 5, true)) {
                                begin = i - 5
                                ok = true
                            }
                            break
                        }
                    }
                    if (ok) {
                        // found beginning
                        break
                    }
                }
                begin += tag.length
            }
        }

        if (begin == -1) {
            return ""
        }

        // Find end
        // This is the first block tag on a new line
        var isLinePrefix = false
        var end = doc.length
        for (i in begin + 1 until doc.length) {
            val c = doc[i]

            if (c == '@' && (isLinePrefix ||
                    doc.startsWith("@param", i, true) ||
                    doc.startsWith("@return", i, true))
            ) {
                // Found it
                end = i
                break
            } else if (c == '\n') {
                isLinePrefix = true
            } else if (c != '*' && !Character.isWhitespace(c)) {
                isLinePrefix = false
            }
        }

        return doc.substring(begin, end)
    }

    private fun checkTodos(item: Item) {
        if (item.documentation.contains("TODO:") || item.documentation.contains("TODO(")) {
            reporter.report(Errors.TODO, item, "Documentation mentions 'TODO'")
        }
    }

    private fun checkRequiresPermission(method: MethodItem) {
        val text = method.documentation

        val annotation = method.modifiers.findAnnotation("android.support.annotation.RequiresPermission")
        if (annotation != null) {
            for (attribute in annotation.attributes()) {
                var values: List<AnnotationAttributeValue>? = null
                when (attribute.name) {
                    "value", "allOf", "anyOf" -> {
                        values = attribute.leafValues()
                    }
                }
                if (values == null || values.isEmpty()) {
                    continue
                }

                for (value in values) {
                    // var perm = String.valueOf(value.value())
                    var perm = value.toSource()
                    if (perm.indexOf('.') >= 0) perm = perm.substring(perm.lastIndexOf('.') + 1)
                    if (text.contains(perm)) {
                        reporter.report(
                            // Why is that a problem? Sometimes you want to describe
                            // particular use cases.
                            Errors.REQUIRES_PERMISSION, method, "Method '" + method.name() +
                                "' documentation mentions permissions already declared by @RequiresPermission"
                        )
                    }
                }
            }
        } else if (text.contains("android.Manifest.permission") || text.contains("android.permission.")) {
            reporter.report(
                Errors.REQUIRES_PERMISSION, method, "Method '" + method.name() +
                    "' documentation mentions permissions without declaring @RequiresPermission"
            )
        }
    }

    private fun checkIntentAction(field: FieldItem) {
        // Intent rules don't apply to support library
        if (field.containingClass().qualifiedName().startsWith("android.support.")) {
            return
        }

        val hasBehavior = field.modifiers.findAnnotation("android.annotation.BroadcastBehavior") != null
        val hasSdkConstant = field.modifiers.findAnnotation("android.annotation.SdkConstant") != null

        val text = field.documentation

        if (text.contains("Broadcast Action:") ||
            text.contains("protected intent") && text.contains("system")
        ) {
            if (!hasBehavior) {
                reporter.report(
                    Errors.BROADCAST_BEHAVIOR, field,
                    "Field '" + field.name() + "' is missing @BroadcastBehavior"
                )
            }
            if (!hasSdkConstant) {
                reporter.report(
                    Errors.SDK_CONSTANT, field, "Field '" + field.name() +
                        "' is missing @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)"
                )
            }
        }

        if (text.contains("Activity Action:")) {
            if (!hasSdkConstant) {
                reporter.report(
                    Errors.SDK_CONSTANT, field, "Field '" + field.name() +
                        "' is missing @SdkConstant(SdkConstantType.ACTIVITY_INTENT_ACTION)"
                )
            }
        }
    }

    private fun checkVariable(
        item: Item,
        tag: String?,
        ident: String,
        type: TypeItem?
    ) {
        type ?: return
        if (type.toString() == "int" && constantPattern.matcher(getDocumentation(item, tag)).find()) {
            var foundTypeDef = false
            for (annotation in item.modifiers.annotations()) {
                val cls = annotation.resolve() ?: continue
                val modifiers = cls.modifiers
                if (modifiers.findAnnotation(SdkConstants.INT_DEF_ANNOTATION.oldName()) != null ||
                    modifiers.findAnnotation(SdkConstants.INT_DEF_ANNOTATION.newName()) != null
                ) {
                    // TODO: Check that all the constants listed in the documentation are included in the
                    // annotation?
                    foundTypeDef = true
                    break
                }
            }

            if (!foundTypeDef) {
                reporter.report(
                    Errors.INT_DEF, item,
                    // TODO: Include source code you can paste right into the code?
                    "$ident documentation mentions constants without declaring an @IntDef"
                )
            }
        }

        if (nullPattern.matcher(getDocumentation(item, tag)).find() &&
            !item.hasNullnessInfo()
        ) {
            reporter.report(
                Errors.NULLABLE, item,
                "$ident documentation mentions 'null' without declaring @NonNull or @Nullable"
            )
        }
    }

    companion object {
        val constantPattern: Pattern = Pattern.compile("[A-Z]{3,}_([A-Z]{3,}|\\*)")
        val nullPattern: Pattern = Pattern.compile("\\bnull\\b")
    }
}
