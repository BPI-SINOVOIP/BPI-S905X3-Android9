/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

@file:JvmName("Constants")
@file:Suppress("PropertyName")

package com.example.android.jobscheduler

@JvmField val MSG_UNCOLOR_START = 0
@JvmField val MSG_UNCOLOR_STOP = 1
@JvmField val MSG_COLOR_START = 2
@JvmField val MSG_COLOR_STOP = 3

@JvmField val MESSENGER_INTENT_KEY = "${BuildConfig.APPLICATION_ID}.MESSENGER_INTENT_KEY"
@JvmField val WORK_DURATION_KEY = "${BuildConfig.APPLICATION_ID}.WORK_DURATION_KEY"
