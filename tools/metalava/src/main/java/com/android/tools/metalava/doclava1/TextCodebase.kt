/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tools.metalava.doclava1

import com.android.tools.metalava.CodebaseComparator
import com.android.tools.metalava.ComparisonVisitor
import com.android.tools.metalava.JAVA_LANG_ANNOTATION
import com.android.tools.metalava.JAVA_LANG_ENUM
import com.android.tools.metalava.JAVA_LANG_OBJECT
import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.DefaultCodebase
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.PackageList
import com.android.tools.metalava.model.TypeParameterList
import com.android.tools.metalava.model.text.TextBackedAnnotationItem
import com.android.tools.metalava.model.text.TextClassItem
import com.android.tools.metalava.model.text.TextMethodItem
import com.android.tools.metalava.model.text.TextPackageItem
import com.android.tools.metalava.model.text.TextTypeItem
import com.android.tools.metalava.model.visitors.ItemVisitor
import com.android.tools.metalava.model.visitors.TypeVisitor
import java.util.ArrayList
import java.util.HashMap
import java.util.function.Predicate

// Copy of ApiInfo in doclava1 (converted to Kotlin + some cleanup to make it work with metalava's data structures.
// (Converted to Kotlin such that I can inherit behavior via interfaces, in particular Codebase.)
class TextCodebase : DefaultCodebase() {
    /**
     * Whether types should be interpreted to be in Kotlin format (e.g. ? suffix means nullable,
     * ! suffix means unknown, and absence of a suffix means not nullable.
     */
    var kotlinStyleNulls = false

    private val mPackages = HashMap<String, TextPackageItem>(300)
    private val mAllClasses = HashMap<String, TextClassItem>(30000)
    private val mClassToSuper = HashMap<TextClassItem, String>(30000)
    private val mClassToInterface = HashMap<TextClassItem, ArrayList<String>>(10000)

    override var description = "Codebase"

    override fun trustedApi(): Boolean = true

    override fun getPackages(): PackageList {
        val list = ArrayList<PackageItem>(mPackages.values)
        list.sortWith(PackageItem.comparator)
        return PackageList(list)
    }

    override fun size(): Int {
        return mPackages.size
    }

    override fun findClass(className: String): TextClassItem? {
        return mAllClasses[className]
    }

    private fun resolveInterfaces() {
        for (cl in mAllClasses.values) {
            val interfaces = mClassToInterface[cl] ?: continue
            for (iface in interfaces) {
                var ci: TextClassItem? = mAllClasses[iface]
                if (ci == null) {
                    // Interface not provided by this codebase. Inject a stub.
                    ci = TextClassItem.createInterfaceStub(this, iface)
                }
                cl.addInterface(ci)
            }
        }
    }

    override fun supportsDocumentation(): Boolean = false

    fun mapClassToSuper(classInfo: TextClassItem, superclass: String?) {
        superclass?.let { mClassToSuper.put(classInfo, superclass) }
    }

    fun mapClassToInterface(classInfo: TextClassItem, iface: String) {
        if (!mClassToInterface.containsKey(classInfo)) {
            mClassToInterface[classInfo] = ArrayList()
        }
        mClassToInterface[classInfo]?.add(iface)
    }

    fun implementsInterface(classInfo: TextClassItem, iface: String): Boolean {
        return mClassToInterface[classInfo]?.contains(iface) ?: false
    }

    fun addPackage(pInfo: TextPackageItem) {
        // track the set of organized packages in the API
        mPackages[pInfo.name()] = pInfo

        // accumulate a direct map of all the classes in the API
        for (cl in pInfo.allClasses()) {
            mAllClasses[cl.qualifiedName()] = cl as TextClassItem
        }
    }

    private fun resolveSuperclasses() {
        for (cl in mAllClasses.values) {
            // java.lang.Object has no superclass
            if (cl.isJavaLangObject()) {
                continue
            }
            var scName: String? = mClassToSuper[cl]
            if (scName == null) {
                scName = when {
                    cl.isEnum() -> JAVA_LANG_ENUM
                    cl.isAnnotationType() -> JAVA_LANG_ANNOTATION
                    else -> JAVA_LANG_OBJECT
                }
            }
            var superclass: TextClassItem? = mAllClasses[scName]
            if (superclass == null) {
                // Superclass not provided by this codebase. Inject a stub.
                superclass = TextClassItem.createClassStub(this, scName)
            }
            cl.setSuperClass(superclass)
        }
    }

    private fun resolveThrowsClasses() {
        for (cl in mAllClasses.values) {
            for (methodItem in cl.constructors()) {
                resolveThrowsClasses(methodItem)
            }
            for (methodItem in cl.methods()) {
                resolveThrowsClasses(methodItem)
            }

            // java.lang.Object has no superclass
            var scName: String? = mClassToSuper[cl]
            if (scName == null) {
                // Make sure we don't set java.lang.Object's super class to itself
                if (cl.qualifiedName == JAVA_LANG_OBJECT) {
                    continue
                }
                scName = JAVA_LANG_OBJECT
            }
            var superclass: TextClassItem? = mAllClasses[scName]
            if (superclass == null) {
                // Superclass not provided by this codebase. Inject a stub.
                superclass = TextClassItem.createClassStub(this, scName)
            }
            cl.setSuperClass(superclass)
        }
    }

