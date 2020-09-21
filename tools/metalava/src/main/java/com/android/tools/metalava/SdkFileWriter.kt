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

import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.FieldItem
import java.io.BufferedWriter
import java.io.File
import java.io.FileWriter
import java.io.IOException

// Ported from doclava1

private const val ANDROID_VIEW_VIEW = "android.view.View"
private const val ANDROID_VIEW_VIEW_GROUP = "android.view.ViewGroup"
private const val ANDROID_VIEW_VIEW_GROUP_LAYOUT_PARAMS = "android.view.ViewGroup.LayoutParams"
private const val SDK_CONSTANT_ANNOTATION = "android.annotation.SdkConstant"
private const val SDK_CONSTANT_TYPE_ACTIVITY_ACTION =
    "android.annotation.SdkConstant.SdkConstantType.ACTIVITY_INTENT_ACTION"
private const val SDK_CONSTANT_TYPE_BROADCAST_ACTION =
    "android.annotation.SdkConstant.SdkConstantType.BROADCAST_INTENT_ACTION"
private const val SDK_CONSTANT_TYPE_SERVICE_ACTION =
    "android.annotation.SdkConstant.SdkConstantType.SERVICE_ACTION"
private const val SDK_CONSTANT_TYPE_CATEGORY = "android.annotation.SdkConstant.SdkConstantType.INTENT_CATEGORY"
private const val SDK_CONSTANT_TYPE_FEATURE = "android.annotation.SdkConstant.SdkConstantType.FEATURE"
private const val SDK_WIDGET_ANNOTATION = "android.annotation.Widget"
private const val SDK_LAYOUT_ANNOTATION = "android.annotation.Layout"

private const val TYPE_NONE = 0
private const val TYPE_WIDGET = 1
private const val TYPE_LAYOUT = 2
private const val TYPE_LAYOUT_PARAM = 3

/**
 * Writes various SDK metadata files packaged with the SDK, such as
 * {@code platforms/android-27/data/features.txt}
 */
class SdkFileWriter(val codebase: Codebase, private val outputDir: java.io.File) {
    /**
     * Collect the values used by the Dev tools and write them in files packaged with the SDK
     */
    fun generate() {
        val activityActions = mutableListOf<String>()
        val broadcastActions = mutableListOf<String>()
        val serviceActions = mutableListOf<String>()
        val categories = mutableListOf<String>()
        val features = mutableListOf<String>()
        val layouts = mutableListOf<ClassItem>()
        val widgets = mutableListOf<ClassItem>()
        val layoutParams = mutableListOf<ClassItem>()

        val classes = codebase.getPackages().allClasses()

        // The topmost LayoutParams class - android.view.ViewGroup.LayoutParams
        var topLayoutParams: ClassItem? = null

        // Go through all the fields of all the classes, looking SDK stuff.
        for (clazz in classes) {
            // first check constant fields for the SdkConstant annotation.
            val fields = clazz.fields()
            for (field in fields) {
                val value = field.initialValue() ?: continue
                val annotations = field.modifiers.annotations()
                for (annotation in annotations) {
                    if (SDK_CONSTANT_ANNOTATION == annotation.qualifiedName()) {
                        val resolved =
                            annotation.findAttribute(null)?.leafValues()?.firstOrNull()?.resolve() as? FieldItem
                                ?: continue
                        val type = resolved.containingClass().qualifiedName() + "." + resolved.name()
                        when {
                            SDK_CONSTANT_TYPE_ACTIVITY_ACTION == type -> activityActions.add(value.toString())
                            SDK_CONSTANT_TYPE_BROADCAST_ACTION == type -> broadcastActions.add(value.toString())
                            SDK_CONSTANT_TYPE_SERVICE_ACTION == type -> serviceActions.add(value.toString())
                            SDK_CONSTANT_TYPE_CATEGORY == type -> categories.add(value.toString())
                            SDK_CONSTANT_TYPE_FEATURE == type -> features.add(value.toString())
                        }
                    }
                }
            }

            // Now check the class for @Widget or if its in the android.widget package
            // (unless the class is hidden or abstract, or non public)
            if (!clazz.isHiddenOrRemoved() && clazz.isPublic && !clazz.modifiers.isAbstract()) {
                var annotated = false
                val annotations = clazz.modifiers.annotations()
                if (!annotations.isEmpty()) {
                    for (annotation in annotations) {
                        if (SDK_WIDGET_ANNOTATION == annotation.qualifiedName()) {
                            widgets.add(clazz)
                            annotated = true
                            break
                        } else if (SDK_LAYOUT_ANNOTATION == annotation.qualifiedName()) {
                            layouts.add(clazz)
                            annotated = true
                            break
                        }
                    }
                }

                if (!annotated) {
                    if (topLayoutParams == null && ANDROID_VIEW_VIEW_GROUP_LAYOUT_PARAMS == clazz.qualifiedName()) {
                        topLayoutParams = clazz
                    }
                    // let's check if this is inside android.widget or android.view
                    if (isIncludedPackage(clazz)) {
                        // now we check what this class inherits either from android.view.ViewGroup
                        // or android.view.View, or android.view.ViewGroup.LayoutParams
                        val type = checkInheritance(clazz)
                        when (type) {
                            TYPE_WIDGET -> widgets.add(clazz)
                            TYPE_LAYOUT -> layouts.add(clazz)
                            TYPE_LAYOUT_PARAM -> layoutParams.add(clazz)
                        }
                    }
                }
            }
        }

        // now write the files, whether or not the list are empty.
        // the SDK built requires those files to be present.

        activityActions.sort()
        writeValues("activity_actions.txt", activityActions)

        broadcastActions.sort()
        writeValues("broadcast_actions.txt", broadcastActions)

        serviceActions.sort()
        writeValues("service_actions.txt", serviceActions)

        categories.sort()
        writeValues("categories.txt", categories)

        features.sort()
        writeValues("features.txt", features)

        // Before writing the list of classes, we do some checks, to make sure the layout params
        // are enclosed by a layout class (and not one that has been declared as a widget)
        var i = 0
        while (i < layoutParams.size) {
            var clazz: ClassItem? = layoutParams[i]
            val containingClass = clazz?.containingClass()
            var remove = containingClass == null || layouts.indexOf(containingClass) == -1
            // Also ensure that super classes of the layout params are in android.widget or android.view.
            while (!remove && clazz != null) {
                clazz = clazz.superClass() ?: break
                if (clazz == topLayoutParams) {
                    break
                }
                remove = !isIncludedPackage(clazz)
            }
            if (remove) {
                layoutParams.removeAt(i)
            } else {
                i++
            }
        }

        writeClasses("widgets.txt", widgets, layouts, layoutParams)
    }

