/*
 * Copyright 2017 The Android Open Source Project
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

package androidx.build.dependencies

const val AUTO_COMMON = "com.google.auto:auto-common:0.6"
const val ANTLR = "org.antlr:antlr4:4.5.3"
const val APACHE_COMMONS_CODEC = "commons-codec:commons-codec:1.10"
const val CONSTRAINT_LAYOUT = "androidx.constraintlayout:constraintlayout:1.1.0@aar"
const val DEXMAKER_MOCKITO = "com.linkedin.dexmaker:dexmaker-mockito:2.16.1-SNAPSHOT"
const val ESPRESSO_CONTRIB = "com.android.support.test.espresso:espresso-contrib:3.0.1"
const val ESPRESSO_CORE = "com.android.support.test.espresso:espresso-core:3.0.1"
const val FIREBASE_JOBDISPATCHER = "com.firebase:firebase-jobdispatcher:0.8.5"
const val GOOGLE_COMPILE_TESTING = "com.google.testing.compile:compile-testing:0.11"
const val GSON = "com.google.code.gson:gson:2.8.0"
const val GUAVA = "com.google.guava:guava:21.0"
const val GUAVA_ANDROID = "com.google.guava:guava:23.6-android"
const val INTELLIJ_ANNOTATIONS = "com.intellij:annotations:12.0"
const val JAVAPOET = "com.squareup:javapoet:1.8.0"
const val JSR250 = "javax.annotation:javax.annotation-api:1.2"
const val JUNIT = "junit:junit:4.12"
const val KOTLIN_STDLIB = "org.jetbrains.kotlin:kotlin-stdlib:1.2.41"
const val KOTLIN_METADATA = "me.eugeniomarletti.kotlin.metadata:kotlin-metadata:1.4.0"
const val LINT = "com.android.tools.lint:lint:26.2.0-alpha16"
const val MOCKITO_CORE = "org.mockito:mockito-core:2.16.0"
const val MULTIDEX = "androidx.multidex:multidex:2.0.0"
const val NULLAWAY = "com.uber.nullaway:nullaway:0.3.7"
const val PLAY_SERVICES = "com.google.android.gms:play-services-base:11.6.0"
const val REACTIVE_STREAMS = "org.reactivestreams:reactive-streams:1.0.0"
const val RX_JAVA = "io.reactivex.rxjava2:rxjava:2.0.6"
const val TEST_RUNNER = "com.android.support.test:runner:1.0.1"
const val TEST_RULES = "com.android.support.test:rules:1.0.1"
const val TRUTH = "com.google.truth:truth:0.34"
/**
 * this Xerial version is newer than we want but we need it to fix
 * https://github.com/xerial/sqlite-jdbc/issues/97
 * https://github.com/xerial/sqlite-jdbc/issues/267
 */
const val XERIAL = "org.xerial:sqlite-jdbc:3.20.1"
const val XPP3 = "xpp3:xpp3:1.1.4c"
const val XMLPULL = "xmlpull:xmlpull:1.1.3.1"

const val ESPRESSO_CONTRIB_TMP = "com.android.temp.support.test.espresso:espresso-contrib:3.0.1"
const val ESPRESSO_CORE_TMP = "com.android.temp.support.test.espresso:espresso-core:3.0.1"
const val TEST_RUNNER_TMP = "com.android.temp.support.test:runner:1.0.1"
const val TEST_RULES_TMP = "com.android.temp.support.test:rules:1.0.1"

private const val NAV_SUPPORT_VERSION = "27.1.1"
const val NAV_SUPPORT_ANNOTATIONS = "com.android.support:support-annotations:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_APPCOMPAT = "com.android.support:appcompat-v7:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_CARDVIEW = "com.android.support:cardview-v7:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_COMPAT = "com.android.support:support-compat:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_CORE_UTILS = "com.android.support:support-core-utils:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_DESIGN = "com.android.support:design:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_FRAGMENTS = "com.android.support:support-fragment:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_RECYCLERVIEW = "com.android.support:recyclerview-v7:$NAV_SUPPORT_VERSION"
const val NAV_SUPPORT_V4 = "com.android.support:support-v4:$NAV_SUPPORT_VERSION"

private const val SUPPORT_VERSION = "1.0.0-alpha1"
const val SUPPORT_ANNOTATIONS = "androidx.annotation:annotation:$SUPPORT_VERSION"
const val SUPPORT_APPCOMPAT = "androidx.appcompat:appcompat:$SUPPORT_VERSION"
const val SUPPORT_CARDVIEW = "androidx.cardview:cardview:$SUPPORT_VERSION"
const val SUPPORT_CORE_UTILS = "androidx.legacy:legacy-support-core-utils:$SUPPORT_VERSION"
const val SUPPORT_DESIGN = "com.google.android.material:material:$SUPPORT_VERSION@aar"
const val SUPPORT_FRAGMENTS = "androidx.fragment:fragment:$SUPPORT_VERSION"
const val SUPPORT_RECYCLERVIEW = "androidx.recyclerview:recyclerview:$SUPPORT_VERSION"
const val SUPPORT_V4 = "androidx.legacy:legacy-support-v4:$SUPPORT_VERSION"

// Arch libraries
const val ARCH_LIFECYCLE_RUNTIME = "androidx.lifecycle:lifecycle-runtime:2.0.0-alpha1@aar"
const val ARCH_LIFECYCLE_LIVEDATA_CORE =
        "androidx.lifecycle:lifecycle-livedata-core:2.0.0-alpha1@aar"
const val ARCH_LIFECYCLE_VIEWMODEL = "androidx.lifecycle:lifecycle-viewmodel:2.0.0-alpha1@aar"
const val ARCH_LIFECYCLE_EXTENSIONS = "androidx.lifecycle:lifecycle-extensions:2.0.0-alpha1@aar"
