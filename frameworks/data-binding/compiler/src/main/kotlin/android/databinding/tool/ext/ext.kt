/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.ext

import android.databinding.tool.expr.VersionProvider
import kotlin.properties.ReadOnlyProperty
import kotlin.reflect.KProperty

private class LazyExt<K, T>(private val initializer: (k : K) -> T) : ReadOnlyProperty<K, T> {
    private val mapping = hashMapOf<K, T>()
    override fun getValue(thisRef: K, property: kotlin.reflect.KProperty<*>): T {
        val t = mapping[thisRef]
        if (t != null) {
            return t
        }
        val result = initializer(thisRef)
        mapping.put(thisRef, result)
        return result
    }
}

private class VersionedLazyExt<K, T>(private val initializer: (k : K) -> T) : ReadOnlyProperty<K, T> {
    private val mapping = hashMapOf<K, VersionedResult<T>>()

    override fun getValue(thisRef: K, property: KProperty<*>): T {
        val t = mapping[thisRef]
        val version = if(thisRef is VersionProvider) thisRef.version else 1
        if (t != null && version == t.version) {
            return t.result
        }
        val result = initializer(thisRef)
        mapping.put(thisRef, VersionedResult(version, result))
        return result
    }
}

data class VersionedResult<T>(val version : Int, val result : T)

fun <K, T> lazyProp(initializer: (k : K) -> T): ReadOnlyProperty<K, T> = LazyExt(initializer)
fun <K, T> versionedLazy(initializer: (k : K) -> T): ReadOnlyProperty<K, T> = VersionedLazyExt(initializer)

public fun Class<*>.toJavaCode() : String {
    if (name.startsWith('[')) {
        val numArray = name.lastIndexOf('[') + 1;
        val componentType : String;
        when (name[numArray]) {
            'Z' -> componentType = "boolean"
            'B' -> componentType = "byte"
            'C' -> componentType = "char"
            'L' -> componentType = name.substring(numArray + 1, name.length - 1).replace('$', '.');
            'D' -> componentType = "double"
            'F' -> componentType = "float"
            'I' -> componentType = "int"
            'J' -> componentType = "long"
            'S' -> componentType = "short"
            else -> componentType = name.substring(numArray)
        }
        val arrayComp = name.substring(0, numArray).replace("[", "[]");
        return componentType + arrayComp;
    } else {
        return name.replace("$", ".")
    }
}

public fun String.androidId() : String = this.split("/")[1]

public fun String.toCamelCase() : String {
    val split = this.split("_")
    if (split.size == 0) return ""
    if (split.size == 1) return split[0].capitalize()
    return split.joinToCamelCase()
}

public fun String.toCamelCaseAsVar() : String {
    val split = this.split("_")
    if (split.size == 0) return ""
    if (split.size == 1) return split[0]
    return split.joinToCamelCaseAsVar()
}

public fun String.br() : String =
    "BR.${if (this == "") "_all" else this}"
