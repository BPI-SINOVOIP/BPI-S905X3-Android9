/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.tradefed.targetprep;


import java.io.File;

/**
 * Interface for retrieving auxiliary image files needed to flash a device.
 */
public interface IFlashingResourcesRetriever {

    /**
     * Returns the image file with given name and version
     *
     * @param imageName the name of the image (ie radio)
     * @param version the version of the file to return
     * @return a temporary local image {@link File}
     * @throws TargetSetupError if file could not be found
     */
    public File retrieveFile(String imageName, String version) throws TargetSetupError;

}
