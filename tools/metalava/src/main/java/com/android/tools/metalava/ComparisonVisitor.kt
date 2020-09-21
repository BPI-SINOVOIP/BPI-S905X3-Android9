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

import com.android.tools.metalava.model.AnnotationItem
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.ConstructorItem
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.visitors.ApiVisitor
import com.android.tools.metalava.model.visitors.VisibleItemVisitor
import com.intellij.util.containers.Stack
import java.util.Comparator
import java.util.function.Predicate

/**
 * Visitor which visits all items in two matching codebases and
 * matches up the items and invokes [compare] on each pair, or
 * [added] or [removed] when items are not matched
 */
open class ComparisonVisitor(
    /**
     * Whether constructors should be visited as part of a [#visitMethod] call
     * instead of just a [#visitConstructor] call. Helps simplify visitors that
     * don't care to distinguish between the two cases. Defaults to true.
     */
    val visitConstructorsAsMethods: Boolean = true,
    /**
     * Normally if a new item is found, the visitor will
     * only visit the top level newly added item, not all
     * of its children. This flags enables you to request
     * all individual items to also be visited.
     */
    val visitAddedItemsRecursively: Boolean = false
) {
    open fun compare(old: Item, new: Item) {}
    open fun added(new: Item) {}
    open fun removed(old: Item, from: Item?) {}

    open fun compare(old: PackageItem, new: PackageItem) {}
    open fun compare(old: ClassItem, new: ClassItem) {}
    open fun compare(old: ConstructorItem, new: ConstructorItem) {}
    open fun compare(old: MethodItem, new: MethodItem) {}
    open fun compare(old: FieldItem, new: FieldItem) {}
    open fun compare(old: ParameterItem, new: ParameterItem) {}

    open fun added(new: PackageItem) {}
    open fun added(new: ClassItem) {}
    open fun added(new: ConstructorItem) {}
    open fun added(new: MethodItem) {}
    open fun added(new: FieldItem) {}
    open fun added(new: ParameterItem) {}

    open fun removed(old: PackageItem, from: Item?) {}
    open fun removed(old: ClassItem, from: Item?) {}
    open fun removed(old: ConstructorItem, from: ClassItem?) {}
    open fun removed(old: MethodItem, from: ClassItem?) {}
    open fun removed(old: FieldItem, from: ClassItem?) {}
    open fun removed(old: ParameterItem, from: MethodItem?) {}
}

class CodebaseComparator {
    /**
     * Visits this codebase and compares it with another codebase, informing the visitors about
     * the correlations and differences that it finds
     */
    fun compare(visitor: ComparisonVisitor, old: Codebase, new: Codebase, filter: Predicate<Item>? = null) {
        // Algorithm: build up two trees (by nesting level); then visit the
        // two trees
        val oldTree = createTree(old, filter)
        val newTree = createTree(new, filter)
        compare(visitor, oldTree, newTree, null)
    }

    private fun compare(
        visitor: ComparisonVisitor,
        oldList: List<ItemTree>,
        newList: List<ItemTree>,
        newParent: Item?
    ) {
        // Debugging tip: You can print out a tree like this: ItemTree.prettyPrint(list)
        var index1 = 0
        var index2 = 0
        val length1 = oldList.size
        val length2 = newList.size

        while (true) {
            if (index1 < length1) {
                if (index2 < length2) {
                    // Compare the items
                    val oldTree = oldList[index1]
                    val newTree = newList[index2]
                    val old = oldTree.item()
                    val new = newTree.item()

                    val compare = compare(old, new)
                    when {
                        compare > 0 -> {
                            index2++
                            visitAdded(visitor, new)
                        }
                        compare < 0 -> {
                            index1++
                            visitRemoved(visitor, old, newParent)
                        }
                        else -> {
                            visitCompare(visitor, old, new)

                            // Compare the children (recurse)
                            compare(visitor, oldTree.children, newTree.children, newTree.item())

                            index1++
                            index2++
                        }
                    }
                } else {
                    // All the remaining items in oldList have been deleted
                    while (index1 < length1) {
                        visitRemoved(visitor, oldList[index1++].item(), newParent)
                    }
                }
            } else if (index2 < length2) {
                // All the remaining items in newList have been added
                while (index2 < length2) {
                    visitAdded(visitor, newList[index2++].item())
                }
            } else {
                break
            }
        }
    }

