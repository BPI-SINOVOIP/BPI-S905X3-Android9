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

import java.io.BufferedInputStream
import java.io.IOException
import java.io.UncheckedIOException
import java.util.Properties

/** Version strings */
object Version {
    val VERSION: String

    init {
        val properties = Properties()
        val stream = BufferedInputStream(Version::class.java.getResourceAsStream("/version.properties"))
        try {
            properties.load(stream)
            VERSION = properties.getProperty("metalavaVersion")
            stream.close()
        } catch (e: IOException) {
            throw UncheckedIOException(e)
        }
    }
}