    private fun resolveThrowsClasses(methodItem: MethodItem) {
        val methodInfo = methodItem as TextMethodItem
        val names = methodInfo.throwsTypeNames()
        if (!names.isEmpty()) {
            val result = ArrayList<TextClassItem>()
            for (exception in names) {
                var exceptionClass: TextClassItem? = mAllClasses[exception]
                if (exceptionClass == null) {
                    // Exception not provided by this codebase. Inject a stub.
                    exceptionClass = TextClassItem.createClassStub(
                        this, exception
                    )
                }
                result.add(exceptionClass)
            }
            methodInfo.setThrowsList(result)
        }
    }

    private fun resolveInnerClasses() {
        mPackages.values
            .asSequence()
            .map { it.classList().listIterator() as MutableListIterator<ClassItem> }
            .forEach {
                while (it.hasNext()) {
                    val cl = it.next() as TextClassItem
                    val name = cl.name
                    var index = name.lastIndexOf('.')
                    if (index != -1) {
                        cl.name = name.substring(index + 1)
                        val qualifiedName = cl.qualifiedName
                        index = qualifiedName.lastIndexOf('.')
                        assert(index != -1) { qualifiedName }
                        val outerClassName = qualifiedName.substring(0, index)
                        val outerClass = mAllClasses[outerClassName]!!
                        cl.containingClass = outerClass
                        outerClass.addInnerClass(cl)

                        // Should no longer be listed as top level
                        it.remove()
                    }
                }
            }
    }

    fun postProcess() {
        resolveSuperclasses()
        resolveInterfaces()
        resolveThrowsClasses()
        resolveInnerClasses()
    }

    override fun findPackage(pkgName: String): PackageItem? {
        return mPackages.values.firstOrNull { pkgName == it.qualifiedName() }
    }

    override fun accept(visitor: ItemVisitor) {
        getPackages().accept(visitor)
    }

    override fun acceptTypes(visitor: TypeVisitor) {
        getPackages().acceptTypes(visitor)
    }

    override fun compareWith(visitor: ComparisonVisitor, other: Codebase, filter: Predicate<Item>?) {
        CodebaseComparator().compare(visitor, this, other, filter)
    }

    override fun createAnnotation(source: String, context: Item?, mapName: Boolean): AnnotationItem {
        return TextBackedAnnotationItem(this, source, mapName)
    }

    override fun toString(): String {
        return description
    }

    override fun unsupported(desc: String?): Nothing {
        error(desc ?: "Not supported for a signature-file based codebase")
    }

    override fun filter(filterEmit: Predicate<Item>, filterReference: Predicate<Item>): Codebase {
        unsupported()
    }

    fun obtainTypeFromString(
        type: String,
        cl: TextClassItem,
        methodTypeParameterList: TypeParameterList
    ): TextTypeItem {
        if (TextTypeItem.isLikelyTypeParameter(type)) {
            val length = type.length
            var nameEnd = length
            for (i in 0 until length) {
                val c = type[i]
                if (c == '<' || c == '[') {
                    nameEnd = i
                    break
                }
            }
            val name = if (nameEnd == length) {
                type
            } else {
                type.substring(0, nameEnd)
            }

            val isMethodTypeVar = methodTypeParameterList.typeParameterNames().contains(name)
            val isClassTypeVar = cl.typeParameterList().typeParameterNames().contains(name)

            if (isMethodTypeVar || isClassTypeVar) {
                // Confirm that it's a type variable
                // If so, create type variable WITHOUT placing it into the
                // cache, since we can't cache these; they can have different
                // inherited bounds etc
                return TextTypeItem(this, type)
            }
        }

        return obtainTypeFromString(type)
    }

    // Copied from Converter:

    fun obtainTypeFromString(type: String): TextTypeItem {
        return mTypesFromString.obtain(type) as TextTypeItem
    }

    private val mTypesFromString = object : Cache(this) {
        override fun make(o: Any): Any {
            val name = o as String

            // Reverse effect of TypeItem.shortenTypes(...)
            if (implicitJavaLangType(name)) {
                return TextTypeItem(codebase, "java.lang.$name")
            }

            return TextTypeItem(codebase, name)
        }

        private fun implicitJavaLangType(s: String): Boolean {
            if (s.length <= 1) {
                return false // Usually a type variable
            }
            if (s[1] == '[') {
                return false // Type variable plus array
            }

            val dotIndex = s.indexOf('.')
            val array = s.indexOf('[')
            val generics = s.indexOf('<')
            if (array == -1 && generics == -1) {
                return dotIndex == -1 && !TextTypeItem.isPrimitive(s)
            }
            val typeEnd =
                if (array != -1) {
                    if (generics != -1) {
                        Math.min(array, generics)
                    } else {
                        array
                    }
                } else {
                    generics
                }

            // Allow dotted type in generic parameter, e.g. "Iterable<java.io.File>" -> return true
            return (dotIndex == -1 || dotIndex > typeEnd) && !TextTypeItem.isPrimitive(s.substring(0, typeEnd).trim())
        }
    }

    private abstract class Cache(val codebase: TextCodebase) {

        protected var mCache = HashMap<Any, Any>()

        internal fun obtain(o: Any?): Any? {
            if (o == null) {
                return null
            }
            var r: Any? = mCache[o]
            if (r == null) {
                r = make(o)
                mCache[o] = r
            }
            return r
        }

        protected abstract fun make(o: Any): Any
    }
}
