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

package tests.support.resource;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;

import tests.support.Support_Configuration;

public class Support_Resources {

    public static final String RESOURCE_PACKAGE = "/tests/resources/";

    public static final String RESOURCE_PACKAGE_NAME = "tests.resources";

    public static InputStream getStream(String name) {
        // System.err.println("getResourceAsStream(" + RESOURCE_PACKAGE + name + ")");
        return Support_Resources.class.getResourceAsStream(RESOURCE_PACKAGE + name);
    }

    public static String getURL(String name) {
        String folder = null;
        String fileName = name;
        File resources = createTempFolder();
        int index = name.lastIndexOf("/");
        if (index != -1) {
            folder = name.substring(0, index);
            name = name.substring(index + 1);
        }
        copyFile(resources, folder, name);
        URL url = null;
        String resPath = resources.toString();
        if (resPath.charAt(0) == '/' || resPath.charAt(0) == '\\') {
            resPath = resPath.substring(1);
        }
        try {
            url = new URL("file:/" + resPath + "/" + fileName);
        } catch (MalformedURLException e) {
            throw new RuntimeException(e);
        }
        return url.toString();
    }

    public static File createTempFolder() {
        File folder = null;
        try {
            folder = File.createTempFile("hyts_resources", "", null);
            folder.delete();
            folder.mkdirs();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        folder.deleteOnExit();
        return folder;
    }

    public static void copyFile(File root, String folder, String file) {
        File f;
        if (folder != null) {
            f = new File(root.toString() + "/" + folder);
            if (!f.exists()) {
                f.mkdirs();
                f.deleteOnExit();
            }
        } else {
            f = root;
        }

        String src = folder == null ? file : folder + "/" + file;
        InputStream in = Support_Resources.getStream(src);
        try {
            File dst = new File(f.toString() + "/" + file);
            copyLocalFileTo(dst, in);
        } catch (Exception e) {
            throw new RuntimeException("copyFile failed: root=" + root + " folder=" + folder + " file=" + file + " (src=" + src + ")", e);
        }
    }

    public static File createTempFile(String suffix) throws IOException {
        return File.createTempFile("hyts_", suffix, null);
    }

    public static void copyLocalFileTo(File dest, InputStream in) throws IOException {
        if (!dest.exists()) {
            FileOutputStream out = new FileOutputStream(dest);
            int result;
            byte[] buf = new byte[4096];
            while ((result = in.read(buf)) != -1) {
                out.write(buf, 0, result);
            }
            in.close();
            out.close();
            dest.deleteOnExit();
        }
    }

    public static File getExternalLocalFile(String url) throws IOException, MalformedURLException {
        File resources = createTempFolder();
        InputStream in = new URL(url).openStream();
        File temp = new File(resources.toString() + "/local.tmp");
        copyLocalFileTo(temp, in);
        return temp;
    }

    public static String getResourceURL(String resource) {
        return "http://" + Support_Configuration.TestResources + resource;
    }

    /**
     * Util method to load resource files
     *
     * @param name - name of resource file
     * @return - resource input stream
     */
    public static InputStream getResourceStream(String name) {
        InputStream is = ClassLoader.getSystemClassLoader().getResourceAsStream(name);
        if (is == null) {
            throw new RuntimeException("Failed to load resource: " + name);
        }
        return is;
    }

    /**
     * Util method to get absolute path to resource file
     *
     * @param name - name of resource file
     * @return - path to resource
     */
    public static String getAbsoluteResourcePath(String name) {
        URL url = ClassLoader.getSystemClassLoader().getResource(name);
        if (url == null) {
            throw new RuntimeException("Failed to load resource: " + name);
        }

        try {
            return new File(url.toURI()).getAbsolutePath();
        } catch (URISyntaxException e) {
            throw new RuntimeException("Failed to load resource: " + name);
        }
    }
}
