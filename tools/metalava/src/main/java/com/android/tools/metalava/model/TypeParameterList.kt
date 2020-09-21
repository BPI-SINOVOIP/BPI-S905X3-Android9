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

package com.android.tools.metalava.model

/**
 * Represents a type parameter list. For example, in
 *     class<S, T extends List<String>>
 * the type parameter list is
 *     <S, T extends List<String>>
 * and has type parameters named S and T, and type parameter T has bounds List<String>.
 */
interface TypeParameterList {
    /**
     * Returns source representation of this type parameter, using fully qualified names
     * (possibly with java.lang. removed if requested via options)
     * */
    override fun toString(): String

    /** Returns the names of the type parameters, if any */
    fun typeParameterNames(): List<String>

    /** Returns the type parameters, if any */
    fun typeParameters(): List<TypeParameterItem>

    /** Returns the number of type parameters */
    fun typeParameterCount() = typeParameterNames().size

    companion object {
        /** Type parameter list when there are no type parameters */
        val NONE = object : TypeParameterList {
            override fun toString(): String = ""
            override fun typeParameterNames(): List<String> = emptyList()
            override fun typeParameters(): List<TypeParameterItem> = emptyList()
            override fun typeParameterCount(): Int = 0
        }
    }
}