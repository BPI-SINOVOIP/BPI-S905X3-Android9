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
import com.android.tools.metalava.model.FieldItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.ParameterItem
import com.android.tools.metalava.model.TypeItem

/**
 * Performs null migration analysis, looking at previous API signature
 * files and new signature files, and replacing new @Nullable and @NonNull
 * annotations with @RecentlyNullable and @RecentlyNonNull.
 *
 * TODO: Enforce compatibility across type use annotations, e.g.
 * changing parameter value from
 *    {@code @NonNull List<@Nullable String>}
 * to
 *    {@code @NonNull List<@NonNull String>}
 * is forbidden.
 */
class NullnessMigration : ComparisonVisitor(visitAddedItemsRecursively = true) {
    override fun compare(old: Item, new: Item) {
        if (hasNullnessInformation(new) && !hasNullnessInformation(old)) {
            markRecent(new)
        }
    }

    override fun added(new: Item) {
        // Translate newly added items into RecentlyNull/RecentlyNonNull
        if (hasNullnessInformation(new)) {
            markRecent(new)
        }
    }
    override fun compare(old: MethodItem, new: MethodItem) {
        val newType = new.returnType() ?: return
        val oldType = old.returnType() ?: return
        checkType(oldType, newType)
    }

    override fun compare(old: FieldItem, new: FieldItem) {
        val newType = new.type()
        val oldType = old.type()
        checkType(oldType, newType)
    }

    override fun compare(old: ParameterItem, new: ParameterItem) {
        val newType = new.type()
        val oldType = old.type()
        checkType(oldType, newType)
    }

    override fun added(new: MethodItem) {
        checkType(new.returnType() ?: return)
    }

    override fun added(new: FieldItem) {
        checkType(new.type())
    }

    override fun added(new: ParameterItem) {
        checkType(new.type())
    }

    private fun hasNullnessInformation(type: TypeItem): Boolean {
        val typeString = type.toTypeString(false, true, false)
        return typeString.contains(".Nullable") || typeString.contains(".NonNull")
    }

    private fun checkType(old: TypeItem, new: TypeItem) {
        if (hasNullnessInformation(new)) {
            if (old.toTypeString(false, true, false) !=
                new.toTypeString(false, true, false)) {
                new.markRecent()
            }
        }
    }

    private fun checkType(new: TypeItem) {
        if (hasNullnessInformation(new)) {
            new.markRecent()
        }
    }

    private fun markRecent(new: Item) {
        val annotation = findNullnessAnnotation(new) ?: return
        // Nullness information change: Add migration annotation
        val annotationClass = if (annotation.isNullable()) RECENTLY_NULLABLE else RECENTLY_NONNULL

        val modifiers = new.mutableModifiers()
        modifiers.removeAnnotation(annotation)

        // Don't map annotation names - this would turn newly non null back into non null
        modifiers.addAnnotation(new.codebase.createAnnotation("@$annotationClass", new, mapName = false))
    }

    companion object {
        fun hasNullnessInformation(item: Item): Boolean {
            return isNullable(item) || isNonNull(item)
        }

        fun findNullnessAnnotation(item: Item): AnnotationItem? {
            return item.modifiers.annotations().firstOrNull { it.isNullnessAnnotation() }
        }

        fun isNullable(item: Item): Boolean {
            return item.modifiers.annotations().any { it.isNullable() }
        }

        private fun isNonNull(item: Item): Boolean {
            return item.modifiers.annotations().any { it.isNonNull() }
        }

        private fun isRecentlyMigrated(item: Item): Boolean {
            return item.modifiers.annotations().any { isRecentlyMigrated(it.qualifiedName() ?: "") }
        }

        private fun isRecentlyMigrated(qualifiedName: String): Boolean {
            return qualifiedName.endsWith(".RecentlyNullable") ||
                qualifiedName.endsWith(".RecentlyNonNull")
        }
    }
}
