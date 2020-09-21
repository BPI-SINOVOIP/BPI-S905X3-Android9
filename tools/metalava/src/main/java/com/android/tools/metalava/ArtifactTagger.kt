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

import com.android.tools.metalava.doclava1.ApiFile
import com.android.tools.metalava.doclava1.ApiParseException
import com.android.tools.metalava.doclava1.Errors
import com.android.tools.metalava.doclava1.TextCodebase
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Codebase
import com.android.tools.metalava.model.visitors.ApiVisitor
import java.io.File

/** Registry of signature files and the corresponding artifact descriptions */
class ArtifactTagger {
    /** Ordered map from signature file to artifact description */
    private val artifacts = LinkedHashMap<File, String>()

    /** Registers the given [artifactId] for the APIs found in the given [signatureFile] */
    fun register(artifactId: String, signatureFile: File) {
        artifacts[signatureFile] = artifactId
    }

    /** Any registered artifacts? */
    fun any() = artifacts.isNotEmpty()

    /** Returns the artifacts */
    private fun getRegistrations(): Collection<Map.Entry<File, String>> = artifacts.entries

    /**
     * Applies the artifact registrations in this map to the given [codebase].
     * If [warnAboutMissing] is true, it will complain if any classes in the API
     * are found that have not been tagged (e.g. where no artifact signature file
     * referenced the API.
     */
    fun tag(codebase: Codebase, warnAboutMissing: Boolean = true) {
        if (!any()) {
            return
        }

        // Read through the XML files in order, applying their artifact information
        // to the Javadoc models.
        for (artifactSpec in getRegistrations()) {
            val xmlFile = artifactSpec.key
            val artifactName = artifactSpec.value

            val specApi: TextCodebase
            try {
                val kotlinStyleNulls = options.inputKotlinStyleNulls
                specApi = ApiFile.parseApi(xmlFile, kotlinStyleNulls, false)
            } catch (e: ApiParseException) {
                reporter.report(
                    Errors.BROKEN_ARTIFACT_FILE, xmlFile,
                    "Failed to parse $xmlFile for $artifactName artifact data.\n"
                )
                continue
            }

            applyArtifactsFromSpec(artifactName, specApi, codebase)
        }

        if (warnAboutMissing) {
            codebase.accept(object : ApiVisitor(codebase) {
                override fun visitClass(cls: ClassItem) {
                    if (cls.artifact == null && cls.isTopLevelClass()) {
                        reporter.report(
                            Errors.NO_ARTIFACT_DATA, cls,
                            "No registered artifact signature file referenced class ${cls.qualifiedName()}"
                        )
                    }
                }
            })
        }
    }

    private fun applyArtifactsFromSpec(
        mavenSpec: String,
        specApi: TextCodebase,
        codebase: Codebase
    ) {
        for (specPkg in specApi.getPackages().packages) {
            val pkg = codebase.findPackage(specPkg.qualifiedName()) ?: continue
            for (cls in pkg.allClasses()) {
                if (cls.artifact == null) {
                    cls.artifact = mavenSpec
                } else {
                    reporter.report(
                        Errors.BROKEN_ARTIFACT_FILE, cls,
                        "Class ${cls.qualifiedName()} belongs to multiple artifacts: ${cls.artifact} and $mavenSpec"
                    )
                }
            }
        }
    }
}