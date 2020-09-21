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

package com.android.tools.metalava.model.text

import com.android.tools.metalava.doclava1.TextCodebase
import com.android.tools.metalava.model.TypeParameterItem
import com.android.tools.metalava.model.TypeParameterList

class TextTypeParameterList(val codebase: TextCodebase, private val typeListString: String) : TypeParameterList {
    private var typeParameters: List<TypeParameterItem>? = null
    private var typeParameterNames: List<String>? = null

    override fun toString(): String = typeListString

    override fun typeParameterNames(): List<String> {
        if (typeParameterNames == null) {
//     TODO: Delete this method now that I'm doing it differently: typeParameterNames(typeListString)
            val typeParameters = typeParameters()
            val names = ArrayList<String>(typeParameters.size)
            for (parameter in typeParameters) {
                names.add(parameter.simpleName())
            }
            typeParameterNames = names
        }
        return typeParameterNames!!
    }

    override fun typeParameters(): List<TypeParameterItem> {
        if (typeParameters == null) {
            val strings = typeParameterStrings(typeListString)
            val list = ArrayList<TypeParameterItem>(strings.size)
            strings.mapTo(list) { TextTypeParameterItem.create(codebase, it) }
            typeParameters = list
        }
        return typeParameters!!
    }

    companion object {
        fun create(codebase: TextCodebase, typeListString: String): TypeParameterList {
            return TextTypeParameterList(codebase, typeListString)
        }

        fun typeParameterStrings(typeString: String?): List<String> {
            val s = typeString ?: return emptyList()
            val list = mutableListOf<String>()
            var balance = 0
            var expect = false
            var start = 0
            for (i in 0 until s.length) {
                val c = s[i]
                if (c == '<') {
                    balance++
                    expect = balance == 1
                } else if (c == '>') {
                    balance--
                    if (balance == 1) {
                        add(list, s, start, i + 1)
                        start = i + 1
                    } else if (balance == 0) {
                        add(list, s, start, i)
                        return list
                    }
                } else if (c == ',') {
                    expect = if (balance == 1) {
                        add(list, s, start, i)
                        true
                    } else {
                        false
                    }
                } else if (expect && balance == 1) {
                    start = i
                    expect = false
                }
            }
            return list
        }

        private fun add(list: MutableList<String>, s: String, from: Int, to: Int) {
            for (i in from until to) {
                if (!Character.isWhitespace(s[i])) {
                    list.add(s.substring(i, to))
                    return
                }
            }
        }
    }
}