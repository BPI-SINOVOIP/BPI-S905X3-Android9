/*
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

package android.hardware.citadel;

interface ICitadeld {
    /**
     * Call into a Nugget app running on Citadel.
     *
     * @param app_id   The ID of the app to call.
     * @param arg      Argument to pass to the app.
     * @param request  Data to send to the app.
     * @param response Buffer to receive data from the app.
     * @return         Status code from the app.
     */
    int callApp(int appId, int arg, in byte[] request, out byte[] response);

    /** Reset Citadel by pulling the reset line. */
    boolean reset();
}
