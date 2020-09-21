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

package com.android.tools.metalava.model.psi

import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.TypeParameterItem
import com.android.tools.metalava.model.psi.ClassType.TYPE_PARAMETER
import com.intellij.psi.PsiTypeParameter

class PsiTypeParameterItem(
    codebase: PsiBasedCodebase,
    psiClass: PsiTypeParameter,
    name: String,
    modifiers: PsiModifierItem

) : PsiClassItem(
    codebase = codebase,
    psiClass = psiClass,
    name = name,
    fullName = name,
    qualifiedName = name,
    hasImplicitDefaultConstructor = false,
    classType = TYPE_PARAMETER,
    modifiers = modifiers,
    documentation = ""
), TypeParameterItem {
    override fun bounds(): List<ClassItem> = bounds

    private lateinit var bounds: List<ClassItem>

    override fun finishInitialization() {
        super.finishInitialization()

        val refs = psiClass.extendsList?.referencedTypes
        bounds = if (refs != null && refs.isNotEmpty()) {
            // Omit java.lang.Object since PSI will turn "T extends Comparable" to "T extends Object & Comparable"
            // and this just makes comparisons harder; *everything* extends Object.
            refs.mapNotNull { PsiTypeItem.create(codebase, it).asClass() }.filter { !it.isJavaLangObject() }
        } else {
            emptyList()
        }
    }

    companion object {
        fun create(codebase: PsiBasedCodebase, psiClass: PsiTypeParameter): PsiTypeParameterItem {
            val simpleName = psiClass.name!!
            val modifiers = modifiers(codebase, psiClass, "")

            val item = PsiTypeParameterItem(
                codebase = codebase,
                psiClass = psiClass,
                name = simpleName,
                modifiers = modifiers
            )
            item.modifiers.setOwner(item)
            item.initialize(emptyList(), emptyList(), emptyList(), emptyList(), emptyList())
            return item
        }
    }
}