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

package com.android.tools.build.apkzlib.zip;

import static org.junit.Assert.assertFalse;

import com.android.tools.build.apkzlib.utils.ApkZFileTestUtils;
import com.google.common.io.Files;
import java.io.File;
import java.io.IOException;
import javax.annotation.Nonnull;
import org.junit.rules.TemporaryFolder;

/**
 * Utility method for zip tests.
 */
class ZipTestUtils {

    /**
     * Obtains the data of a resource with the given name.
     *
     * @param rsrcName the resource name inside packaging resource folder
     * @return the resource data
     * @throws IOException I/O failed
     */
    @Nonnull
    static byte[] rsrcBytes(@Nonnull String rsrcName) throws IOException {
        return ApkZFileTestUtils.getResourceBytes("/testData/packaging/" + rsrcName).read();
    }

    /**
     * Clones a resource to a temporary folder. Generally, resources do not need to be cloned to
     * be used. However, in code where there is danger of changing resource files and corrupting
     * the source directory, cloning should be done before accessing the resources.
     *
     * @param rsrcName the resource name
     * @param folder the temporary folder
     * @return the file that was created with the resource
     * @throws IOException failed to clone the resource
     */
    static File cloneRsrc(@Nonnull String rsrcName, @Nonnull TemporaryFolder folder)
            throws IOException {
        String cloneName;
        if (rsrcName.contains("/")) {
            cloneName = rsrcName.substring(rsrcName.lastIndexOf('/') + 1);
        } else {
            cloneName = rsrcName;
        }

        return cloneRsrc(rsrcName, folder, cloneName);
    }

    /**
     * Clones a resource to a temporary folder. Generally, resources do not need to be cloned to
     * be used. However, in code where there is danger of changing resource files and corrupting
     * the source directory, cloning should be done before accessing the resources.
     *
     * @param rsrcName the resource name
     * @param folder the temporary folder
     * @param cloneName the name of the cloned resource that will be created inside the temporary
     * folder
     * @return the file that was created with the resource
     * @throws IOException failed to clone the resource
     */
    static File cloneRsrc(
            @Nonnull String rsrcName,
            @Nonnull TemporaryFolder folder,
            @Nonnull String cloneName)
            throws IOException {
        File result = new File(folder.getRoot(), cloneName);
        assertFalse(result.exists());

        Files.write(rsrcBytes(rsrcName), result);
        return result;
    }
}