    /**
     * Check if the clazz is in package android.view or android.widget
     */
    private fun isIncludedPackage(clazz: ClassItem): Boolean {
        val pkgName = clazz.containingPackage().qualifiedName()
        return "android.widget" == pkgName || "android.view" == pkgName
    }

    /**
     * Writes a list of values into a text files.
     *
     * @param name the name of the file to write in the SDK directory
     * @param values the list of values to write.
     */
    private fun writeValues(name: String, values: List<String>) {
        val pathname = File(outputDir, name)
        var fw: FileWriter? = null
        var bw: BufferedWriter? = null
        try {
            fw = FileWriter(pathname, false)
            bw = BufferedWriter(fw)

            for (value in values) {
                bw.append(value).append('\n')
            }
        } catch (e: IOException) {
            // pass for now
        } finally {
            try {
                if (bw != null) bw.close()
            } catch (e: IOException) {
                // pass for now
            }

            try {
                if (fw != null) fw.close()
            } catch (e: IOException) {
                // pass for now
            }
        }
    }

    /**
     * Writes the widget/layout/layout param classes into a text files.
     *
     * @param name the name of the output file.
     * @param widgets the list of widget classes to write.
     * @param layouts the list of layout classes to write.
     * @param layoutParams the list of layout param classes to write.
     */
    private fun writeClasses(
        name: String,
        widgets: List<ClassItem>,
        layouts: List<ClassItem>,
        layoutParams: List<ClassItem>
    ) {
        var fw: FileWriter? = null
        var bw: BufferedWriter? = null
        try {
            val pathname = File(outputDir, name)
            fw = FileWriter(pathname, false)
            bw = BufferedWriter(fw)

            // write the 3 types of classes.
            for (clazz in widgets) {
                writeClass(bw, clazz, 'W')
            }
            for (clazz in layoutParams) {
                writeClass(bw, clazz, 'P')
            }
            for (clazz in layouts) {
                writeClass(bw, clazz, 'L')
            }
        } catch (ignore: IOException) {
        } finally {
            bw?.close()
            fw?.close()
        }
    }

    /**
     * Writes a class name and its super class names into a [BufferedWriter].
     *
     * @param writer the BufferedWriter to write into
     * @param clazz the class to write
     * @param prefix the prefix to put at the beginning of the line.
     * @throws IOException
     */
    @Throws(IOException::class)
    private fun writeClass(writer: BufferedWriter, clazz: ClassItem, prefix: Char) {
        writer.append(prefix).append(clazz.qualifiedName())
        var superClass: ClassItem? = clazz.superClass()
        while (superClass != null) {
            writer.append(' ').append(superClass.qualifiedName())
            superClass = superClass.superClass()
        }
        writer.append('\n')
    }

    /**
     * Checks the inheritance of [ClassItem] objects. This method return
     *
     *  * [.TYPE_LAYOUT]: if the class extends `android.view.ViewGroup`
     *  * [.TYPE_WIDGET]: if the class extends `android.view.View`
     *  * [.TYPE_LAYOUT_PARAM]: if the class extends
     * `android.view.ViewGroup$LayoutParams`
     *  * [.TYPE_NONE]: in all other cases
     *
     * @param clazz the [ClassItem] to check.
     */
    private fun checkInheritance(clazz: ClassItem): Int {
        return when {
            ANDROID_VIEW_VIEW_GROUP == clazz.qualifiedName() -> TYPE_LAYOUT
            ANDROID_VIEW_VIEW == clazz.qualifiedName() -> TYPE_WIDGET
            ANDROID_VIEW_VIEW_GROUP_LAYOUT_PARAMS == clazz.qualifiedName() -> TYPE_LAYOUT_PARAM
            else -> {
                val parent = clazz.superClass()
                if (parent != null) {
                    checkInheritance(parent)
                } else TYPE_NONE
            }
        }
    }
}