    private fun visitAdded(visitor: ComparisonVisitor, new: Item) {
        if (visitor.visitAddedItemsRecursively) {
            new.accept(object : VisibleItemVisitor() {
                override fun visitItem(item: Item) {
                    doVisitAdded(visitor, item)
                }
            })
        } else {
            doVisitAdded(visitor, new)
        }
    }

    @Suppress("USELESS_CAST") // Overloaded visitor methods: be explicit about which one is being invoked
    private fun doVisitAdded(visitor: ComparisonVisitor, item: Item) {
        visitor.added(item)

        when (item) {
            is PackageItem -> visitor.added(item)
            is ClassItem -> visitor.added(item)
            is MethodItem -> {
                if (visitor.visitConstructorsAsMethods) {
                    visitor.added(item)
                } else {
                    if (item is ConstructorItem) {
                        visitor.added(item as ConstructorItem)
                    } else {
                        visitor.added(item as MethodItem)
                    }
                }
            }
            is FieldItem -> visitor.added(item)
            is ParameterItem -> visitor.added(item)
        }
    }

    @Suppress("USELESS_CAST") // Overloaded visitor methods: be explicit about which one is being invoked
    private fun visitRemoved(visitor: ComparisonVisitor, item: Item, from: Item?) {
        visitor.removed(item, from)

        when (item) {
            is PackageItem -> visitor.removed(item, from)
            is ClassItem -> visitor.removed(item, from)
            is MethodItem -> {
                if (visitor.visitConstructorsAsMethods) {
                    visitor.removed(item, from as ClassItem?)
                } else {
                    if (item is ConstructorItem) {
                        visitor.removed(item as ConstructorItem, from as ClassItem?)
                    } else {
                        visitor.removed(item as MethodItem, from as ClassItem?)
                    }
                }
            }
            is FieldItem -> visitor.removed(item, from as ClassItem?)
            is ParameterItem -> visitor.removed(item, from as MethodItem?)
        }
    }

    @Suppress("USELESS_CAST") // Overloaded visitor methods: be explicit about which one is being invoked
    private fun visitCompare(visitor: ComparisonVisitor, old: Item, new: Item) {
        visitor.compare(old, new)

        when (old) {
            is PackageItem -> visitor.compare(old, new as PackageItem)
            is ClassItem -> visitor.compare(old, new as ClassItem)
            is MethodItem -> {
                if (visitor.visitConstructorsAsMethods) {
                    visitor.compare(old, new as MethodItem)
                } else {
                    if (old is ConstructorItem) {
                        visitor.compare(old as ConstructorItem, new as MethodItem)
                    } else {
                        visitor.compare(old as MethodItem, new as MethodItem)
                    }
                }
            }
            is FieldItem -> visitor.compare(old, new as FieldItem)
            is ParameterItem -> visitor.compare(old, new as ParameterItem)
        }
    }

    private fun compare(item1: Item, item2: Item): Int = comparator.compare(item1, item2)

