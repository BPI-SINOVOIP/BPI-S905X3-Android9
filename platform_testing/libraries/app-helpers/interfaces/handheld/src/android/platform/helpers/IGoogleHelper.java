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

package android.platform.helpers;

public interface IGoogleHelper extends IAppHelper {

    /**
     * Setup expectations: Google app open
     *
     * This method will start a voice search
     *
     * @return true if the search is started, false otherwise
     */
    public boolean doVoiceSearch();

    /**
     * Setup expectations: Google app open to a search result
     *
     * This method will return the query from the search
     */
    public String getSearchQuery();

}
