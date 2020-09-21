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

package com.android.tools.metalava.model.psi

import com.android.tools.metalava.compatibility
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.CompilationUnit
import com.android.tools.metalava.model.ConstructorItem
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.TypeItem
import com.android.tools.metalava.model.TypeParameterList
import com.intellij.lang.jvm.types.JvmReferenceType
import com.intellij.psi.PsiClass
import com.intellij.psi.PsiClassType
import com.intellij.psi.PsiCompiledFile
import com.intellij.psi.PsiModifier
import com.intellij.psi.PsiModifierListOwner
import com.intellij.psi.PsiType
import com.intellij.psi.PsiTypeParameter
import com.intellij.psi.impl.source.PsiClassReferenceType
import com.intellij.psi.util.PsiUtil

open class PsiClassItem(
    override val codebase: PsiBasedCodebase,
    val psiClass: PsiClass,
    private val name: String,
    private val fullName: String,
    private val qualifiedName: String,
    private val hasImplicitDefaultConstructor: Boolean,
    val classType: ClassType,
    modifiers: PsiModifierItem,
    documentation: String
) :
    PsiItem(
        codebase = codebase,
        modifiers = modifiers,
        documentation = documentation,
        element = psiClass
    ), ClassItem {
    lateinit var containingPackage: PsiPackageItem

    override fun containingPackage(): PackageItem = containingClass?.containingPackage() ?: containingPackage
    override fun simpleName(): String = name
    override fun fullName(): String = fullName
    override fun qualifiedName(): String = qualifiedName
    override fun isInterface(): Boolean = classType == ClassType.INTERFACE
    override fun isAnnotationType(): Boolean = classType == ClassType.ANNOTATION_TYPE
    override fun isEnum(): Boolean = classType == ClassType.ENUM
    override fun hasImplicitDefaultConstructor(): Boolean = hasImplicitDefaultConstructor

    private var superClass: ClassItem? = null
    private var superClassType: TypeItem? = null
    override fun superClass(): ClassItem? = superClass
    override fun superClassType(): TypeItem? = superClassType

    override fun setSuperClass(superClass: ClassItem?, superClassType: TypeItem?) {
        this.superClass = superClass
        this.superClassType = superClassType
    }

    override var defaultConstructor: ConstructorItem? = null
    override var artifact: String? = null

    private var containingClass: PsiClassItem? = null
    override fun containingClass(): PsiClassItem? = containingClass
    fun setContainingClass(containingClass: ClassItem?) {
        this.containingClass = containingClass as PsiClassItem?
    }

    // TODO: Come up with a better scheme for how to compute this
    override var included: Boolean = true

    override var hasPrivateConstructor: Boolean = false

    override fun interfaceTypes(): List<TypeItem> = interfaceTypes

    override fun setInterfaceTypes(interfaceTypes: List<TypeItem>) {
        @Suppress("UNCHECKED_CAST")
        setInterfaces(interfaceTypes as List<PsiTypeItem>)
    }

    fun setInterfaces(interfaceTypes: List<PsiTypeItem>) {
        this.interfaceTypes = interfaceTypes
    }

    private var allInterfaces: List<ClassItem>? = null

    override fun allInterfaces(): Sequence<ClassItem> {
        if (allInterfaces == null) {
            val classes = mutableSetOf<PsiClass>()
            var curr: PsiClass? = psiClass
            while (curr != null) {
                if (curr.isInterface && !classes.contains(curr)) {
                    classes.add(curr)
                }
                addInterfaces(classes, curr.interfaces)
                curr = curr.superClass
            }
            val result = mutableListOf<ClassItem>()
            for (cls in classes) {
                val item = codebase.findOrCreateClass(cls)
                result.add(item)
            }

            allInterfaces = result
        }

        return allInterfaces!!.asSequence()
    }

    private fun addInterfaces(result: MutableSet<PsiClass>, interfaces: Array<out PsiClass>) {
        for (itf in interfaces) {
            if (itf.isInterface && !result.contains(itf)) {
                result.add(itf)
                addInterfaces(result, itf.interfaces)
                val superClass = itf.superClass
                if (superClass != null) {
                    addInterfaces(result, arrayOf(superClass))
                }
            }
        }
    }

    private lateinit var innerClasses: List<PsiClassItem>
    private lateinit var interfaceTypes: List<TypeItem>
    private lateinit var constructors: List<PsiConstructorItem>
    private lateinit var methods: List<PsiMethodItem>
    private lateinit var fields: List<FieldItem>

    /**
     * If this item was created by filtering down a different codebase, this temporarily
     * points to the original item during construction. This is used to let us initialize
     * for example throws lists later, when all classes in the codebase have been
     * initialized.
     */
    internal var source: PsiClassItem? = null

    override fun innerClasses(): List<PsiClassItem> = innerClasses
    override fun constructors(): List<PsiConstructorItem> = constructors
    override fun methods(): List<PsiMethodItem> = methods
    override fun fields(): List<FieldItem> = fields

    override fun toType(): TypeItem {
        return PsiTypeItem.create(codebase, codebase.getClassType(psiClass))
    }

    override fun hasTypeVariables(): Boolean = psiClass.hasTypeParameters()

    override fun typeParameterList(): TypeParameterList {
        if (psiClass.hasTypeParameters()) {
            return PsiTypeParameterList(
                codebase, psiClass.typeParameterList
                    ?: return TypeParameterList.NONE
            )
        } else {
            return TypeParameterList.NONE
        }
    }

    override fun typeArgumentClasses(): List<ClassItem> {
        return PsiTypeItem.typeParameterClasses(
            codebase,
            psiClass.typeParameterList
        )
    }

    override val isTypeParameter: Boolean
        get() = psiClass is PsiTypeParameter

    override fun getCompilationUnit(): CompilationUnit? {
        if (isInnerClass()) {
            return null
        }

        val containingFile = psiClass.containingFile ?: return null
        if (containingFile is PsiCompiledFile) {
            return null
        }

        return PsiCompilationUnit(codebase, containingFile)
    }

    override fun finishInitialization() {
        super.finishInitialization()

        for (method in methods) {
            method.finishInitialization()
        }
        for (method in constructors) {
            method.finishInitialization()
        }
        for (field in fields) {
            // There may be non-Psi fields here later (thanks to addField) but not during construction
            (field as PsiFieldItem).finishInitialization()
        }
        for (inner in innerClasses) {
            inner.finishInitialization()
        }

        val extendsListTypes = psiClass.extendsListTypes
        if (!extendsListTypes.isEmpty()) {
            val type = PsiTypeItem.create(codebase, extendsListTypes[0])
            this.superClassType = type
            this.superClass = type.asClass()
        } else {
            val superType = psiClass.superClassType
            if (superType is PsiType) {
                this.superClassType = PsiTypeItem.create(codebase, superType)
                this.superClass = this.superClassType?.asClass()
            }
        }

        // Add interfaces. If this class is an interface, it can implement both
        // classes from the extends clause and from the implements clause.
        val interfaces = psiClass.implementsListTypes
        setInterfaces(if (interfaces.isEmpty() && extendsListTypes.size <= 1) {
            emptyList()
        } else {
            val result = ArrayList<PsiTypeItem>(interfaces.size + extendsListTypes.size - 1)
            val create: (PsiClassType) -> PsiTypeItem = {
                val type = PsiTypeItem.create(codebase, it)
                type.asClass() // ensure that we initialize classes eagerly too such that they're registered etc
                type
            }
            (1 until extendsListTypes.size).mapTo(result) { create(extendsListTypes[it]) }
            interfaces.mapTo(result) { create(it) }
            result
        })
    }

    protected fun initialize(
        innerClasses: List<PsiClassItem>,
        interfaceTypes: List<TypeItem>,
        constructors: List<PsiConstructorItem>,
        methods: List<PsiMethodItem>,
        fields: List<FieldItem>
    ) {
        this.innerClasses = innerClasses
        this.interfaceTypes = interfaceTypes
        this.constructors = constructors
        this.methods = methods
        this.fields = fields
    }

    override fun mapTypeVariables(target: ClassItem, reverse: Boolean): Map<String, String> {
        val targetPsi = target.psi() as PsiClass
        val maps = mapTypeVariablesToSuperclass(
            psiClass, targetPsi, considerSuperClasses = true,
            considerInterfaces = targetPsi.isInterface
        ) ?: return emptyMap()

        if (maps.isEmpty()) {
            return emptyMap()
        }

        if (maps.size == 1) {
            return maps[0]
        }

        val first = maps[0]
        val flattened = mutableMapOf<String, String>()
        for (key in first.keys) {
            var variable: String? = key
            for (map in maps) {
                val value = map[variable]
                variable = value
                if (value == null) {
                    break
                } else {
                    flattened[key] = value
                }
            }
        }
        return flattened
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) {
            return true
        }
        return other is ClassItem && qualifiedName == other.qualifiedName()
    }

    /**
     * Creates a constructor in this class
     */
    override fun createDefaultConstructor(): ConstructorItem {
        return PsiConstructorItem.createDefaultConstructor(codebase, this, psiClass)
    }

    override fun createMethod(template: MethodItem): MethodItem {
        val method = template as PsiMethodItem

        val replacementMap = mapTypeVariables(template.containingClass(), reverse = true)

        val newMethod: PsiMethodItem
        if (replacementMap.isEmpty()) {
            newMethod = PsiMethodItem.create(codebase, this, method)
        } else {
            val stub = method.toStub(replacementMap)
            val psiMethod = codebase.createPsiMethod(stub, psiClass)
            newMethod = PsiMethodItem.create(codebase, this, psiMethod)
            newMethod.inheritedMethod = method.inheritedMethod
            newMethod.documentation = method.documentation
        }

        if (template.throwsTypes().isEmpty()) {
            newMethod.setThrowsTypes(emptyList())
        } else {
            val throwsTypes = mutableListOf<ClassItem>()
            for (type in template.throwsTypes()) {
                if (type.codebase === codebase) {
                    throwsTypes.add(type)
                } else {
                    throwsTypes.add(codebase.findOrCreateClass(((type as PsiClassItem).psiClass)))
                }
            }
            newMethod.setThrowsTypes(throwsTypes)
        }

        return newMethod
    }

    override fun addMethod(method: MethodItem) {
        (methods as MutableList<PsiMethodItem>).add(method as PsiMethodItem)
    }

    override fun hashCode(): Int = qualifiedName.hashCode()

    override fun toString(): String = "class ${qualifiedName()}"

    companion object {
        fun create(codebase: PsiBasedCodebase, psiClass: PsiClass): PsiClassItem {
            if (psiClass is PsiTypeParameter) {
                return PsiTypeParameterItem.create(codebase, psiClass)
            }
            val simpleName = psiClass.name!!
            val fullName = computeFullClassName(psiClass)
            val qualifiedName = psiClass.qualifiedName ?: simpleName
            val hasImplicitDefaultConstructor = hasImplicitDefaultConstructor(psiClass)
            val classType = ClassType.getClassType(psiClass)

            val commentText = PsiItem.javadoc(psiClass)
            val modifiers = modifiers(codebase, psiClass, commentText)
            val item = PsiClassItem(
                codebase = codebase,
                psiClass = psiClass,
                name = simpleName,
                fullName = fullName,
                qualifiedName = qualifiedName,
                classType = classType,
                hasImplicitDefaultConstructor = hasImplicitDefaultConstructor,
                documentation = commentText,
                modifiers = modifiers
            )
            codebase.registerClass(item)
            item.modifiers.setOwner(item)

            // Construct the children
            val psiMethods = psiClass.methods
            val methods: MutableList<PsiMethodItem> = ArrayList(psiMethods.size)

            if (classType == ClassType.ENUM) {
                addEnumMethods(codebase, item, psiClass, methods)
            }

            val constructors: MutableList<PsiConstructorItem> = ArrayList(5)
            for (psiMethod in psiMethods) {
                if (psiMethod.isPrivate() || psiMethod.isPackagePrivate()) {
                    item.hasPrivateConstructor = true
                }
                if (psiMethod.isConstructor) {
                    val constructor = PsiConstructorItem.create(codebase, item, psiMethod)
                    constructors.add(constructor)
                } else {
                    val method = PsiMethodItem.create(codebase, item, psiMethod)
                    methods.add(method)
                }
            }

            if (hasImplicitDefaultConstructor) {
                assert(constructors.isEmpty())
                constructors.add(PsiConstructorItem.createDefaultConstructor(codebase, item, psiClass))
            }

            val fields: MutableList<FieldItem> = mutableListOf()
            val psiFields = psiClass.fields
            if (!psiFields.isEmpty()) {
                psiFields.asSequence()
                    .mapTo(fields) {
                        PsiFieldItem.create(codebase, item, it)
                    }
            }

            if (classType == ClassType.INTERFACE) {
                // All members are implicitly public, fields are implicitly static, non-static methods are abstract
                // (except in Java 1.9, where they can be private
                for (method in methods) {
                    if (!method.isPrivate) {
                        method.mutableModifiers().setPublic(true)
                    }
                }
                for (method in fields) {
                    val m = method.mutableModifiers()
                    m.setPublic(true)
                    m.setStatic(true)
                }
            }

            item.constructors = constructors
            item.methods = methods
            item.fields = fields

            val psiInnerClasses = psiClass.innerClasses
            item.innerClasses = if (psiInnerClasses.isEmpty()) {
                emptyList()
            } else {
                val result = psiInnerClasses.asSequence()
                    .map {
                        val inner = codebase.findOrCreateClass(it)
                        inner.containingClass = item
                        inner
                    }
                    .toMutableList()
                result
            }

            return item
        }

        fun create(codebase: PsiBasedCodebase, classFilter: FilteredClassView): PsiClassItem {
            val original = classFilter.cls

            val newClass = PsiClassItem(
                codebase = codebase,
                psiClass = original.psiClass,
                name = original.name,
                fullName = original.fullName,
                qualifiedName = original.qualifiedName,
                classType = original.classType,
                hasImplicitDefaultConstructor = original.hasImplicitDefaultConstructor,
                documentation = original.documentation,
                modifiers = PsiModifierItem.create(codebase, original.modifiers)
            )

            newClass.modifiers.setOwner(newClass)
            codebase.registerClass(newClass)
            newClass.source = original

            newClass.constructors = classFilter.constructors.map {
                PsiConstructorItem.create(codebase, newClass, it as PsiConstructorItem)
            }.toMutableList()

            newClass.methods = classFilter.methods.map {
                PsiMethodItem.create(codebase, newClass, it as PsiMethodItem)
            }.toMutableList()

            newClass.fields = classFilter.fields.asSequence()
                // Preserve sorting order for enums
                .sortedBy { it.sortingRank }.map {
                    PsiFieldItem.create(codebase, newClass, it as PsiFieldItem)
                }.toMutableList()

            newClass.innerClasses = classFilter.innerClasses.map {
                val newInnerClass = codebase.findClass(it.cls.qualifiedName) ?: it.create(codebase)
                newInnerClass.containingClass = newClass
                codebase.registerClass(newInnerClass)
                newInnerClass
            }.toMutableList()

            newClass.hasPrivateConstructor = classFilter.cls.hasPrivateConstructor

            return newClass
        }

        private fun addEnumMethods(
            codebase: PsiBasedCodebase,
            classItem: PsiClassItem,
            psiClass: PsiClass,
            result: MutableList<PsiMethodItem>
        ) {
            // Add these two methods as overrides into the API; this isn't necessary but is done in the old
            // API generator
            //    method public static android.graphics.ColorSpace.Adaptation valueOf(java.lang.String);
            //    method public static final android.graphics.ColorSpace.Adaptation[] values();

            if (compatibility.defaultAnnotationMethods) {
                // TODO: Skip if we already have these methods here (but that shouldn't happen; nobody would
                // type this by hand)
                addEnumMethod(
                    codebase, classItem,
                    psiClass, result,
                    "public static ${psiClass.qualifiedName} valueOf(java.lang.String s) { return null; }"
                )
                addEnumMethod(
                    codebase, classItem,
                    psiClass, result,
                    "public static final ${psiClass.qualifiedName}[] values() { return null; }"
                )
                // Also add a private constructor; used when emitting the private API
                val psiMethod = codebase.createConstructor("private ${psiClass.name}", psiClass)
                result.add(PsiConstructorItem.create(codebase, classItem, psiMethod))
            }
        }

        private fun addEnumMethod(
            codebase: PsiBasedCodebase,
            classItem: PsiClassItem,
            psiClass: PsiClass,
            result: MutableList<PsiMethodItem>,
            source: String
        ) {
            val psiMethod = codebase.createPsiMethod(source, psiClass)
            result.add(PsiMethodItem.create(codebase, classItem, psiMethod))
        }

        /**
         * Computes the "full" class name; this is not the qualified class name (e.g. with package)
         * but for an inner class it includes all the outer classes
         */
        private fun computeFullClassName(cls: PsiClass): String {
            if (cls.containingClass == null) {
                val name = cls.name
                return name!!
            } else {
                val list = mutableListOf<String>()
                var curr: PsiClass? = cls
                while (curr != null) {
                    val name = curr.name
                    curr = if (name != null) {
                        list.add(name)
                        curr.containingClass
                    } else {
                        break
                    }
                }
                return list.asReversed().asSequence().joinToString(separator = ".") { it }
            }
        }

        private fun hasImplicitDefaultConstructor(psiClass: PsiClass): Boolean {
            if (psiClass.name?.startsWith("-") == true) {
                // Deliberately hidden; see examples like
                //     @file:JvmName("-ViewModelExtensions") // Hide from Java sources in the IDE.
                return false
            }

            val constructors = psiClass.constructors
            if (constructors.isEmpty() && !psiClass.isInterface && !psiClass.isAnnotationType && !psiClass.isEnum) {
                if (PsiUtil.hasDefaultConstructor(psiClass)) {
                    return true
                }

                // The above method isn't always right; for example, for the ContactsContract.Presence class
                // in the framework, which looks like this:
                //    @Deprecated
                //    public static final class Presence extends StatusUpdates {
                //    }
                // javac makes a default constructor:
                //    public final class android.provider.ContactsContract$Presence extends android.provider.ContactsContract$StatusUpdates {
                //        public android.provider.ContactsContract$Presence();
                //    }
                // but the above method returns false. So add some of our own heuristics:
                if (psiClass.hasModifierProperty(PsiModifier.FINAL) && !psiClass.hasModifierProperty(
                        PsiModifier.ABSTRACT
                    ) &&
                    psiClass.hasModifierProperty(PsiModifier.PUBLIC)
                ) {
                    return true
                }
            }

            return false
        }

        fun mapTypeVariablesToSuperclass(
            psiClass: PsiClass,
            targetClass: PsiClass,
            considerSuperClasses: Boolean = true,
            considerInterfaces: Boolean = psiClass.isInterface
        ): MutableList<Map<String, String>>? {
            // TODO: Prune search if type doesn't have type arguments!
            if (considerSuperClasses) {
                val list = mapTypeVariablesToSuperclass(
                    psiClass.superClassType, targetClass,
                    considerSuperClasses, considerInterfaces
                )
                if (list != null) {
                    return list
                }
            }

            if (considerInterfaces) {
                for (interfaceType in psiClass.interfaceTypes) {
                    val list = mapTypeVariablesToSuperclass(
                        interfaceType, targetClass,
                        considerSuperClasses, considerInterfaces
                    )
                    if (list != null) {
                        return list
                    }
                }
            }

            return null
        }

        private fun mapTypeVariablesToSuperclass(
            type: JvmReferenceType?,
            targetClass: PsiClass,
            considerSuperClasses: Boolean = true,
            considerInterfaces: Boolean = true
        ): MutableList<Map<String, String>>? {
            // TODO: Prune search if type doesn't have type arguments!
            val superType = type as? PsiClassReferenceType
            val superClass = superType?.resolve()
            if (superClass != null) {
                if (superClass == targetClass) {
                    val map = mapTypeVariablesToSuperclass(superType)
                    return if (map != null) {
                        mutableListOf(map)
                    } else {
                        null
                    }
                } else {
                    val list = mapTypeVariablesToSuperclass(
                        superClass, targetClass, considerSuperClasses,
                        considerInterfaces
                    )
                    if (list != null) {
                        val map = mapTypeVariablesToSuperclass(superType)
                        if (map != null) {
                            list.add(map)
                        }
                        return list
                    }
                }
            }

            return null
        }

        private fun mapTypeVariablesToSuperclass(superType: PsiClassReferenceType?): Map<String, String>? {
            superType ?: return null

            val map = mutableMapOf<String, String>()
            val superClass = superType.resolve()
            if (superClass != null && superType.hasParameters()) {
                val superTypeParameters = superClass.typeParameters
                superType.parameters.forEachIndexed { index, parameter ->
                    if (parameter is PsiClassReferenceType) {
                        val parameterClass = parameter.resolve()
                        if (parameterClass != null) {
                            val parameterName = parameterClass.qualifiedName ?: parameterClass.name ?: parameter.name
                            if (index < superTypeParameters.size) {
                                val superTypeParameter = superTypeParameters[index]
                                val superTypeName = superTypeParameter.qualifiedName ?: superTypeParameter.name
                                if (superTypeName != null) {
                                    map[superTypeName] = parameterName
                                }
                            }
                        }
                    }
                }
            }

            return map
        }
    }
}

fun PsiModifierListOwner.isPrivate(): Boolean = modifierList?.hasExplicitModifier(PsiModifier.PRIVATE) == true
fun PsiModifierListOwner.isPackagePrivate(): Boolean {
    val modifiers = modifierList ?: return false
    return !(modifiers.hasModifierProperty(PsiModifier.PUBLIC) ||
        modifiers.hasModifierProperty(PsiModifier.PROTECTED))
}