    companion object {
        /** Sorting rank for types */
        private fun typeRank(item: Item): Int {
            return when (item) {
                is PackageItem -> 0
                is MethodItem -> if (item.isConstructor()) 1 else 2
                is FieldItem -> 3
                is ClassItem -> 4
                is ParameterItem -> 5
                is AnnotationItem -> 6
                else -> 7
            }
        }

        val comparator: Comparator<Item> = Comparator { item1, item2 ->
            val typeSort = typeRank(item1) - typeRank(item2)
            when {
                typeSort != 0 -> typeSort
                item1 == item2 -> 0
                else -> when (item1) {
                    is PackageItem -> {
                        item1.qualifiedName().compareTo((item2 as PackageItem).qualifiedName())
                    }
                    is ClassItem -> {
                        item1.qualifiedName().compareTo((item2 as ClassItem).qualifiedName())
                    }
                    is MethodItem -> {
                        var delta = item1.name().compareTo((item2 as MethodItem).name())
                        if (delta == 0) {
                            val parameters1 = item1.parameters()
                            val parameters2 = item2.parameters()
                            val parameterCount1 = parameters1.size
                            val parameterCount2 = parameters2.size
                            delta = parameterCount1 - parameterCount2
                            if (delta == 0) {
                                for (i in 0 until parameterCount1) {
                                    val parameter1 = parameters1[i]
                                    val parameter2 = parameters2[i]
                                    val type1 = parameter1.type().toTypeString()
                                    val type2 = parameter2.type().toTypeString()
                                    delta = type1.compareTo(type2)
                                    if (delta != 0) {
                                        // Try a little harder:
                                        //  (1) treat varargs and arrays the same, and
                                        //  (2) drop java.lang. prefixes from comparisons in wildcard
                                        //      signatures since older signature files may have removed
                                        //      those
                                        val simpleType1 = parameter1.type().toCanonicalType()
                                        val simpleType2 = parameter2.type().toCanonicalType()
                                        delta = simpleType1.compareTo(simpleType2)
                                        if (delta != 0) {
                                            break
                                        }
                                    }
                                }
                            }
                        }
                        delta
                    }
                    is FieldItem -> {
                        item1.name().compareTo((item2 as FieldItem).name())
                    }
                    is ParameterItem -> {
                        item1.parameterIndex.compareTo((item2 as ParameterItem).parameterIndex)
                    }
                    is AnnotationItem -> {
                        (item1.qualifiedName() ?: "").compareTo((item2 as AnnotationItem).qualifiedName() ?: "")
                    }
                    else -> {
                        error("Unexpected item type ${item1.javaClass}")
                    }
                }
            }
        }

        val treeComparator: Comparator<ItemTree> = Comparator { item1, item2 ->
            comparator.compare(item1.item, item2.item())
        }
    }

    private fun ensureSorted(items: MutableList<ItemTree>) {
        items.sortWith(treeComparator)
        for (item in items) {
            ensureSorted(item)
        }
    }

    private fun ensureSorted(item: ItemTree) {
        item.children.sortWith(treeComparator)
        for (child in item.children) {
            ensureSorted(child)
        }
    }

    private fun createTree(codebase: Codebase, filter: Predicate<Item>? = null): List<ItemTree> {
        // TODO: Make sure the items are sorted!
        val stack = Stack<ItemTree>()
        val root = ItemTree(null)
        stack.push(root)

        val predicate = filter ?: Predicate { true }
        // TODO: Skip empty packages
        codebase.accept(object : ApiVisitor(
            nestInnerClasses = true,
            inlineInheritedFields = true,
            filterEmit = predicate,
            filterReference = predicate
        ) {
            override fun visitItem(item: Item) {
                val node = ItemTree(item)
                val parent = stack.peek()
                parent.children += node

                stack.push(node)
            }

            override fun afterVisitItem(item: Item) {
                stack.pop()
            }
        })

        ensureSorted(root.children)
        return root.children
    }

    data class ItemTree(val item: Item?) : Comparable<ItemTree> {
        val children: MutableList<ItemTree> = mutableListOf()
        fun item(): Item = item!! // Only the root note can be null, and this method should never be called on it

        override fun compareTo(other: ItemTree): Int {
            return comparator.compare(item(), other.item())
        }

        override fun toString(): String {
            return item.toString()
        }

        @Suppress("unused") // Left for debugging
        fun prettyPrint(): String {
            val sb = StringBuilder(1000)
            prettyPrint(sb, 0)
            return sb.toString()
        }

        private fun prettyPrint(sb: StringBuilder, depth: Int) {
            for (i in 0 until depth) {
                sb.append("    ")
            }
            sb.append(toString())
            sb.append('\n')
            for (child in children) {
                child.prettyPrint(sb, depth + 1)
            }
        }

        companion object {
            @Suppress("unused") // Left for debugging
            fun prettyPrint(list: List<ItemTree>): String {
                val sb = StringBuilder(1000)
                for (child in list) {
                    child.prettyPrint(sb, 0)
                }
                return sb.toString()
            }
        }
    }
}