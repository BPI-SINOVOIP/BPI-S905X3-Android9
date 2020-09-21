/**
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.os;

import android.os.IIncidentReportStatusListener;
import android.os.IncidentReportArgs;

/**
  * Binder interface to report system health incidents.
  * {@hide}
  */
oneway interface IIncidentManager {

    /**
     * Takes a report with the given args, reporting status to the optional listener.
     *
     * When the report is completed, the system report listener will be notified.
     */
    void reportIncident(in IncidentReportArgs args);

    /**
     * Takes a report with the given args, reporting status to the optional listener.
     *
     * When the report is completed, the system report listener will be notified.
     */
    void reportIncidentToStream(in IncidentReportArgs args,
            @nullable IIncidentReportStatusListener listener,
            FileDescriptor stream);

    /**
     * Tell the incident daemon that the android system server is up and running.
     */
    void systemRunning();
}
