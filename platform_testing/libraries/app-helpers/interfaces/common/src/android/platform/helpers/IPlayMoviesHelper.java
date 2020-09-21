/*
 * Copyright (C) 2016 The Android Open Source Project
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

public interface IPlayMoviesHelper extends IAppHelper {
    /**
     * Setup expectations: PlayMovies is open on any screen with access to the navigation bar.
     *
     * This method will navigate to "My Library" and select the "My Movies" tab. This will block
     * until the method is complete.
     */
    public void openMoviesTab();

    /**
     * Setup expectations: PlayMovies is open on any screen.
     *
     * PlayMovies will select the movie card and subsequently press the play button.
     */
    public void playMovie(String name);
}
