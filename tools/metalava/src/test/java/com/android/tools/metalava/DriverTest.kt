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

import com.android.SdkConstants
import com.android.SdkConstants.DOT_JAVA
import com.android.SdkConstants.DOT_KT
import com.android.SdkConstants.VALUE_TRUE
import com.android.ide.common.process.DefaultProcessExecutor
import com.android.ide.common.process.LoggedProcessOutputHandler
import com.android.ide.common.process.ProcessException
import com.android.ide.common.process.ProcessInfoBuilder
import com.android.tools.lint.checks.ApiLookup
import com.android.tools.lint.checks.infrastructure.LintDetectorTest
import com.android.tools.lint.checks.infrastructure.TestFile
import com.android.tools.lint.checks.infrastructure.TestFiles
import com.android.tools.lint.checks.infrastructure.TestFiles.java
import com.android.tools.lint.checks.infrastructure.stripComments
import com.android.tools.metalava.doclava1.Errors
import com.android.utils.FileUtils
import com.android.utils.SdkUtils
import com.android.utils.StdLogger
import com.google.common.base.Charsets
import com.google.common.io.ByteStreams
import com.google.common.io.Closeables
import com.google.common.io.Files
import org.intellij.lang.annotations.Language
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Rule
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.PrintWriter
import java.io.StringWriter
import java.net.URL

const val CHECK_OLD_DOCLAVA_TOO = false
const val CHECK_STUB_COMPILATION = false
const val SKIP_NON_COMPAT = false

abstract class DriverTest {
    @get:Rule
    var temporaryFolder = TemporaryFolder()

    protected fun createProject(vararg files: TestFile): File {
        val dir = temporaryFolder.newFolder()

        files
            .map { it.createFile(dir) }
            .forEach { assertNotNull(it) }

        return dir
    }

    protected fun runDriver(vararg args: String, expectedFail: String = ""): String {
        resetTicker()

        val sw = StringWriter()
        val writer = PrintWriter(sw)
        if (!com.android.tools.metalava.run(arrayOf(*args), writer, writer)) {
            val actualFail = sw.toString().trim()
            if (expectedFail != actualFail.replace(".", "").trim()) {
                fail(actualFail)
            }
        }

        return sw.toString()
    }

    private fun findKotlinStdlibPath(): List<String> {
        val classPath: String = System.getProperty("java.class.path")
        val paths = mutableListOf<String>()
        for (path in classPath.split(':')) {
            val file = File(path)
            val name = file.name
            if (name.startsWith("kotlin-stdlib") ||
                name.startsWith("kotlin-reflect") ||
                name.startsWith("kotlin-script-runtime")
            ) {
                paths.add(file.path)
            }
        }
        if (paths.isEmpty()) {
            error("Did not find kotlin-stdlib-jre8 in $PROGRAM_NAME classpath: $classPath")
        }
        return paths
    }

    protected fun getJdkPath(): String? {
        val javaHome = System.getProperty("java.home")
        if (javaHome != null) {
            var javaHomeFile = File(javaHome)
            if (File(javaHomeFile, "bin${File.separator}javac").exists()) {
                return javaHome
            } else if (javaHomeFile.name == "jre") {
                javaHomeFile = javaHomeFile.parentFile
                if (javaHomeFile != null && File(javaHomeFile, "bin${File.separator}javac").exists()) {
                    return javaHomeFile.path
                }
            }
        }
        return System.getenv("JAVA_HOME")
    }

