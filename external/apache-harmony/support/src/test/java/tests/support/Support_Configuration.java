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

package tests.support;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;

/**
 * This class is responsible for providing the dynamic names and addresses for
 * the java.net classes. There are two directories which need to be placed on an
 * ftp server and an http server which should accompany this source. The
 * ftp-files have to be placed on an ftp server and have to be the root of a
 * user jcltest with password jclpass. The testres files must be available on an
 * HTTP server and the name and location can be configured below.
 */
public class Support_Configuration {

    public static String DomainAddress = "apache.org";

    public static String WebName = "jcltest.";

    public static final String HomeAddress;

    public static String TestResourcesDir = "/testres231";

    public static final String TestResources;

    public static String HomeAddressResponse = "HTTP/1.1 200 OK";

    public static String HomeAddressSoftware = "Jetty(6.0.x)";

    public static String ProxyServerTestHost = "jcltest.apache.org";

    public static String SocksServerTestHost = "jcltest.apache.org";

    public static int SocksServerTestPort = 1080;

    // Need an IP address that does not resolve to a host name
    public static String UnresolvedIP = "192.168.99.99";

    // the bytes for an address which represents an address which is not
    // one of the addresses for any of our machines on which tests will run
    // it is used to verify we get the expected error when we try to bind
    // to an address that is not one of the machines local addresses
    public static byte nonLocalAddressBytes[] = { 1, 0, 0, 0 };

    public static String InetTestIP = "127.0.0.1";

    public static String InetTestIP2 = "127.0.0.1";

    public static final String HomeAddress6 = "jcltest6.apache.org";

    public static String ProxyServerTestHostIPv6 = "jcltest6.apache.org";

    // ip address that resolves to a host that is not present on the local
    // network
    // this allows us to check the timeouts for connect
    public static String ResolvedNotExistingHost = "9.26.194.72";

    /**
     * You can compute the hash code with the following code: try { String name =
     * "whatever.xxx.com";
     * System.out.println(InetAddress.getByName(name).hashCode()); } catch
     * (UnknownHostException e) {}
     */

    public static String FTPTestAddress = "jcltest:jclpass@localhost";

    public static String URLConnectionLastModifiedString = "Mon, 14 Jun 1999 21:06:22 GMT";

    public static long URLConnectionLastModified = 929394382000L;

    public static boolean RunCommTests = false;

    public static String Port1 = "COM1";

    public static String Port2 = "COM2";

    static Hashtable<String, String> props = null;

    static {
        loadProperties();
        HomeAddress = WebName + DomainAddress;
        TestResources = HomeAddress + TestResourcesDir;
    }

    static void loadProperties() {
        InputStream in = null;
        Hashtable<String, String> props = new Hashtable<String, String>();

        String iniName = System.getProperty("test.ini.file", "JCLAuto.ini");
        if (System.getProperty("test.comm") != null) {
            RunCommTests = true;
        }

        try {
            in = new FileInputStream(iniName);
        } catch (IOException e) {
        } catch (Exception e) {
            System.out.println("SupportConfiguration.loadProperties()");
            System.out.println(e);
            e.printStackTrace();
        }
        if (in == null) {
            try {
                Class<?> cl = Class
                        .forName("com.ibm.support.Support_Configuration");
                in = cl.getResourceAsStream(iniName);
            } catch (ClassNotFoundException e) {
            }
        }
        try {
            if (in != null) {
                load(in, props);
            }
        } catch (IOException e) {
        }
        if (props.size() == 0) {
            return;
        }
        String value;

        value = props.get("DomainAddress");
        if (value != null) {
            DomainAddress = value;
        }

        value = props.get("WebName");
        if (value != null) {
            WebName = value;
        }

        value = props.get("TestResourcesDir");
        if (value != null) {
            TestResourcesDir = value;
        }
        value = props.get("HomeAddressResponse");
        if (value != null) {
            HomeAddressResponse = value;
        }

        value = props.get("HomeAddressSoftware");
        if (value != null) {
            HomeAddressSoftware = value;
        }

        value = props.get("ProxyServerTestHost");
        if (value != null) {
            ProxyServerTestHost = value;
        }

        value = props.get("SocksServerTestHost");
        if (value != null) {
            SocksServerTestHost = value;
        }

        value = props.get("SocksServerTestPort");
        if (value != null) {
            SocksServerTestPort = Integer.parseInt(value);
        }

        value = props.get("UnresolvedIP");
        if (value != null) {
            UnresolvedIP = value;
        }

        value = props.get("FTPTestAddress");
        if (value != null) {
            FTPTestAddress = value;
        }

        value = props.get("URLConnectionLastModifiedString");
        if (value != null) {
            URLConnectionLastModifiedString = value;
        }

        value = props.get("URLConnectionLastModified");
        if (value != null) {
            URLConnectionLastModified = Long.parseLong(value);
        }

        value = props.get("Port1");
        if (value != null) {
            Port1 = value;
        }

        value = props.get("Port2");
        if (value != null) {
            Port2 = value;
        }

        value = props.get("ProxyServerTestHostIPv6");
        if (value != null) {
            ProxyServerTestHostIPv6 = value;
        }

        value = props.get("ResolvedNotExistingHost");
        if (value != null) {
            ResolvedNotExistingHost = value;
        }

    }

