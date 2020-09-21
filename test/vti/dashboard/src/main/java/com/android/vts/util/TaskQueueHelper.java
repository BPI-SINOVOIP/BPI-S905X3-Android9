/**
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.google.appengine.api.taskqueue.Queue;
import com.google.appengine.api.taskqueue.TaskOptions;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** TaskQueueHelper, a helper class for interacting with App Engine Task Queue API. */
public class TaskQueueHelper {
    protected static final Logger logger = Logger.getLogger(TaskQueueHelper.class.getName());
    public static final int MAX_BATCH_ADD_SIZE = 100;

    /**
     * Add the list of tasks to the provided queue.
     *
     * @param queue The task queue in which to insert the tasks.
     * @param tasks The list of tasks to add.
     */
    public static void addToQueue(Queue queue, List<TaskOptions> tasks) {
        List<TaskOptions> puts = new ArrayList<>();
        for (TaskOptions task : tasks) {
            puts.add(task);
            if (puts.size() == MAX_BATCH_ADD_SIZE) {
                queue.addAsync(puts);
                puts = new ArrayList<>();
            } else if (puts.size() > MAX_BATCH_ADD_SIZE) {
                logger.log(Level.SEVERE, "Too many tasks batched in the task queue API.");
                return;
            }
        }
        if (puts.size() > 0) {
            queue.addAsync(puts);
        }
    }
}