    protected fun check(
        /** The source files to pass to the analyzer */
        vararg sourceFiles: TestFile,
        /** The API signature content (corresponds to --api) */
        api: String? = null,
        /** The exact API signature content (corresponds to --exact-api) */
        exactApi: String? = null,
        /** The removed API (corresponds to --removed-api) */
        removedApi: String? = null,
        /** The removed dex API (corresponds to --removed-dex-api) */
        removedDexApi: String? = null,
        /** The private API (corresponds to --private-api) */
        privateApi: String? = null,
        /** The private DEX API (corresponds to --private-dex-api) */
        privateDexApi: String? = null,
        /** The DEX API (corresponds to --dex-api) */
        dexApi: String? = null,
        /** Expected stubs (corresponds to --stubs) */
        @Language("JAVA") stubs: Array<String> = emptyArray(),
        /** Stub source file list generated */
        stubsSourceList: String? = null,
        /** Whether the stubs should be written as documentation stubs instead of plain stubs. Decides
         * whether the stubs include @doconly elements, uses rewritten/migration annotations, etc */
        docStubs: Boolean = false,
        /** Whether to run in doclava1 compat mode */
        compatibilityMode: Boolean = true,
        /** Whether to trim the output (leading/trailing whitespace removal) */
        trim: Boolean = true,
        /** Whether to remove blank lines in the output (the signature file usually contains a lot of these) */
        stripBlankLines: Boolean = true,
        /** Warnings expected to be generated when analyzing these sources */
        warnings: String? = "",
        /** Whether to run doclava1 on the test output and assert that the output is identical */
        checkDoclava1: Boolean = compatibilityMode,
        checkCompilation: Boolean = false,
        /** Annotations to merge in (in .xml format) */
        @Language("XML") mergeXmlAnnotations: String? = null,
        /** Annotations to merge in (in .jaif format) */
        @Language("TEXT") mergeJaifAnnotations: String? = null,
        /** Annotations to merge in (in .txt/.signature format) */
        @Language("TEXT") mergeSignatureAnnotations: String? = null,
        /** An optional API signature file content to load **instead** of Java/Kotlin source files */
        @Language("TEXT") signatureSource: String? = null,
        /** An optional API jar file content to load **instead** of Java/Kotlin source files */
        apiJar: File? = null,
        /** An optional API signature representing the previous API level to diff */
        @Language("TEXT") previousApi: String? = null,
        /** An optional Proguard keep file to generate */
        @Language("Proguard") proguard: String? = null,
        /** Whether we should migrate nullness information */
        migrateNulls: Boolean = false,
        /** Whether we should check compatibility */
        checkCompatibility: Boolean = false,
        /** Show annotations (--show-annotation arguments) */
        showAnnotations: Array<String> = emptyArray(),
        /** If using [showAnnotations], whether to include unannotated */
        showUnannotated: Boolean = false,
        /** Additional arguments to supply */
        extraArguments: Array<String> = emptyArray(),
        /** Whether we should emit Kotlin-style null signatures */
        outputKotlinStyleNulls: Boolean = !compatibilityMode,
        /** Whether we should interpret API files being read as having Kotlin-style nullness types */
        inputKotlinStyleNulls: Boolean = false,
        /** Whether we should omit java.lang. etc from signature files */
        omitCommonPackages: Boolean = !compatibilityMode,
        /** Expected output (stdout and stderr combined). If null, don't check. */
        expectedOutput: String? = null,
        /** List of extra jar files to record annotation coverage from */
        coverageJars: Array<TestFile>? = null,
        /** Optional manifest to load and associate with the codebase */
        @Language("XML")
        manifest: String? = null,
        /** Packages to pre-import (these will therefore NOT be included in emitted stubs, signature files etc */
        importedPackages: List<String> = emptyList(),
        /** Packages to skip emitting signatures/stubs for even if public (typically used for unit tests
         * referencing to classpath classes that aren't part of the definitions and shouldn't be part of the
         * test output; e.g. a test may reference java.lang.Enum but we don't want to start reporting all the
         * public APIs in the java.lang package just because it's indirectly referenced via the "enum" superclass
         */
        skipEmitPackages: List<String> = listOf("java.lang", "java.util", "java.io"),
        /** Whether we should include --showAnnotations=android.annotation.SystemApi */
        includeSystemApiAnnotations: Boolean = false,
        /** Whether we should warn about super classes that are stripped because they are hidden */
        includeStrippedSuperclassWarnings: Boolean = false,
        /** Apply level to XML */
        applyApiLevelsXml: String? = null,
        /** Corresponds to SDK constants file broadcast_actions.txt */
        sdk_broadcast_actions: String? = null,
        /** Corresponds to SDK constants file activity_actions.txt */
        sdk_activity_actions: String? = null,
        /** Corresponds to SDK constants file service_actions.txt */
        sdk_service_actions: String? = null,
        /** Corresponds to SDK constants file categories.txt */
        sdk_categories: String? = null,
        /** Corresponds to SDK constants file features.txt */
        sdk_features: String? = null,
        /** Corresponds to SDK constants file widgets.txt */
        sdk_widgets: String? = null,
        /** Map from artifact id to artifact descriptor */
        artifacts: Map<String, String>? = null,
        /** Extract annotations and check that the given packages contain the given extracted XML files */
        extractAnnotations: Map<String, String>? = null
    ) {
        System.setProperty("METALAVA_TESTS_RUNNING", VALUE_TRUE)

        if (compatibilityMode && mergeXmlAnnotations != null) {
            fail(
                "Can't specify both compatibilityMode and mergeAnnotations: there were no " +
                    "annotations output in doclava1"
            )
        }
        if (compatibilityMode && mergeJaifAnnotations != null) {
            fail(
                "Can't specify both compatibilityMode and mergeJaifAnnotations: there were no " +
                    "annotations output in doclava1"
            )
        }

        if (compatibilityMode && mergeSignatureAnnotations != null) {
            fail(
                "Can't specify both compatibilityMode and mergeSignatureAnnotations: there were no " +
                    "annotations output in doclava1"
            )
        }
        Errors.resetLevels()

        /** Expected output if exiting with an error code */
        val expectedFail = if (checkCompatibility) {
            "Aborting: Found compatibility problems with --check-compatibility"
        } else {
            ""
        }

        // Unit test which checks that a signature file is as expected
        val androidJar = getPlatformFile("android.jar")

        val project = createProject(*sourceFiles)

        val packages = sourceFiles.asSequence().map { findPackage(it.getContents()!!) }.filterNotNull().toSet()

        val sourcePathDir = File(project, "src")
        val sourcePath = sourcePathDir.path
        val sourceList =
            if (signatureSource != null) {
                sourcePathDir.mkdirs()
                assert(sourceFiles.isEmpty()) { "Shouldn't combine sources with signature file loads" }
                val signatureFile = File(project, "load-api.txt")
                Files.asCharSink(signatureFile, Charsets.UTF_8).write(signatureSource.trimIndent())
                if (includeStrippedSuperclassWarnings) {
                    arrayOf(signatureFile.path)
                } else {
                    arrayOf(
                        signatureFile.path,
                        "--hide",
                        "HiddenSuperclass"
                    ) // Suppress warning #111
                }
            } else if (apiJar != null) {
                sourcePathDir.mkdirs()
                assert(sourceFiles.isEmpty()) { "Shouldn't combine sources with API jar file loads" }
                arrayOf(apiJar.path)
            } else {
                sourceFiles.asSequence().map { File(project, it.targetPath).path }.toList().toTypedArray()
            }

        val reportedWarnings = StringBuilder()
        reporter = object : Reporter(project) {
            override fun print(message: String) {
                reportedWarnings.append(message.replace(project.path, "TESTROOT").trim()).append('\n')
            }
        }

        val mergeAnnotationsArgs = if (mergeXmlAnnotations != null) {
            val merged = File(project, "merged-annotations.xml")
            Files.asCharSink(merged, Charsets.UTF_8).write(mergeXmlAnnotations.trimIndent())
            arrayOf("--merge-annotations", merged.path)
        } else {
            emptyArray()
        }

        val jaifAnnotationsArgs = if (mergeJaifAnnotations != null) {
            val merged = File(project, "merged-annotations.jaif")
            Files.asCharSink(merged, Charsets.UTF_8).write(mergeJaifAnnotations.trimIndent())
            arrayOf("--merge-annotations", merged.path)
        } else {
            emptyArray()
        }

        val signatureAnnotationsArgs = if (mergeSignatureAnnotations != null) {
            val merged = File(project, "merged-annotations.txt")
            Files.asCharSink(merged, Charsets.UTF_8).write(mergeSignatureAnnotations.trimIndent())
            arrayOf("--merge-annotations", merged.path)
        } else {
            emptyArray()
        }

        val previousApiFile = if (previousApi != null) {
            val prevApiJar = File(previousApi)
            if (prevApiJar.isFile) {
                prevApiJar
            } else {
                val file = File(project, "previous-api.txt")
                Files.asCharSink(file, Charsets.UTF_8).write(previousApi.trimIndent())
                file
            }
        } else {
            null
        }

        val previousApiArgs = if (previousApiFile != null) {
            arrayOf("--previous-api", previousApiFile.path)
        } else {
            emptyArray()
        }

        val manifestFileArgs = if (manifest != null) {
            val file = File(project, "manifest.xml")
            Files.asCharSink(file, Charsets.UTF_8).write(manifest.trimIndent())
            arrayOf("--manifest", file.path)
        } else {
            emptyArray()
        }

        val migrateNullsArguments = if (migrateNulls) {
            arrayOf("--migrate-nullness")
        } else {
            emptyArray()
        }

        val checkCompatibilityArguments = if (checkCompatibility) {
            arrayOf("--check-compatibility")
        } else {
            emptyArray()
        }

        val quiet = if (expectedOutput != null && !extraArguments.contains("--verbose")) {
            // If comparing output, avoid noisy output such as the banner etc
            arrayOf("--quiet")
        } else {
            emptyArray()
        }

        val coverageStats = if (coverageJars != null && coverageJars.isNotEmpty()) {
            val sb = StringBuilder()
            val root = File(project, "coverageJars")
            root.mkdirs()
            for (jar in coverageJars) {
                if (sb.isNotEmpty()) {
                    sb.append(File.pathSeparator)
                }
                val file = jar.createFile(root)
                sb.append(file.path)
            }
            arrayOf("--annotation-coverage-of", sb.toString())
        } else {
            emptyArray()
        }

        var proguardFile: File? = null
        val proguardKeepArguments = if (proguard != null) {
            proguardFile = File(project, "proguard.cfg")
            arrayOf("--proguard", proguardFile.path)
        } else {
            emptyArray()
        }

        val showAnnotationArguments = if (showAnnotations.isNotEmpty() || includeSystemApiAnnotations) {
            val args = mutableListOf<String>()
            for (annotation in showAnnotations) {
                args.add("--show-annotation")
                args.add(annotation)
            }
            if (includeSystemApiAnnotations && !args.contains("android.annotation.SystemApi")) {
                args.add("--show-annotation")
                args.add("android.annotation.SystemApi")
            }
            if (includeSystemApiAnnotations && !args.contains("android.annotation.SystemService")) {
                args.add("--show-annotation")
                args.add("android.annotation.SystemService")
            }
            args.toTypedArray()
        } else {
            emptyArray()
        }

        val showUnannotatedArgs =
            if (showUnannotated) {
                arrayOf("--show-unannotated")
            } else {
                emptyArray<String>()
            }

        var removedApiFile: File? = null
        val removedArgs = if (removedApi != null) {
            removedApiFile = temporaryFolder.newFile("removed.txt")
            arrayOf("--removed-api", removedApiFile.path)
        } else {
            emptyArray()
        }

        var removedDexApiFile: File? = null
        val removedDexArgs = if (removedDexApi != null) {
            removedDexApiFile = temporaryFolder.newFile("removed-dex.txt")
            arrayOf("--removed-dex-api", removedDexApiFile.path)
        } else {
            emptyArray()
        }

        var apiFile: File? = null
        val apiArgs = if (api != null) {
            apiFile = temporaryFolder.newFile("public-api.txt")
            arrayOf("--api", apiFile.path)
        } else {
            emptyArray()
        }

        var exactApiFile: File? = null
        val exactApiArgs = if (exactApi != null) {
            exactApiFile = temporaryFolder.newFile("exact-api.txt")
            arrayOf("--exact-api", exactApiFile.path)
        } else {
            emptyArray()
        }

        var privateApiFile: File? = null
        val privateApiArgs = if (privateApi != null) {
            privateApiFile = temporaryFolder.newFile("private.txt")
            arrayOf("--private-api", privateApiFile.path)
        } else {
            emptyArray()
        }

        var dexApiFile: File? = null
        val dexApiArgs = if (dexApi != null) {
            dexApiFile = temporaryFolder.newFile("public-dex.txt")
            arrayOf("--dex-api", dexApiFile.path)
        } else {
            emptyArray()
        }

        var privateDexApiFile: File? = null
        val privateDexApiArgs = if (privateDexApi != null) {
            privateDexApiFile = temporaryFolder.newFile("private-dex.txt")
            arrayOf("--private-dex-api", privateDexApiFile.path)
        } else {
            emptyArray()
        }

        var stubsDir: File? = null
        val stubsArgs = if (stubs.isNotEmpty()) {
            stubsDir = temporaryFolder.newFolder("stubs")
            if (docStubs) {
                arrayOf("--doc-stubs", stubsDir.path)
            } else {
                arrayOf("--stubs", stubsDir.path)
            }
        } else {
            emptyArray()
        }

        var stubsSourceListFile: File? = null
        val stubsSourceListArgs = if (stubsSourceList != null) {
            stubsSourceListFile = temporaryFolder.newFile("droiddoc-src-list")
            arrayOf("--write-stubs-source-list", stubsSourceListFile.path)
        } else {
            emptyArray()
        }

        val applyApiLevelsXmlFile: File?
        val applyApiLevelsXmlArgs = if (applyApiLevelsXml != null) {
            ApiLookup::class.java.getDeclaredMethod("dispose").apply { isAccessible = true }.invoke(null)
            applyApiLevelsXmlFile = temporaryFolder.newFile("api-versions.xml")
            Files.asCharSink(applyApiLevelsXmlFile!!, Charsets.UTF_8).write(applyApiLevelsXml.trimIndent())
            arrayOf("--apply-api-levels", applyApiLevelsXmlFile.path)
        } else {
            emptyArray()
        }

        val importedPackageArgs = mutableListOf<String>()
        importedPackages.forEach {
            importedPackageArgs.add("--stub-import-packages")
            importedPackageArgs.add(it)
        }

        val skipEmitPackagesArgs = mutableListOf<String>()
        skipEmitPackages.forEach {
            skipEmitPackagesArgs.add("--skip-emit-packages")
            skipEmitPackagesArgs.add(it)
        }

        val kotlinPath = findKotlinStdlibPath()
        val kotlinPathArgs =
            if (kotlinPath.isNotEmpty() &&
                sourceList.asSequence().any { it.endsWith(DOT_KT) }
            ) {
                arrayOf("--classpath", kotlinPath.joinToString(separator = File.pathSeparator) { it })
            } else {
                emptyArray()
            }

        val sdkFilesDir: File?
        val sdkFilesArgs: Array<String>
        if (sdk_broadcast_actions != null ||
            sdk_activity_actions != null ||
            sdk_service_actions != null ||
            sdk_categories != null ||
            sdk_features != null ||
            sdk_widgets != null
        ) {
            val dir = File(project, "sdk-files")
            sdkFilesArgs = arrayOf("--sdk-values", dir.path)
            sdkFilesDir = dir
        } else {
            sdkFilesArgs = emptyArray()
            sdkFilesDir = null
        }

        val artifactArgs = if (artifacts != null) {
            val args = mutableListOf<String>()
            var index = 1
            for ((artifactId, signatures) in artifacts) {
                val signatureFile = temporaryFolder.newFile("signature-file-$index.txt")
                Files.asCharSink(signatureFile, Charsets.UTF_8).write(signatures.trimIndent())
                index++

                args.add("--register-artifact")
                args.add(signatureFile.path)
                args.add(artifactId)
            }
            args.toTypedArray()
        } else {
            emptyArray()
        }

        val extractedAnnotationsZip: File?
        val extractAnnotationsArgs = if (extractAnnotations != null) {
            extractedAnnotationsZip = temporaryFolder.newFile("extracted-annotations.zip")
            arrayOf("--extract-annotations", extractedAnnotationsZip.path)
        } else {
            extractedAnnotationsZip = null
            emptyArray()
        }

        val actualOutput = runDriver(
            "--no-color",
            "--no-banner",

            // For the tests we want to treat references to APIs like java.io.Closeable
            // as a class that is part of the API surface, not as a hidden class as would
            // be the case when analyzing a complete API surface
            // "--unhide-classpath-classes",
            "--allow-referencing-unknown-classes",

            // Annotation generation temporarily turned off by default while integrating with
            // SDK builds; tests need these
            "--include-annotations",

            "--sourcepath",
            sourcePath,
            "--classpath",
            androidJar.path,
            *kotlinPathArgs,
            *removedArgs,
            *removedDexArgs,
            *apiArgs,
            *exactApiArgs,
            *privateApiArgs,
            *dexApiArgs,
            *privateDexApiArgs,
            *stubsArgs,
            *stubsSourceListArgs,
            "--compatible-output=${if (compatibilityMode) "yes" else "no"}",
            "--output-kotlin-nulls=${if (outputKotlinStyleNulls) "yes" else "no"}",
            "--input-kotlin-nulls=${if (inputKotlinStyleNulls) "yes" else "no"}",
            "--omit-common-packages=${if (omitCommonPackages) "yes" else "no"}",
            *coverageStats,
            *quiet,
            *mergeAnnotationsArgs,
            *jaifAnnotationsArgs,
            *signatureAnnotationsArgs,
            *previousApiArgs,
            *migrateNullsArguments,
            *checkCompatibilityArguments,
            *proguardKeepArguments,
            *manifestFileArgs,
            *applyApiLevelsXmlArgs,
            *showAnnotationArguments,
            *showUnannotatedArgs,
            *sdkFilesArgs,
            *importedPackageArgs.toTypedArray(),
            *skipEmitPackagesArgs.toTypedArray(),
            *artifactArgs,
            *extractAnnotationsArgs,
            *sourceList,
            *extraArguments,
            expectedFail = expectedFail
        )

        if (expectedOutput != null) {
            assertEquals(expectedOutput.trimIndent().trim(), actualOutput.trim())
        }

        if (api != null && apiFile != null) {
            assertTrue("${apiFile.path} does not exist even though --api was used", apiFile.exists())
            val expectedText = readFile(apiFile, stripBlankLines, trim)
            assertEquals(stripComments(api, stripLineComments = false).trimIndent(), expectedText)
        }

        if (removedApi != null && removedApiFile != null) {
            assertTrue(
                "${removedApiFile.path} does not exist even though --removed-api was used",
                removedApiFile.exists()
            )
            val expectedText = readFile(removedApiFile, stripBlankLines, trim)
            assertEquals(stripComments(removedApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (removedDexApi != null && removedDexApiFile != null) {
            assertTrue(
                "${removedDexApiFile.path} does not exist even though --removed-dex-api was used",
                removedDexApiFile.exists()
            )
            val expectedText = readFile(removedDexApiFile, stripBlankLines, trim)
            assertEquals(stripComments(removedDexApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (exactApi != null && exactApiFile != null) {
            assertTrue(
                "${exactApiFile.path} does not exist even though --exact-api was used",
                exactApiFile.exists()
            )
            val expectedText = readFile(exactApiFile, stripBlankLines, trim)
            assertEquals(stripComments(exactApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (privateApi != null && privateApiFile != null) {
            assertTrue(
                "${privateApiFile.path} does not exist even though --private-api was used",
                privateApiFile.exists()
            )
            val expectedText = readFile(privateApiFile, stripBlankLines, trim)
            assertEquals(stripComments(privateApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (dexApi != null && dexApiFile != null) {
            assertTrue(
                "${dexApiFile.path} does not exist even though --dex-api was used",
                dexApiFile.exists()
            )
            val expectedText = readFile(dexApiFile, stripBlankLines, trim)
            assertEquals(stripComments(dexApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (privateDexApi != null && privateDexApiFile != null) {
            assertTrue(
                "${privateDexApiFile.path} does not exist even though --private-dex-api was used",
                privateDexApiFile.exists()
            )
            val expectedText = readFile(privateDexApiFile, stripBlankLines, trim)
            assertEquals(stripComments(privateDexApi, stripLineComments = false).trimIndent(), expectedText)
        }

        if (proguard != null && proguardFile != null) {
            val expectedProguard = readFile(proguardFile)
            assertTrue(
                "${proguardFile.path} does not exist even though --proguard was used",
                proguardFile.exists()
            )
            assertEquals(stripComments(proguard, stripLineComments = false).trimIndent(), expectedProguard.trim())
        }

        if (sdk_broadcast_actions != null) {
            val actual = readFile(File(sdkFilesDir, "broadcast_actions.txt"), stripBlankLines, trim)
            assertEquals(sdk_broadcast_actions.trimIndent().trim(), actual.trim())
        }

        if (sdk_activity_actions != null) {
            val actual = readFile(File(sdkFilesDir, "activity_actions.txt"), stripBlankLines, trim)
            assertEquals(sdk_activity_actions.trimIndent().trim(), actual.trim())
        }

        if (sdk_service_actions != null) {
            val actual = readFile(File(sdkFilesDir, "service_actions.txt"), stripBlankLines, trim)
            assertEquals(sdk_service_actions.trimIndent().trim(), actual.trim())
        }

        if (sdk_categories != null) {
            val actual = readFile(File(sdkFilesDir, "categories.txt"), stripBlankLines, trim)
            assertEquals(sdk_categories.trimIndent().trim(), actual.trim())
        }

        if (sdk_features != null) {
            val actual = readFile(File(sdkFilesDir, "features.txt"), stripBlankLines, trim)
            assertEquals(sdk_features.trimIndent().trim(), actual.trim())
        }

        if (sdk_widgets != null) {
            val actual = readFile(File(sdkFilesDir, "widgets.txt"), stripBlankLines, trim)
            assertEquals(sdk_widgets.trimIndent().trim(), actual.trim())
        }

        if (warnings != null) {
            assertEquals(
                warnings.trimIndent().trim(),
                reportedWarnings.toString().replace(project.path, "TESTROOT").replace(
                    project.canonicalPath,
                    "TESTROOT"
                ).trim()
            )
        }

        if (extractAnnotations != null && extractedAnnotationsZip != null) {
            assertTrue(
                "Using --extract-annotations but $extractedAnnotationsZip was not created",
                extractedAnnotationsZip.isFile
            )
            for ((pkg, xml) in extractAnnotations) {
                assertPackageXml(pkg, extractedAnnotationsZip, xml)
            }
        }

        if (stubs.isNotEmpty() && stubsDir != null) {
            for (i in 0 until stubs.size) {
                val stub = stubs[i]
                val sourceFile = sourceFiles[i]
                val targetPath = if (sourceFile.targetPath.endsWith(DOT_KT)) {
                    // Kotlin source stubs are rewritten as .java files for now
                    sourceFile.targetPath.substring(0, sourceFile.targetPath.length - 3) + DOT_JAVA
                } else {
                    sourceFile.targetPath
                }
                val stubFile = File(stubsDir, targetPath.substring("src/".length))
                val expectedText = readFile(stubFile, stripBlankLines, trim)
                assertEquals(stub.trimIndent(), expectedText)
            }
        }

        if (stubsSourceList != null && stubsSourceListFile != null) {
            assertTrue(
                "${stubsSourceListFile.path} does not exist even though --write-stubs-source-list was used",
                stubsSourceListFile.exists()
            )
            val expectedText = readFile(stubsSourceListFile, stripBlankLines, trim)
            assertEquals(stripComments(stubsSourceList, stripLineComments = false).trimIndent(), expectedText)
        }

        if (checkCompilation && stubsDir != null && CHECK_STUB_COMPILATION) {
            val generated = gatherSources(listOf(stubsDir)).map { it.path }.toList().toTypedArray()

            // Also need to include on the compile path annotation classes referenced in the stubs
            val extraAnnotationsDir = File("stub-annotations/src/main/java")
            if (!extraAnnotationsDir.isDirectory) {
                fail("Couldn't find $extraAnnotationsDir: Is the pwd set to the root of the metalava source code?")
                fail("Couldn't find $extraAnnotationsDir: Is the pwd set to the root of an Android source tree?")
            }
            val extraAnnotations = gatherSources(listOf(extraAnnotationsDir)).map { it.path }.toList().toTypedArray()

            if (!runCommand(
                    "${getJdkPath()}/bin/javac", arrayOf(
                        "-d", project.path, *generated, *extraAnnotations
                    )
                )
            ) {
                fail("Couldn't compile stub file -- compilation problems")
                return
            }
        }

        if (checkDoclava1 && !CHECK_OLD_DOCLAVA_TOO) {
            println(
                "This test requested diffing with doclava1, but doclava1 testing was disabled with the " +
                    "DriverTest#CHECK_OLD_DOCLAVA_TOO = false"
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            api != null && apiFile != null
        ) {
            apiFile.delete()
            checkSignaturesWithDoclava1(
                api = api,
                argument = "-api",
                output = apiFile,
                expected = apiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            exactApi != null && exactApiFile != null
        ) {
            exactApiFile.delete()
            checkSignaturesWithDoclava1(
                api = exactApi,
                argument = "-exactApi",
                output = exactApiFile,
                expected = exactApiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            removedApi != null && removedApiFile != null
        ) {
            removedApiFile.delete()
            checkSignaturesWithDoclava1(
                api = removedApi,
                argument = "-removedApi",
                output = removedApiFile,
                expected = removedApiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null && stubsDir != null) {
            stubsDir.deleteRecursively()
            val firstFile = File(stubsDir, sourceFiles[0].targetPath.substring("src/".length))
            checkSignaturesWithDoclava1(
                api = stubs[0],
                argument = "-stubs",
                output = stubsDir,
                expected = firstFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && proguard != null && proguardFile != null) {
            proguardFile.delete()
            checkSignaturesWithDoclava1(
                api = proguard,
                argument = "-proguard",
                output = proguardFile,
                expected = proguardFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            privateApi != null && privateApiFile != null
        ) {
            privateApiFile.delete()
            checkSignaturesWithDoclava1(
                api = privateApi,
                argument = "-privateApi",
                output = privateApiFile,
                expected = privateApiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                // Workaround: -privateApi is a no-op if you don't also provide -api
                extraArguments = arrayOf("-api", File(privateApiFile.parentFile, "dummy-api.txt").path),
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            privateDexApi != null && privateDexApiFile != null
        ) {
            privateDexApiFile.delete()
            checkSignaturesWithDoclava1(
                api = privateDexApi,
                argument = "-privateDexApi",
                output = privateDexApiFile,
                expected = privateDexApiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                // Workaround: -privateDexApi is a no-op if you don't also provide -api
                extraArguments = arrayOf("-api", File(privateDexApiFile.parentFile, "dummy-api.txt").path),
                showUnannotated = showUnannotated
            )
        }

        if (CHECK_OLD_DOCLAVA_TOO && checkDoclava1 && signatureSource == null &&
            dexApi != null && dexApiFile != null
        ) {
            dexApiFile.delete()
            checkSignaturesWithDoclava1(
                api = dexApi,
                argument = "-dexApi",
                output = dexApiFile,
                expected = dexApiFile,
                sourceList = sourceList,
                sourcePath = sourcePath,
                packages = packages,
                androidJar = androidJar,
                trim = trim,
                stripBlankLines = stripBlankLines,
                showAnnotationArgs = showAnnotationArguments,
                stubImportPackages = importedPackages,
                // Workaround: -dexApi is a no-op if you don't also provide -api
                extraArguments = arrayOf("-api", File(dexApiFile.parentFile, "dummy-api.txt").path),
                showUnannotated = showUnannotated
            )
        }
    }

    /** Checks that the given zip annotations file contains the given XML package contents */
    private fun assertPackageXml(pkg: String, output: File, @Language("XML") expected: String) {
        assertNotNull(output)
        assertTrue(output.exists())
        val url = URL(
            "jar:" + SdkUtils.fileToUrlString(output) + "!/" + pkg.replace('.', '/') +
                "/annotations.xml"
        )
        val stream = url.openStream()
        try {
            val bytes = ByteStreams.toByteArray(stream)
            assertNotNull(bytes)
            val xml = String(bytes, Charsets.UTF_8).replace("\r\n", "\n")
            assertEquals(expected.trimIndent().trim(), xml.trimIndent().trim())
        } finally {
            Closeables.closeQuietly(stream)
        }
    }

    private fun checkSignaturesWithDoclava1(
        api: String,
        argument: String,
        output: File,
        expected: File = output,
        sourceList: Array<String>,
        sourcePath: String,
        packages: Set<String>,
        androidJar: File,
        trim: Boolean = true,
        stripBlankLines: Boolean = true,
        showAnnotationArgs: Array<String> = emptyArray(),
        stubImportPackages: List<String>,
        extraArguments: Array<String> = emptyArray(),
        showUnannotated: Boolean
    ) {
        // We have to run Doclava out of process because running it in process
        // (with Doclava1 jars on the test classpath) only works once; it leaves
        // around state such that the second test fails. Instead we invoke it
        // separately on each test; slower but reliable.

        val doclavaArg = when (argument) {
            "--api" -> "-api"
            "--removed-api" -> "-removedApi"
            else -> if (argument.startsWith("--")) argument.substring(1) else argument
        }

        val showAnnotationArgsDoclava1: Array<String> = if (showAnnotationArgs.isNotEmpty()) {
            showAnnotationArgs.map { if (it == "--show-annotation") "-showAnnotation" else it }.toTypedArray()
        } else {
            emptyArray()
        }
        val showUnannotatedArgs = if (showUnannotated) {
            arrayOf("-showUnannotated")
        } else {
            emptyArray()
        }

        val docLava1 = File("testlibs/doclava-1.0.6-full-SNAPSHOT.jar")
        if (!docLava1.isFile) {
            /*
                Not checked in (it's 22MB).
                To generate the doclava1 jar, add this to external/doclava/build.gradle and run ./gradlew shadowJar:

                // shadow jar: Includes all dependencies
                buildscript {
                    repositories {
                        jcenter()
                    }
                    dependencies {
                        classpath 'com.github.jengelman.gradle.plugins:shadow:2.0.2'
                    }
                }
                apply plugin: 'com.github.johnrengelman.shadow'
                shadowJar {
                   baseName = "doclava-$version-full-SNAPSHOT"
                   classifier = null
                   version = null
                }
             */
            fail("Couldn't find $docLava1: Is the pwd set to the root of the metalava source code?")
        }

        val jdkPath = getJdkPath()
        if (jdkPath == null) {
            fail("JDK not found in the environment; make sure \$JAVA_HOME is set.")
        }

        val hidePackageArgs = mutableListOf<String>()
        options.hidePackages.forEach {
            hidePackageArgs.add("-hidePackage")
            hidePackageArgs.add(it)
        }

        val stubImports = if (stubImportPackages.isNotEmpty()) {
            arrayOf("-stubimportpackages", stubImportPackages.joinToString(separator = ":") { it })
        } else {
            emptyArray()
        }

        val args = arrayOf(
            *sourceList,
            "-stubpackages",
            packages.joinToString(separator = ":") { it },
            *stubImports,
            "-doclet",
            "com.google.doclava.Doclava",
            "-docletpath",
            docLava1.path,
            "-encoding",
            "UTF-8",
            "-source",
            "1.8",
            "-nodocs",
            "-quiet",
            "-sourcepath",
            sourcePath,
            "-classpath",
            androidJar.path,

            *showAnnotationArgsDoclava1,
            *showUnannotatedArgs,
            *hidePackageArgs.toTypedArray(),
            *extraArguments,

            // -api, or // -stub, etc
            doclavaArg,
            output.path
        )

        val message = "\n${args.joinToString(separator = "\n") { "\"$it\"," }}"
        println("Running doclava1 with the following args:\n$message")

        if (!runCommand(
                "$jdkPath/bin/java",
                arrayOf(
                    "-classpath",
                    "${docLava1.path}:$jdkPath/lib/tools.jar",
                    "com.google.doclava.Doclava",
                    *args
                )
            )
        ) {
            return
        }

        val expectedText = readFile(expected, stripBlankLines, trim)
        assertEquals(stripComments(api, stripLineComments = false).trimIndent(), expectedText)
    }

    private fun runCommand(executable: String, args: Array<String>): Boolean {
        try {
            val logger = StdLogger(StdLogger.Level.ERROR)
            val processExecutor = DefaultProcessExecutor(logger)
            val processInfo = ProcessInfoBuilder()
                .setExecutable(executable)
                .addArgs(args)
                .createProcess()

            val processOutputHandler = LoggedProcessOutputHandler(logger)
            val result = processExecutor.execute(processInfo, processOutputHandler)

            result.rethrowFailure().assertNormalExitValue()
        } catch (e: ProcessException) {
            fail("Failed to run $executable (${e.message}): not verifying this API on the old doclava engine")
            return false
        }
        return true
    }

    companion object {
        const val API_LEVEL = 27

        private val latestAndroidPlatform: String
            get() = "android-$API_LEVEL"

        private val sdk: File
            get() = File(
                System.getenv("ANDROID_HOME")
                    ?: error("You must set \$ANDROID_HOME before running tests")
            )

        fun getAndroidJar(apiLevel: Int): File? {
            val localFile = File("../../prebuilts/sdk/$apiLevel/public/android.jar")
            if (localFile.exists()) {
                return localFile
            }

            return null
        }

        fun getPlatformFile(path: String): File {
            return getAndroidJar(API_LEVEL) ?: run {
                val file = FileUtils.join(sdk, SdkConstants.FD_PLATFORMS, latestAndroidPlatform, path)
                if (!file.exists()) {
                    throw IllegalArgumentException(
                        "File \"$path\" not found in platform $latestAndroidPlatform"
                    )
                }
                file
            }
        }

        fun java(@Language("JAVA") source: String): LintDetectorTest.TestFile {
            return TestFiles.java(source.trimIndent())
        }

        fun kotlin(@Language("kotlin") source: String): LintDetectorTest.TestFile {
            return TestFiles.kotlin(source.trimIndent())
        }

        fun kotlin(to: String, @Language("kotlin") source: String): LintDetectorTest.TestFile {
            return TestFiles.kotlin(to, source.trimIndent())
        }

        private fun readFile(file: File, stripBlankLines: Boolean = false, trim: Boolean = false): String {
            var apiLines: List<String> = Files.asCharSource(file, Charsets.UTF_8).readLines()
            if (stripBlankLines) {
                apiLines = apiLines.asSequence().filter { it.isNotBlank() }.toList()
            }
            var apiText = apiLines.joinToString(separator = "\n") { it }
            if (trim) {
                apiText = apiText.trim()
            }
            return apiText
        }
    }
}

val intRangeAnnotationSource: TestFile = java(
    """
        package android.annotation;
        import java.lang.annotation.*;
        import static java.lang.annotation.ElementType.*;
        import static java.lang.annotation.RetentionPolicy.SOURCE;
        @Retention(SOURCE)
        @Target({METHOD,PARAMETER,FIELD,LOCAL_VARIABLE,ANNOTATION_TYPE})
        public @interface IntRange {
            long from() default Long.MIN_VALUE;
            long to() default Long.MAX_VALUE;
        }
        """
).indented()

val intDefAnnotationSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.Retention;
    import java.lang.annotation.RetentionPolicy;
    import java.lang.annotation.Target;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @Retention(SOURCE)
    @Target({ANNOTATION_TYPE})
    public @interface IntDef {
        int[] value() default {};
        boolean flag() default false;
    }
    """
).indented()

val longDefAnnotationSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.Retention;
    import java.lang.annotation.RetentionPolicy;
    import java.lang.annotation.Target;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @Retention(SOURCE)
    @Target({ANNOTATION_TYPE})
    public @interface LongDef {
        long[] value() default {};
        boolean flag() default false;
    }
    """
).indented()

val nonNullSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.Retention;
    import java.lang.annotation.Target;

    import static java.lang.annotation.ElementType.FIELD;
    import static java.lang.annotation.ElementType.METHOD;
    import static java.lang.annotation.ElementType.PARAMETER;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    /**
     * Denotes that a parameter, field or method return value can never be null.
     * @paramDoc This value must never be {@code null}.
     * @returnDoc This value will never be {@code null}.
     * @hide
     */
    @SuppressWarnings({"WeakerAccess", "JavaDoc"})
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD, TYPE_USE})
    public @interface NonNull {
    }
    """
).indented()

val libcoreNonNullSource: TestFile = DriverTest.java(
    """
    package libcore.util;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    import java.lang.annotation.*;
    @Documented
    @Retention(SOURCE)
    @Target({TYPE_USE})
    public @interface NonNull {
       int from() default Integer.MIN_VALUE;
       int to() default Integer.MAX_VALUE;
    }
    """
).indented()

val libcoreNullableSource: TestFile = DriverTest.java(
    """
    package libcore.util;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    import java.lang.annotation.*;
    @Documented
    @Retention(SOURCE)
    @Target({TYPE_USE})
    public @interface Nullable {
       int from() default Integer.MIN_VALUE;
       int to() default Integer.MAX_VALUE;
    }
    """
).indented()
val requiresPermissionSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @Retention(SOURCE)
    @Target({ANNOTATION_TYPE,METHOD,CONSTRUCTOR,FIELD,PARAMETER})
    public @interface RequiresPermission {
        String value() default "";
        String[] allOf() default {};
        String[] anyOf() default {};
        boolean conditional() default false;
    }
                    """
).indented()

val requiresFeatureSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @Retention(SOURCE)
    @Target({TYPE,FIELD,METHOD,CONSTRUCTOR})
    public @interface RequiresFeature {
        String value();
    }
            """
).indented()

val requiresApiSource: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @Retention(SOURCE)
    @Target({TYPE,FIELD,METHOD,CONSTRUCTOR})
    public @interface RequiresApi {
        int value() default 1;
        int api() default 1;
    }
            """
).indented()

val sdkConstantSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.*;
    @Target({ ElementType.FIELD })
    @Retention(RetentionPolicy.SOURCE)
    public @interface SdkConstant {
        enum SdkConstantType {
            ACTIVITY_INTENT_ACTION, BROADCAST_INTENT_ACTION, SERVICE_ACTION, INTENT_CATEGORY, FEATURE
        }
        SdkConstantType value();
    }
        """
).indented()

val broadcastBehaviorSource: TestFile = java(
    """
        package android.annotation;
        import java.lang.annotation.*;
        /** @hide */
        @Target({ ElementType.FIELD })
        @Retention(RetentionPolicy.SOURCE)
        public @interface BroadcastBehavior {
            boolean explicitOnly() default false;
            boolean registeredOnly() default false;
            boolean includeBackground() default false;
            boolean protectedBroadcast() default false;
        }
        """
).indented()

val nullableSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    /**
     * Denotes that a parameter, field or method return value can be null.
     * @paramDoc This value may be {@code null}.
     * @returnDoc This value may be {@code null}.
     * @hide
     */
    @SuppressWarnings({"WeakerAccess", "JavaDoc"})
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD, TYPE_USE})
    public @interface Nullable {
    }
    """
).indented()

val supportNonNullSource: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @SuppressWarnings("WeakerAccess")
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD, TYPE_USE})
    public @interface NonNull {
    }
    """
).indented()

val supportNullableSource: TestFile = java(
    """
package androidx.annotation;
import java.lang.annotation.*;
import static java.lang.annotation.ElementType.*;
import static java.lang.annotation.RetentionPolicy.SOURCE;
@SuppressWarnings("WeakerAccess")
@Retention(SOURCE)
@Target({METHOD, PARAMETER, FIELD, TYPE_USE})
public @interface Nullable {
}
                """
)

val androidxNonNullSource: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @SuppressWarnings("WeakerAccess")
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD, TYPE_USE})
    public @interface NonNull {
    }
    """
).indented()

val androidxNullableSource: TestFile = java(
    """
package androidx.annotation;
import java.lang.annotation.*;
import static java.lang.annotation.ElementType.*;
import static java.lang.annotation.RetentionPolicy.SOURCE;
@SuppressWarnings("WeakerAccess")
@Retention(SOURCE)
@Target({METHOD, PARAMETER, FIELD, TYPE_USE})
public @interface Nullable {
}
                """
)

val supportParameterName: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @SuppressWarnings("WeakerAccess")
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD})
    public @interface ParameterName {
        String value();
    }
    """
).indented()

val supportDefaultValue: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    @SuppressWarnings("WeakerAccess")
    @Retention(SOURCE)
    @Target({METHOD, PARAMETER, FIELD})
    public @interface DefaultValue {
        String value();
    }
    """
).indented()

val uiThreadSource: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    /**
     * Denotes that the annotated method or constructor should only be called on the
     * UI thread. If the annotated element is a class, then all methods in the class
     * should be called on the UI thread.
     * @memberDoc This method must be called on the thread that originally created
     *            this UI element. This is typically the main thread of your app.
     * @classDoc Methods in this class must be called on the thread that originally created
     *            this UI element, unless otherwise noted. This is typically the
     *            main thread of your app. * @hide
     */
    @SuppressWarnings({"WeakerAccess", "JavaDoc"})
    @Retention(SOURCE)
    @Target({METHOD,CONSTRUCTOR,TYPE,PARAMETER})
    public @interface UiThread {
    }
    """
).indented()

val workerThreadSource: TestFile = java(
    """
    package androidx.annotation;
    import java.lang.annotation.*;
    import static java.lang.annotation.ElementType.*;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    /**
     * @memberDoc This method may take several seconds to complete, so it should
     *            only be called from a worker thread.
     * @classDoc Methods in this class may take several seconds to complete, so it should
     *            only be called from a worker thread unless otherwise noted.
     * @hide
     */
    @SuppressWarnings({"WeakerAccess", "JavaDoc"})
    @Retention(SOURCE)
    @Target({METHOD,CONSTRUCTOR,TYPE,PARAMETER})
    public @interface WorkerThread {
    }
    """
).indented()

val suppressLintSource: TestFile = java(
    """
package android.annotation;

import static java.lang.annotation.ElementType.*;
import java.lang.annotation.*;
@Target({TYPE, FIELD, METHOD, PARAMETER, CONSTRUCTOR, LOCAL_VARIABLE})
@Retention(RetentionPolicy.CLASS)
public @interface SuppressLint {
    String[] value();
}
                """
)

val systemServiceSource: TestFile = java(
    """
    package android.annotation;
    import static java.lang.annotation.ElementType.TYPE;
    import static java.lang.annotation.RetentionPolicy.SOURCE;
    import java.lang.annotation.*;
    @Retention(SOURCE)
    @Target(TYPE)
    public @interface SystemService {
        String value();
    }
    """
).indented()

val systemApiSource: TestFile = java(
    """
    package android.annotation;
    import static java.lang.annotation.ElementType.*;
    import java.lang.annotation.*;
    @Target({TYPE, FIELD, METHOD, CONSTRUCTOR, ANNOTATION_TYPE, PACKAGE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface SystemApi {
    }
    """
).indented()

val testApiSource: TestFile = java(
    """
    package android.annotation;
    import static java.lang.annotation.ElementType.*;
    import java.lang.annotation.*;
    @Target({TYPE, FIELD, METHOD, CONSTRUCTOR, ANNOTATION_TYPE, PACKAGE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface TestApi {
    }
    """
).indented()

val widgetSource: TestFile = java(
    """
    package android.annotation;
    import java.lang.annotation.*;
    @Target({ ElementType.TYPE })
    @Retention(RetentionPolicy.SOURCE)
    public @interface Widget {
    }
    """
).indented()