    static void load(InputStream in, Hashtable<String, String> result) throws IOException {
        int NONE = 0, SLASH = 1, UNICODE = 2, CONTINUE = 3, DONE = 4, IGNORE = 5;
        int mode = NONE, unicode = 0, count = 0, nextChar;
        StringBuffer key = new StringBuffer(), value = new StringBuffer(), buffer = key;
        boolean firstChar = true;

        while ((nextChar = in.read()) != -1) {
            if (mode == UNICODE) {
                int digit = Character.digit((char) nextChar, 16);
                if (digit >= 0) {
                    unicode = (unicode << 4) + digit;
                    if (++count < 4) {
                        continue;
                    }
                }
                mode = NONE;
                buffer.append((char) unicode);
                if (nextChar != '\n') {
                    continue;
                }
            }
            if (mode == SLASH) {
                mode = NONE;
                switch (nextChar) {
                    case '\r':
                        mode = CONTINUE; // Look for a following \n
                        continue;
                    case '\n':
                        mode = IGNORE; // Ignore whitespace on the next line
                        continue;
                    case 'b':
                        nextChar = '\b';
                        break;
                    case 'f':
                        nextChar = '\f';
                        break;
                    case 'n':
                        nextChar = '\n';
                        break;
                    case 'r':
                        nextChar = '\r';
                        break;
                    case 't':
                        nextChar = '\t';
                        break;
                    case 'u':
                        mode = UNICODE;
                        unicode = count = 0;
                        continue;
                }
            } else {
                switch (nextChar) {
                    case '#':
                    case '!':
                        if (firstChar) {
                            while ((nextChar = in.read()) != -1) {
                                if (nextChar == '\r' || nextChar == '\n') {
                                    break;
                                }
                            }
                            continue;
                        }
                        break;
                    case '\n':
                        if (mode == CONTINUE) { // Part of a \r\n sequence
                            mode = IGNORE; // Ignore whitespace on the next line
                            continue;
                        }
                        // fall into the next case
                    case '\r':
                        mode = NONE;
                        firstChar = true;
                        if (key.length() > 0 || buffer == value) {
                            result.put(key.toString(), value.toString());
                        }
                        key.setLength(0);
                        value.setLength(0);
                        buffer = key;
                        continue;
                    case '\\':
                        mode = SLASH;
                        continue;
                    case ':':
                    case '=':
                        if (buffer == key) {
                            buffer = value;
                            continue;
                        }
                        break;
                }
                char c = (char) nextChar;
                if ((c >= 0x1c && c <= 0x20) || (c >= 0x9 && c <= 0xd)) {
                    if (mode == CONTINUE) {
                        mode = IGNORE;
                    }
                    if (buffer.length() == 0 || mode == IGNORE) {
                        continue;
                    }
                    if (buffer == key) {
                        mode = DONE;
                        continue;
                    }
                }
                if (mode == IGNORE || mode == CONTINUE) {
                    mode = NONE;
                }
            }
            firstChar = false;
            if (mode == DONE) {
                buffer = value;
                mode = NONE;
            }
            buffer.append((char) nextChar);
        }
        if (key.length() > 0 || buffer == value) {
            result.put(key.toString(), value.toString());
        }
    }

}
