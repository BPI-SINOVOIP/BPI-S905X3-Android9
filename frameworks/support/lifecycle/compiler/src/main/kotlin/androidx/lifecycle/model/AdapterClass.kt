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

package androidx.lifecycle.model

import androidx.lifecycle.Lifecycling
import androidx.lifecycle.getPackage
import javax.lang.model.element.ExecutableElement
import javax.lang.model.element.TypeElement

data class AdapterClass(val type: TypeElement,
                        val calls: List<EventMethodCall>,
                        val syntheticMethods: Set<ExecutableElement>)

fun getAdapterName(type: TypeElement): String {
    val packageElement = type.getPackage()
    val qName = type.qualifiedName.toString()
    val partialName = if (packageElement.isUnnamed) qName else qName.substring(
            packageElement.qualifiedName.toString().length + 1)
    return Lifecycling.getAdapterName(partialName)
}
