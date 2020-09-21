/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package libcore.java.util.jar;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.jar.JarEntry;
import java.util.jar.JarOutputStream;
import java.util.jar.Manifest;

public class OldJarOutputStreamTest extends junit.framework.TestCase {

    public void test_putNextEntryLjava_util_zip_ZipEntry() throws Exception {
        final String entryName = "foo/bar/execjartest/MainClass.class";
        Manifest newman = new Manifest();
        File outputJar = File.createTempFile("hyts_", ".jar");
        OutputStream os = new FileOutputStream(outputJar);
        JarOutputStream jout = new JarOutputStream(os, newman);
        os.close();
        try {
            jout.putNextEntry(new JarEntry(entryName));
            fail("IOException expected");
        } catch (IOException expected) {
        }
    }
}
