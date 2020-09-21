/*
 * Copyright 2016 The Android Open Source Project
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

package org.chromium.latency.walt;

public interface RobotAutomationListener {
    public static final String START_EVENT = "START_EVENT";
    public static final String RESTART_EVENT = "RESTART_EVENT";
    public static final String FINISH_EVENT = "FINISH_EVENT";
    public static final String WRITE_LOG_EVENT = "WRITE_LOG_EVENT";
    public static final String CLEAR_LOG_EVENT = "CLEAR_LOG_EVENT";

    public void onRobotAutomationEvent(String event);
}
