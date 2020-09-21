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

package com.android.tools.metalava

import com.android.tools.metalava.NullnessMigration.Companion.findNullnessAnnotation
import com.android.tools.metalava.NullnessMigration.Companion.isNullable
import com.android.tools.metalava.doclava1.ApiPredicate
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.doclava1.Errors.Error
import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.TypeItem
import com.intellij.psi.PsiField

/**
 * Compares the current API with a previous version and makes sure
 * the changes are compatible. For example, you can make a previously
 * nullable parameter non null, but not vice versa.
 *
 * TODO: Only allow nullness changes on final classes!
 */
class CompatibilityCheck : ComparisonVisitor() {
    var foundProblems = false

    override fun compare(old: Item, new: Item) {
        // Should not remove nullness information
        // Can't change information incompatibly
        val oldNullnessAnnotation = findNullnessAnnotation(old)
        if (oldNullnessAnnotation != null) {
            val newNullnessAnnotation = findNullnessAnnotation(new)
            if (newNullnessAnnotation == null) {
                val name = AnnotationItem.simpleName(oldNullnessAnnotation)
                report(
                    Errors.INVALID_NULL_CONVERSION, new,
                    "Attempted to remove $name annotation from ${describe(new)}"
                )
            } else {
                val oldNullable = isNullable(old)
                val newNullable = isNullable(new)
                if (oldNullable != newNullable) {
                    // You can change a parameter from nonnull to nullable
                    // You can change a method from nullable to nonnull
                    // You cannot change a parameter from nullable to nonnull
                    // You cannot change a method from nonnull to nullable
                    if (oldNullable && old is ParameterItem) {
                        report(
                            Errors.INVALID_NULL_CONVERSION,
                            new,
                            "Attempted to change parameter from @Nullable to @NonNull: " +
                                "incompatible change for ${describe(new)}"
                        )
                    } else if (!oldNullable && old is MethodItem) {
                        report(
                            Errors.INVALID_NULL_CONVERSION,
                            new,
                            "Attempted to change method return from @NonNull to @Nullable: " +
                                "incompatible change for ${describe(new)}"
                        )
                    }
                }
            }
        }

        val oldModifiers = old.modifiers
        val newModifiers = new.modifiers
        if (oldModifiers.isOperator() && !newModifiers.isOperator()) {
            report(
                Errors.OPERATOR_REMOVAL,
                new,
                "Cannot remove `operator` modifier from ${describe(new)}: Incompatible change"
            )
        }

        if (oldModifiers.isInfix() && !newModifiers.isInfix()) {
            report(
                Errors.INFIX_REMOVAL,
                new,
                "Cannot remove `infix` modifier from ${describe(new)}: Incompatible change"
            )
        }
    }

