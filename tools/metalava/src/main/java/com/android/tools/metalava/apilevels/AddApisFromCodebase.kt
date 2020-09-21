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

package com.android.tools.metalava.apilevels

import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.visitors.ApiVisitor

/** Visits the API codebase and inserts into the [Api] the classes, methods and fields */
fun addApisFromCodebase(api: Api, apiLevel: Int, codebase: Codebase) {
    codebase.accept(object : ApiVisitor(
        codebase,
        visitConstructorsAsMethods = true,
        nestInnerClasses = false
    ) {

        var currentClass: ApiClass? = null

        override fun afterVisitClass(cls: ClassItem) {
            currentClass = null
        }

        override fun visitClass(cls: ClassItem) {
            val newClass = api.addClass(cls.internalName(), apiLevel, cls.deprecated)
            currentClass = newClass

            if (cls.isClass()) {
                // Sadly it looks like the signature files use the non-public references instead
                val filteredSuperClass = cls.filteredSuperclass(filterReference)
                val superClass = cls.superClass()
                if (filteredSuperClass != superClass && filteredSuperClass != null) {
                    val existing = newClass.superClasses.firstOrNull()?.name
                    if (existing == superClass?.internalName()) {
                        newClass.addSuperClass(superClass?.internalName(), apiLevel)
                    } else {
                        newClass.addSuperClass(filteredSuperClass.internalName(), apiLevel)
                    }
                } else if (superClass != null) {
                    newClass.addSuperClass(superClass.internalName(), apiLevel)
                }
            }

            for (interfaceType in cls.filteredInterfaceTypes(filterReference)) {
                val interfaceClass = interfaceType.asClass() ?: return
                newClass.addInterface(interfaceClass.internalName(), apiLevel)
            }
        }

        override fun visitMethod(method: MethodItem) {
            currentClass?.addMethod(
                method.internalName() +
                    // Use "V" instead of the type of the constructor for backwards compatibility
                    // with the older bytecode
                    method.internalDesc(voidConstructorTypes = true), apiLevel, method.deprecated
            )
        }

        override fun visitField(field: FieldItem) {
            // We end up moving constants from interfaces in the codebase but that's not the
            // case in older bytecode
            if (field.isCloned()) {
                return
            }
            currentClass?.addField(field.internalName(), apiLevel, field.deprecated)
        }
    })
}