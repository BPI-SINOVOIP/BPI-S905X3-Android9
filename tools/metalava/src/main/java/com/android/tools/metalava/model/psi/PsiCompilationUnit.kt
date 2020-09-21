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

package com.android.tools.metalava.model.psi

import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.CompilationUnit
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.MemberItem
import com.android.tools.metalava.model.visitors.ItemVisitor
import com.google.common.collect.ArrayListMultimap
import com.google.common.collect.Multimap
import com.intellij.psi.PsiClass
import com.intellij.psi.PsiClassOwner
import com.intellij.psi.PsiComment
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import com.intellij.psi.PsiJavaFile
import com.intellij.psi.PsiWhiteSpace
import org.jetbrains.kotlin.kdoc.psi.api.KDoc
import org.jetbrains.kotlin.psi.KtFile
import org.jetbrains.kotlin.psi.psiUtil.startOffset
import java.util.function.Predicate

class PsiCompilationUnit(val codebase: PsiBasedCodebase, containingFile: PsiFile) : CompilationUnit(containingFile) {
    override fun getHeaderComments(): String? {
        // https://youtrack.jetbrains.com/issue/KT-22135
        if (file is PsiJavaFile) {
            val pkg = file.packageStatement ?: return null
            return file.text.substring(0, pkg.startOffset)
        } else if (file is KtFile) {
            var curr: PsiElement? = file.firstChild
            var comment: String? = null
            while (curr != null) {
                if (curr is PsiComment || curr is KDoc) {
                    val text = curr.text
                    comment = if (comment != null) {
                        comment + "\n" + text
                    } else {
                        text
                    }
                } else if (curr !is PsiWhiteSpace) {
                    break
                }
                curr = curr.nextSibling
            }
            return comment
        }

        return super.getHeaderComments()
    }

    override fun getImportStatements(predicate: Predicate<Item>): Collection<Item> {
        val imports = mutableListOf<Item>()

        if (file is PsiJavaFile) {
            // TODO: Do we need to deal with static imports?
            val importList = file.importList
            if (importList != null) {
                for (importStatement in importList.importStatements) {
                    val resolved = importStatement.resolve() ?: continue
                    if (resolved is PsiClass) {
                        val classItem = codebase.findClass(resolved) ?: continue
                        if (predicate.test(classItem)) {
                            imports.add(classItem)
                        }
                    }
                }
            }

            for (psiClass in file.classes) {
                val classItem = codebase.findClass(psiClass) ?: continue
                if (predicate.test(classItem)) {
                }
            }
        } else if (file is KtFile) {
            for (importDirective in file.importDirectives) {
                val resolved = importDirective.reference?.resolve() ?: continue
                if (resolved is PsiClass) {
                    val classItem = codebase.findClass(resolved) ?: continue
                    if (predicate.test(classItem)) {
                        imports.add(classItem)
                    }
                }
            }
        }

        // Next only keep those that are present in any docs; those are the only ones
        // we need to import
        if (!imports.isEmpty()) {
            val map: Multimap<String, Item> = ArrayListMultimap.create()
            for (item in imports) {
                if (item is ClassItem) {
                    map.put(item.simpleName(), item)
                } else if (item is MemberItem) {
                    map.put(item.name(), item)
                }
            }

            // Compute set of import statements that are actually referenced
            // from the documentation (we do inexact matching here; we don't
            // need to have an exact set of imports since it's okay to have
            // some extras)
            val result = mutableListOf<Item>()
            for (cls in classes(predicate)) {
                cls.accept(object : ItemVisitor() {
                    override fun visitItem(item: Item) {
                        val doc = item.documentation
                        if (doc.isNotBlank()) {
                            var found: MutableList<String>? = null
                            for (name in map.keys()) {
                                if (doc.contains(name)) {
                                    if (found == null) {
                                        found = mutableListOf()
                                    }
                                    found.add(name)
                                }
                            }
                            found?.let {
                                for (name in found) {
                                    val all = map.get(name) ?: continue
                                    for (referenced in all) {
                                        if (!result.contains(referenced)) {
                                            result.add(referenced)
                                        }
                                    }
                                    map.removeAll(name)
                                }
                            }
                        }
                    }
                })
            }

            return result
        }

        return emptyList()
    }

    private fun classes(predicate: Predicate<Item>): List<ClassItem> {
        val topLevel = mutableListOf<ClassItem>()
        if (file is PsiClassOwner) {
            for (psiClass in file.classes) {
                val classItem = codebase.findClass(psiClass) ?: continue
                if (predicate.test(classItem)) {
                    topLevel.add(classItem)
                }
            }
        }

        return topLevel
    }
}