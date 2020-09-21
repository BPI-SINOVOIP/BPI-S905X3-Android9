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

package com.android.tools.metalava

import com.android.tools.metalava.doclava1.ApiPredicate
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.doclava1.FilterPredicate
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.Item
import java.io.File
import java.io.IOException
import java.io.PrintWriter
import java.io.StringWriter
import java.util.function.Predicate

/**
 * The [AnnotationsDiffer] can take a codebase with annotations, and subtract
 * another codebase with annotations, and emit a signature file that contains
 * *only* the signatures that have annotations that were not present in the
 * second codebase.
 *
 * The usecase for this a scenario like the following:
 * - A lot of new annotations are added to the master branch
 * - These annotations also apply to older APIs/branches, but for practical
 *   reasons we can't cherrypick the CLs that added these annotations to the
 *   older branches -- sometimes because those branches are under more strict
 *   access control, sometimes because the changes contain non-annotation
 *   changes as well, and sometimes because even a pure annotation CL won't
 *   cleanly apply to older branches since other content around the annotations
 *   have changed.
 * - We want to merge these annotations into the older codebase as an external
 *   annotation file. However, we don't really want to check in the *entire*
 *   signature file, since it's massive (which will slow down build times in
 *   that older branch), may leak new APIs etc.
 * - Solution: We can produce a "diff": create a signature file which contains
 *   *only* the signatures that have annotations from the new branch where
 *   (a) the signature is also present in the older codebase, and (b) where
 *   the annotation is not also present in the older codebase.
 *
 * That's what this codebase is used for: "take the master signature files
 * with annotations, subtract out the signature files from say the P release,
 * and check that in as the "annotations to import from master into P" delta
 * file.
 */
class AnnotationsDiffer(
    superset: Codebase,
    private val codebase: Codebase
) {
    private val relevant = HashSet<Item>(1000)

    private val predicate = object : Predicate<Item> {
        override fun test(item: Item): Boolean {
            if (relevant.contains(item)) {
                return true
            }

            val parent = item.parent() ?: return false
            return test(parent)
        }
    }

    init {
        // Limit the API to the codebase, and look at the super set codebase
        // for annotations that are only there (not in the current codebase)
        // and emit those
        val visitor = object : ComparisonVisitor() {
            override fun compare(old: Item, new: Item) {
                val newModifiers = new.modifiers
                for (annotation in old.modifiers.annotations()) {
                    var addAnnotation = false
                    if (annotation.isNullnessAnnotation()) {
                        if (!newModifiers.hasNullnessInfo()) {
                            addAnnotation = true
                        }
                    } else {
                        // TODO: Check for other incompatibilities than nullness?
                        val qualifiedName = annotation.qualifiedName() ?: continue
                        if (newModifiers.findAnnotation(qualifiedName) == null) {
                            addAnnotation = true
                        }
                    }

                    if (addAnnotation) {
                        relevant.add(new)
                    }
                }
            }
        }
        CodebaseComparator().compare(visitor, superset, codebase, ApiPredicate(codebase))
    }

    fun writeDiffSignature(apiFile: File) {
        val apiFilter = FilterPredicate(ApiPredicate(codebase))
        val apiReference = ApiPredicate(codebase, ignoreShown = true)
        val apiEmit = apiFilter.and(predicate)

        progress("\nWriting annotation diff file: ")
        try {
            val stringWriter = StringWriter()
            val writer = PrintWriter(stringWriter)
            writer.use { printWriter ->
                val preFiltered = codebase.original != null
                val apiWriter = SignatureWriter(printWriter, apiEmit, apiReference, preFiltered)
                codebase.accept(apiWriter)
            }

            // Clean up blank lines
            var prev: Char = ' '
            val cleanedUp = stringWriter.toString().filter {
                if (it == '\n' && prev == '\n')
                    false
                else {
                    prev = it
                    true
                }
            }

            apiFile.writeText(cleanedUp, Charsets.UTF_8)
        } catch (e: IOException) {
            reporter.report(Errors.IO_ERROR, apiFile, "Cannot open file for write.")
        }
    }
}