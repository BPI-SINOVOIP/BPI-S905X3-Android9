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

interface MemberItem : Item {
    /** The name of this method/field. Constructors have the same name as their containing class' simple name */
    fun name(): String

    /** Returns the internal name of the method, as seen in bytecode */
    fun internalName(): String = name()

    /** The containing class */
    fun containingClass(): ClassItem

    override fun parent(): ClassItem? = containingClass()

    /**
     * Returns true if this member is effectively final: it's either final itself, or implied to
     * be final because its containing class is final
     */
    fun isEffectivelyFinal(): Boolean {
        return modifiers.isFinal() || containingClass().modifiers.isFinal()
    }
}