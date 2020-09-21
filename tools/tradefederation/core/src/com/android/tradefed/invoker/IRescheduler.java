/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.invoker;

import com.android.tradefed.command.ICommandOptions;
import com.android.tradefed.config.IConfiguration;

/**
 * Interface for rescheduling a config for future execution.
 */
public interface IRescheduler {

    /**
     * Schedules a config for future execution.
     *
     * @param config the {@link IConfiguration} to run
     *
     * @return <code>true</code> if config was successfully rescheduled. <code>false</code>
     * otherwise
     */
    boolean scheduleConfig(IConfiguration config);

    /**
     * Reschedule the command for future execution.
     *<p>
     * The command should respect {@link ICommandOptions#getLoopTime()} and schedule the command
     * with the appropriate delay.
     *</p>
     * @return <code>true</code> if command was successfully rescheduled. <code>false</code>
     * otherwise
     */
    boolean rescheduleCommand();

}
