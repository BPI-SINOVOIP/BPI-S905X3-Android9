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
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.TypeParameterItem

class TextTypeParameterItem(
    codebase: TextCodebase,
    private val typeParameterString: String,
    name: String,
    private var bounds: List<ClassItem>? = null
) :
    TextClassItem(codebase = codebase, isPublic = true, name = name, qualifiedName = name),
    TypeParameterItem {

    override fun bounds(): List<ClassItem> {
        if (bounds == null) {
            val boundsString = bounds(typeParameterString)
            bounds = if (boundsString.isEmpty()) {
                emptyList()
            } else {
                boundsString.mapNotNull { codebase.findClass(it) }.filter { !it.isJavaLangObject() }
            }
        }
        return bounds!!
    }

    companion object {
        fun create(
            codebase: TextCodebase,
            typeParameterString: String,
            bounds: List<ClassItem>? = null
        ): TextTypeParameterItem {
            val length = typeParameterString.length
            var nameEnd = length
            for (i in 0 until length) {
                val c = typeParameterString[i]
                if (!Character.isJavaIdentifierPart(c)) {
                    nameEnd = i
                    break
                }
            }
            val name = typeParameterString.substring(0, nameEnd)
            return TextTypeParameterItem(
                codebase = codebase,
                typeParameterString = typeParameterString,
                name = name,
                bounds = bounds
            )
        }

        fun bounds(typeString: String?): List<String> {
            val s = typeString ?: return emptyList()
            val index = s.indexOf("extends ")
            if (index == -1) {
                return emptyList()
            }
            val list = mutableListOf<String>()
            var balance = 0
            var start = index + "extends ".length
            val length = s.length
            for (i in start until length) {
                val c = s[i]
                if (c == '&' && balance == 0) {
                    add(list, typeString, start, i)
                    start = i + 1
                } else if (c == '<') {
                    balance++
                    if (balance == 1) {
                        add(list, typeString, start, i)
                    }
                } else if (c == '>') {
                    balance--
                    if (balance == 0) {
                        start = i + 1
                    }
                }
            }
            if (start < length) {
                add(list, typeString, start, length)
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