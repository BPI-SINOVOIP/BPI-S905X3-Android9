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
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.visitors.ApiVisitor
import java.io.PrintWriter
import java.util.function.Predicate

class DexApiWriter(
    private val writer: PrintWriter,
    filterEmit: Predicate<Item>,
    filterReference: Predicate<Item>,
    inlineInheritedFields: Boolean = true
) : ApiVisitor(
    visitConstructorsAsMethods = true,
    nestInnerClasses = false,
    inlineInheritedFields = inlineInheritedFields,
    filterEmit = filterEmit,
    filterReference = filterReference
) {
    override fun visitClass(cls: ClassItem) {
        if (filterEmit.test(cls)) {
            writer.print(cls.toType().internalName())
            writer.print("\n")
        }
    }

    override fun visitMethod(method: MethodItem) {
        if (method.inheritedMethod) {
            return
        }

        writer.print(method.containingClass().toType().internalName())
        writer.print("->")
        writer.print(method.internalName())
        writer.print("(")
        for (pi in method.parameters()) {
            writer.print(pi.type().internalName())
        }
        writer.print(")")
        if (method.isConstructor()) {
            writer.print("V")
        } else {
            val returnType = method.returnType()
            writer.print(returnType?.internalName() ?: "V")
        }
        writer.print("\n")
    }

    override fun visitField(field: FieldItem) {
        val cls = field.containingClass()

        writer.print(cls.toType().internalName())
        writer.print("->")
        writer.print(field.name())
        writer.print(":")
        writer.print(field.type().internalName())
        writer.print("\n")
    }
}