    override fun compare(old: ParameterItem, new: ParameterItem) {
        val prevName = old.publicName() ?: return
        val newName = new.publicName()
        if (newName == null) {
            report(
                Errors.PARAMETER_NAME_CHANGE,
                new,
                "Attempted to remove parameter name from ${describe(new)} in ${describe(new.containingMethod())}"
            )
        } else if (newName != prevName) {
            report(
                Errors.PARAMETER_NAME_CHANGE,
                new,
                "Attempted to change parameter name from $prevName to $newName in ${describe(new.containingMethod())}"
            )
        }

        if (old.hasDefaultValue() && !new.hasDefaultValue()) {
            report(
                Errors.DEFAULT_VALUE_CHANGE,
                new,
                "Attempted to remove default value from ${describe(new)} in ${describe(new.containingMethod())}"
            )
        }

        if (old.isVarArgs() && !new.isVarArgs()) {
            // In Java, changing from array to varargs is a compatible change, but
            // not the other way around. Kotlin is the same, though in Kotlin
            // you have to change the parameter type as well to an array type; assuming you
            // do that it's the same situation as Java; otherwise the normal
            // signature check will catch the incompatibility.
            report(
                Errors.VARARG_REMOVAL,
                new,
                "Changing from varargs to array is an incompatible change: ${describe(
                    new,
                    includeParameterTypes = true,
                    includeParameterNames = true
                )}"
            )
        }
    }

    override fun compare(old: ClassItem, new: ClassItem) {
        val oldModifiers = old.modifiers
        val newModifiers = new.modifiers

        if (old.isInterface() != new.isInterface()) {
            report(
                Errors.CHANGED_CLASS, new, "${describe(new, capitalize = true)} changed class/interface declaration"
            )
            return // Avoid further warnings like "has changed abstract qualifier" which is implicit in this change
        }

        for (iface in old.interfaceTypes()) {
            val qualifiedName = iface.asClass()?.qualifiedName() ?: continue
            if (!new.implements(qualifiedName)) {
                report(
                    Errors.REMOVED_INTERFACE, new, "${describe(old, capitalize = true)} no longer implements $iface"
                )
            }
        }

        for (iface in new.interfaceTypes()) {
            val qualifiedName = iface.asClass()?.qualifiedName() ?: continue
            if (!old.implements(qualifiedName)) {
                report(
                    Errors.ADDED_INTERFACE, new, "Added interface $iface to class ${describe(old)}"
                )
            }
        }

        if (!oldModifiers.isSealed() && newModifiers.isSealed()) {
            report(Errors.ADD_SEALED, new, "Cannot add `sealed` modifier to ${describe(new)}: Incompatible change")
        } else if (oldModifiers.isAbstract() != newModifiers.isAbstract()) {
            report(
                Errors.CHANGED_ABSTRACT, new, "${describe(new, capitalize = true)} changed abstract qualifier"
            )
        }

        // Check for changes in final & static, but not in enums (since PSI and signature files differ
        // a bit in whether they include these for enums
        if (!new.isEnum()) {
            if (!oldModifiers.isFinal() && newModifiers.isFinal()) {
                // It is safe to make a class final if it did not previously have any public
                // constructors because it was impossible for an application to create a subclass.
                if (old.constructors().filter { it.isPublic || it.isProtected }.none()) {
                    report(
                        Errors.ADDED_FINAL_UNINSTANTIABLE, new,
                        "${describe(
                            new,
                            capitalize = true
                        )} added final qualifier but was previously uninstantiable and therefore could not be subclassed"
                    )
                } else {
                    report(
                        Errors.ADDED_FINAL, new, "${describe(new, capitalize = true)} added final qualifier"
                    )
                }
            } else if (oldModifiers.isFinal() && !newModifiers.isFinal()) {
                report(
                    Errors.REMOVED_FINAL, new, "${describe(new, capitalize = true)} removed final qualifier"
                )
            }

            if (oldModifiers.isStatic() != newModifiers.isStatic()) {
                report(
                    Errors.CHANGED_STATIC, new, "${describe(new, capitalize = true)} changed static qualifier"
                )
            }
        }

        val oldVisibility = oldModifiers.getVisibilityString()
        val newVisibility = newModifiers.getVisibilityString()
        if (oldVisibility != newVisibility) {
            // TODO: Use newModifiers.asAccessibleAs(oldModifiers) to provide different error messages
            // based on whether this seems like a reasonable change, e.g. making a private or final method more
            // accessible is fine (no overridden method affected) but not making methods less accessible etc
            report(
                Errors.CHANGED_SCOPE, new,
                "${describe(new, capitalize = true)} changed visibility from $oldVisibility to $newVisibility"
            )
        }

        if (!old.deprecated == new.deprecated) {
            report(
                Errors.CHANGED_DEPRECATED, new,
                "${describe(
                    new,
                    capitalize = true
                )} has changed deprecation state ${old.deprecated} --> ${new.deprecated}"
            )
        }

        val oldSuperClassName = old.superClass()?.qualifiedName()
        if (oldSuperClassName != null) { // java.lang.Object can't have a superclass.
            if (!new.extends(oldSuperClassName)) {
                report(
                    Errors.CHANGED_SUPERCLASS, new,
                    "${describe(
                        new,
                        capitalize = true
                    )} superclass changed from $oldSuperClassName to ${new.superClass()?.qualifiedName()}"
                )
            }
        }

        if (old.hasTypeVariables() && new.hasTypeVariables()) {
            val oldTypeParamsCount = old.typeParameterList().typeParameterCount()
            val newTypeParamsCount = new.typeParameterList().typeParameterCount()
            if (oldTypeParamsCount != newTypeParamsCount) {
                report(
                    Errors.CHANGED_TYPE, new,
                    "${describe(
                        old,
                        capitalize = true
                    )} changed number of type parameters from $oldTypeParamsCount to $newTypeParamsCount"
                )
            }
        }
    }

    override fun compare(old: MethodItem, new: MethodItem) {
        val oldModifiers = old.modifiers
        val newModifiers = new.modifiers

        val oldReturnType = old.returnType()
        val newReturnType = new.returnType()
        if (!new.isConstructor() && oldReturnType != null && newReturnType != null) {
            val oldTypeParameter = oldReturnType.asTypeParameter(old)
            val newTypeParameter = newReturnType.asTypeParameter(new)
            var compatible = true
            if (oldTypeParameter == null &&
                newTypeParameter == null
            ) {
                if (oldReturnType != newReturnType ||
                    oldReturnType.arrayDimensions() != newReturnType.arrayDimensions()
                ) {
                    compatible = false
                }
            } else if (oldTypeParameter == null && newTypeParameter != null) {
                val constraints = newTypeParameter.bounds()
                for (constraint in constraints) {
                    val oldClass = oldReturnType.asClass()
                    if (oldClass == null || !oldClass.extendsOrImplements(constraint.qualifiedName())) {
                        compatible = false
                    }
                }
            } else if (oldTypeParameter != null && newTypeParameter == null) {
                // It's never valid to go from being a parameterized type to not being one.
                // This would drop the implicit cast breaking backwards compatibility.
                compatible = false
            } else {
                // If both return types are parameterized then the constraints must be
                // exactly the same.
                val oldConstraints = oldTypeParameter?.bounds() ?: emptyList()
                val newConstraints = newTypeParameter?.bounds() ?: emptyList()
                if (oldConstraints.size != newConstraints.size ||
                    newConstraints != oldConstraints
                ) {
                    val oldTypeString = describeBounds(oldReturnType, oldConstraints)
                    val newTypeString = describeBounds(newReturnType, newConstraints)
                    val message =
                        "${describe(
                            new,
                            capitalize = true
                        )} has changed return type from $oldTypeString to $newTypeString"

                    report(Errors.CHANGED_TYPE, new, message)
                    return
                }
            }

            if (!compatible) {
                val oldTypeString = oldReturnType.toSimpleType()
                val newTypeString = newReturnType.toSimpleType()
                val message =
                    "${describe(new, capitalize = true)} has changed return type from $oldTypeString to $newTypeString"
                report(Errors.CHANGED_TYPE, new, message)
            }
        }

        // Check for changes in abstract, but only for regular classes; older signature files
        // sometimes describe interface methods as abstract
        if (new.containingClass().isClass()) {
            if (oldModifiers.isAbstract() != newModifiers.isAbstract()) {
                report(
                    Errors.CHANGED_ABSTRACT, new, "${describe(new, capitalize = true)} has changed 'abstract' qualifier"
                )
            }
        }

        if (oldModifiers.isNative() != newModifiers.isNative()) {
            report(
                Errors.CHANGED_NATIVE, new, "${describe(new, capitalize = true)} has changed 'native' qualifier"
            )
        }

        // Check changes to final modifier. But skip enums where it varies between signature files and PSI
        // whether the methods are considered final.
        if (!new.containingClass().isEnum()) {
            if (!oldModifiers.isStatic()) {
                // Compiler-generated methods vary in their 'final' qualifier between versions of
                // the compiler, so this check needs to be quite narrow. A change in 'final'
                // status of a method is only relevant if (a) the method is not declared 'static'
                // and (b) the method is not already inferred to be 'final' by virtue of its class.
                if (!old.isEffectivelyFinal() && new.isEffectivelyFinal()) {
                    report(
                        Errors.ADDED_FINAL, new, "${describe(new, capitalize = true)} has added 'final' qualifier"
                    )
                } else if (old.isEffectivelyFinal() && !new.isEffectivelyFinal()) {
                    report(
                        Errors.REMOVED_FINAL, new, "${describe(new, capitalize = true)} has removed 'final' qualifier"
                    )
                }
            }
        }

        if (oldModifiers.isStatic() != newModifiers.isStatic()) {
            report(
                Errors.CHANGED_STATIC, new, "${describe(new, capitalize = true)} has changed 'static' qualifier"
            )
        }

        val oldVisibility = oldModifiers.getVisibilityString()
        val newVisibility = newModifiers.getVisibilityString()
        if (oldVisibility != newVisibility) {
            // TODO: Use newModifiers.asAccessibleAs(oldModifiers) to provide different error messages
            // based on whether this seems like a reasonable change, e.g. making a private or final method more
            // accessible is fine (no overridden method affected) but not making methods less accessible etc
            report(
                Errors.CHANGED_SCOPE, new,
                "${describe(new, capitalize = true)} changed visibility from $oldVisibility to $newVisibility"
            )
        }

        if (old.deprecated != new.deprecated) {
            report(
                Errors.CHANGED_DEPRECATED, new,
                "${describe(
                    new,
                    capitalize = true
                )} has changed deprecation state ${old.deprecated} --> ${new.deprecated}"
            )
        }

        /*
        // see JLS 3 13.4.20 "Adding or deleting a synchronized modifier of a method does not break "
        // "compatibility with existing binaries."
        if (oldModifiers.isSynchronized() != newModifiers.isSynchronized()) {
            report(
                Errors.CHANGED_SYNCHRONIZED, new,
                "${describe(
                    new,
                    capitalize = true
                )} has changed 'synchronized' qualifier from ${oldModifiers.isSynchronized()} to ${newModifiers.isSynchronized()}"
            )
        }
        */

        for (exception in old.throwsTypes()) {
            if (!new.throws(exception.qualifiedName())) {
                // exclude 'throws' changes to finalize() overrides with no arguments
                if (old.name() != "finalize" || old.parameters().isNotEmpty()) {
                    report(
                        Errors.CHANGED_THROWS, new,
                        "${describe(new, capitalize = true)} no longer throws exception ${exception.qualifiedName()}"
                    )
                }
            }
        }

        for (exec in new.throwsTypes()) {
            // exclude 'throws' changes to finalize() overrides with no arguments
            if (!old.throws(exec.qualifiedName())) {
                if (old.name() != "finalize" || old.parameters().isNotEmpty()) {
                    val message = "${describe(new, capitalize = true)} added thrown exception ${exec.qualifiedName()}"
                    report(Errors.CHANGED_THROWS, new, message)
                }
            }
        }
    }

    private fun describeBounds(
        type: TypeItem,
        constraints: List<ClassItem>
    ): String {
        return type.toSimpleType() +
            if (constraints.isEmpty()) {
                " (extends java.lang.Object)"
            } else {
                " (extends ${constraints.joinToString(separator = " & ") { it.qualifiedName() }})"
            }
    }

    override fun compare(old: FieldItem, new: FieldItem) {
        val oldModifiers = old.modifiers
        val newModifiers = new.modifiers

        if (!old.isEnumConstant()) {
            val oldType = old.type()
            val newType = new.type()
            if (oldType != newType) {
                val message = "${describe(new, capitalize = true)} has changed type from $oldType to $newType"
                report(Errors.CHANGED_TYPE, new, message)
            } else if (!old.hasSameValue(new)) {
                val prevValue = old.initialValue(true)
                val prevString = if (prevValue == null && !old.modifiers.isFinal()) {
                    "nothing/not constant"
                } else {
                    prevValue
                }

                val newValue = new.initialValue(true)
                val newString = if (newValue is PsiField) {
                    newValue.containingClass?.qualifiedName + "." + newValue.name
                } else {
                    newValue
                }
                val message = "${describe(
                    new,
                    capitalize = true
                )} has changed value from $prevString to $newString"
                report(Errors.CHANGED_VALUE, new, message)
            }
        }

        val oldVisibility = oldModifiers.getVisibilityString()
        val newVisibility = newModifiers.getVisibilityString()
        if (oldVisibility != newVisibility) {
            // TODO: Use newModifiers.asAccessibleAs(oldModifiers) to provide different error messages
            // based on whether this seems like a reasonable change, e.g. making a private or final method more
            // accessible is fine (no overridden method affected) but not making methods less accessible etc
            report(
                Errors.CHANGED_SCOPE, new,
                "${describe(new, capitalize = true)} changed visibility from $oldVisibility to $newVisibility"
            )
        }

        if (oldModifiers.isStatic() != newModifiers.isStatic()) {
            report(
                Errors.CHANGED_STATIC, new, "${describe(new, capitalize = true)} has changed 'static' qualifier"
            )
        }

        if (!oldModifiers.isFinal() && newModifiers.isFinal()) {
            report(
                Errors.ADDED_FINAL, new, "${describe(new, capitalize = true)} has added 'final' qualifier"
            )
        } else if (oldModifiers.isFinal() && !newModifiers.isFinal()) {
            report(
                Errors.REMOVED_FINAL, new, "${describe(new, capitalize = true)} has removed 'final' qualifier"
            )
        }

        if (oldModifiers.isTransient() != newModifiers.isTransient()) {
            report(
                Errors.CHANGED_TRANSIENT, new, "${describe(new, capitalize = true)} has changed 'transient' qualifier"
            )
        }

        if (oldModifiers.isVolatile() != newModifiers.isVolatile()) {
            report(
                Errors.CHANGED_VOLATILE, new, "${describe(new, capitalize = true)} has changed 'volatile' qualifier"
            )
        }

        if (old.deprecated != new.deprecated) {
            report(
                Errors.CHANGED_DEPRECATED, new,
                "${describe(
                    new,
                    capitalize = true
                )} has changed deprecation state ${old.deprecated} --> ${new.deprecated}"
            )
        }
    }

    private fun handleAdded(error: Error, item: Item) {
        report(error, item, "Added ${describe(item)}")
    }

    private fun handleRemoved(error: Error, item: Item) {
        report(error, item, "Removed ${if (item.deprecated) "deprecated " else ""}${describe(item)}")
    }

    override fun added(new: PackageItem) {
        handleAdded(Errors.ADDED_PACKAGE, new)
    }

    override fun added(new: ClassItem) {
        val error = if (new.isInterface()) {
            Errors.ADDED_INTERFACE
        } else {
            Errors.ADDED_CLASS
        }
        handleAdded(error, new)
    }

    override fun added(new: MethodItem) {
        // *Overriding* methods from super classes that are outside the
        // API is OK (e.g. overriding toString() from java.lang.Object)
        val superMethods = new.superMethods()
        for (superMethod in superMethods) {
            if (superMethod.isFromClassPath()) {
                return
            }
        }

        // Do not fail if this "new" method is really an override of an
        // existing superclass method, but we should fail if this is overriding
        // an abstract method, because method's abstractness affects how users use it.
        // See if there's a member from inherited class
        val inherited = if (new.isConstructor()) {
            null
        } else {
            new.containingClass().findMethod(
                new,
                includeSuperClasses = true,
                includeInterfaces = false
            )
        }
        if (inherited != null && !inherited.modifiers.isAbstract()) {
            val error = if (new.modifiers.isAbstract()) Errors.ADDED_ABSTRACT_METHOD else Errors.ADDED_METHOD
            handleAdded(error, new)
        }
    }

    override fun added(new: FieldItem) {
        handleAdded(Errors.ADDED_FIELD, new)
    }

    override fun removed(old: PackageItem, from: Item?) {
        handleRemoved(Errors.REMOVED_PACKAGE, old)
    }

    override fun removed(old: ClassItem, from: Item?) {
        val error = when {
            old.isInterface() -> Errors.REMOVED_INTERFACE
            old.deprecated -> Errors.REMOVED_DEPRECATED_CLASS
            else -> Errors.REMOVED_CLASS
        }
        handleRemoved(error, old)
    }

    override fun removed(old: MethodItem, from: ClassItem?) {
        // See if there's a member from inherited class
        val inherited = if (old.isConstructor()) {
            null
        } else {
            from?.findMethod(
                old,
                includeSuperClasses = true,
                includeInterfaces = from.isInterface()
            )
        }
        if (inherited == null) {
            val error = if (old.deprecated) Errors.REMOVED_DEPRECATED_METHOD else Errors.REMOVED_METHOD
            handleRemoved(error, old)
        }
    }

    override fun removed(old: FieldItem, from: ClassItem?) {
        val inherited = from?.findField(
            old.name(),
            includeSuperClasses = true,
            includeInterfaces = from.isInterface()
        )
        if (inherited == null) {
            val error = if (old.deprecated) Errors.REMOVED_DEPRECATED_FIELD else Errors.REMOVED_FIELD
            handleRemoved(error, old)
        }
    }

    private fun describe(item: Item, capitalize: Boolean = false): String {
        return when (item) {
            is PackageItem -> describe(item, capitalize = capitalize)
            is ClassItem -> describe(item, capitalize = capitalize)
            is FieldItem -> describe(item, capitalize = capitalize)
            is MethodItem -> describe(
                item,
                includeParameterNames = false,
                includeParameterTypes = true,
                capitalize = capitalize
            )
            is ParameterItem -> describe(
                item,
                includeParameterNames = true,
                includeParameterTypes = true,
                capitalize = capitalize
            )
            else -> item.toString()
        }
    }

    private fun describe(
        item: MethodItem,
        includeParameterNames: Boolean = false,
        includeParameterTypes: Boolean = false,
        includeReturnValue: Boolean = false,
        capitalize: Boolean = false
    ): String {
        val builder = StringBuilder()
        if (item.isConstructor()) {
            builder.append(if (capitalize) "Constructor" else "constructor")
        } else {
            builder.append(if (capitalize) "Method" else "method")
        }
        builder.append(' ')
        if (includeReturnValue && !item.isConstructor()) {
            builder.append(item.returnType()?.toSimpleType())
            builder.append(' ')
        }
        appendMethodSignature(builder, item, includeParameterNames, includeParameterTypes)
        return builder.toString()
    }

    private fun describe(
        item: ParameterItem,
        includeParameterNames: Boolean = false,
        includeParameterTypes: Boolean = false,
        capitalize: Boolean = false
    ): String {
        val builder = StringBuilder()
        builder.append(if (capitalize) "Parameter" else "parameter")
        builder.append(' ')
        builder.append(item.name())
        builder.append(" in ")
        val method = item.containingMethod()
        appendMethodSignature(builder, method, includeParameterNames, includeParameterTypes)
        return builder.toString()
    }

    private fun appendMethodSignature(
        builder: StringBuilder,
        item: MethodItem,
        includeParameterNames: Boolean,
        includeParameterTypes: Boolean
    ) {
        builder.append(item.containingClass().qualifiedName())
        if (!item.isConstructor()) {
            builder.append('.')
            builder.append(item.name())
        }
        if (includeParameterNames || includeParameterTypes) {
            builder.append('(')
            var first = true
            for (parameter in item.parameters()) {
                if (first) {
                    first = false
                } else {
                    builder.append(',')
                    if (includeParameterNames && includeParameterTypes) {
                        builder.append(' ')
                    }
                }
                if (includeParameterTypes) {
                    builder.append(parameter.type().toSimpleType())
                    if (includeParameterNames) {
                        builder.append(' ')
                    }
                }
                if (includeParameterNames) {
                    builder.append(parameter.publicName() ?: parameter.name())
                }
            }
            builder.append(')')
        }
    }

    private fun describe(item: FieldItem, capitalize: Boolean = false): String {
        return if (item.isEnumConstant()) {
            "${if (capitalize) "Enum" else "enum"} constant ${item.containingClass().qualifiedName()}.${item.name()}"
        } else {
            "${if (capitalize) "Field" else "field"} ${item.containingClass().qualifiedName()}.${item.name()}"
        }
    }

    private fun describe(item: ClassItem, capitalize: Boolean = false): String {
        return "${if (capitalize) "Class" else "class"} ${item.qualifiedName()}"
    }

    private fun describe(item: PackageItem, capitalize: Boolean = false): String {
        return "${if (capitalize) "Package" else "package"} ${item.qualifiedName()}"
    }

    private fun report(
        error: Error,
        item: Item,
        message: String
    ) {
        reporter.report(error, item, message)
        foundProblems = true
    }

    companion object {
        fun checkCompatibility(codebase: Codebase, previous: Codebase) {
            val checker = CompatibilityCheck()
            CodebaseComparator().compare(checker, previous, codebase, ApiPredicate(codebase))
            if (checker.foundProblems) {
                throw DriverException(
                    exitCode = -1,
                    stderr = "Aborting: Found compatibility problems with --check-compatibility"
                )
            }
        }
    }
}