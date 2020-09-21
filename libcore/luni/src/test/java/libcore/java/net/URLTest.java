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

package libcore.java.net;

import junit.framework.TestCase;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;
import java.net.URLStreamHandlerFactory;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Callable;
import libcore.libcore.util.SerializationTester;

import dalvik.system.BlockGuard;

import static java.util.Arrays.asList;

public final class URLTest extends TestCase {

    public void testUrlParts() throws Exception {
        URL url = new URL("http://username:password@host:8080/directory/file?query#ref");
        assertEquals("http", url.getProtocol());
        assertEquals("username:password@host:8080", url.getAuthority());
        assertEquals("username:password", url.getUserInfo());
        assertEquals("host", url.getHost());
        assertEquals(8080, url.getPort());
        assertEquals(80, url.getDefaultPort());
        assertEquals("/directory/file?query", url.getFile());
        assertEquals("/directory/file", url.getPath());
        assertEquals("query", url.getQuery());
        assertEquals("ref", url.getRef());
    }
    // http://code.google.com/p/android/issues/detail?id=12724
    public void testExplicitPort() throws Exception {
        URL url = new URL("http://www.google.com:80/example?language[id]=2");
        assertEquals("www.google.com", url.getHost());
        assertEquals(80, url.getPort());
    }

    /**
     * Android's URL.equals() works as if the network is down. This is different
     * from the RI, which does potentially slow and inconsistent DNS lookups in
     * URL.equals.
     */
    public void testEqualsDoesNotDoHostnameResolution() throws Exception {
        for (InetAddress inetAddress : InetAddress.getAllByName("localhost")) {
            String address = inetAddress.getHostAddress();
            if (inetAddress instanceof Inet6Address) {
                address = "[" + address + "]";
            }
            URL urlByHostName = new URL("http://localhost/foo?bar=baz#quux");
            URL urlByAddress = new URL("http://" + address + "/foo?bar=baz#quux");
            assertFalse("Expected " + urlByHostName + " to not equal " + urlByAddress,
                    urlByHostName.equals(urlByAddress)); // fails on RI, which does DNS
        }
    }

    public void testEqualsCaseMapping() throws Exception {
        assertEquals(new URL("HTTP://localhost/foo?bar=baz#quux"),
                new URL("HTTP://localhost/foo?bar=baz#quux"));
        assertTrue(new URL("http://localhost/foo?bar=baz#quux").equals(
                new URL("http://LOCALHOST/foo?bar=baz#quux")));
        assertFalse(new URL("http://localhost/foo?bar=baz#quux").equals(
                new URL("http://localhost/FOO?bar=baz#quux")));
        assertFalse(new URL("http://localhost/foo?bar=baz#quux").equals(
                new URL("http://localhost/foo?BAR=BAZ#quux")));
        assertFalse(new URL("http://localhost/foo?bar=baz#quux").equals(
                new URL("http://localhost/foo?bar=baz#QUUX")));
    }

    public void testFileEqualsWithEmptyHost() throws Exception {
        assertEquals(new URL("file", "", -1, "/a/"), new URL("file:/a/"));
    }

    public void testHttpEqualsWithEmptyHost() throws Exception {
        assertEquals(new URL("http", "", 80, "/a/"), new URL("http:/a/"));
        assertFalse(new URL("http", "", 80, "/a/").equals(new URL("http://host/a/")));
    }

    public void testFileEquals() throws Exception {
        assertEquals(new URL("file", null, -1, "/a"), new URL("file", null, -1, "/a"));
        assertFalse(new URL("file", null, -1, "/a").equals(new URL("file", null, -1, "/A")));
    }

    public void testJarEquals() throws Exception {
        assertEquals(new URL("jar", null, -1, "/a!b"), new URL("jar", null, -1, "/a!b"));
        assertFalse(new URL("jar", null, -1, "/a!b").equals(new URL("jar", null, -1, "/a!B")));
        assertFalse(new URL("jar", null, -1, "/a!b").equals(new URL("jar", null, -1, "/A!b")));
    }

    public void testUrlSerialization() throws Exception {
        String s = "aced00057372000c6a6176612e6e65742e55524c962537361afce472030006490004706f72744c0"
                + "009617574686f726974797400124c6a6176612f6c616e672f537472696e673b4c000466696c65710"
                + "07e00014c0004686f737471007e00014c000870726f746f636f6c71007e00014c000372656671007"
                + "e00017870ffffffff74000e757365723a7061737340686f73747400102f706174682f66696c653f7"
                + "175657279740004686f7374740004687474707400046861736878";
        URL url = new URL("http://user:pass@host/path/file?query#hash");
        new SerializationTester<URL>(url, s).test();
    }

    /**
     * For a custom URLStreamHandler, a (de)serialization round trip reconstructs an
     * inconsistently null authority from host and port.
     */
    public void testUrlSerializationRoundTrip_customHandler_nullAuthorityReconstructed()
            throws Exception {
        withCustomURLStreamHandlerFactory(() -> {
            URL url = new URL("android://example.com:80/file");
            getUrlField("authority").set(url, null);
            URL reserializedUrl = (URL) SerializationTester.reserialize(url);

            assertFields(url, "android", "example.com", 80, null, "/file");
            assertFields(reserializedUrl, "android", "example.com", 80, "example.com:80", "/file");
            return null;
        });
    }

    /**
     * For a custom URLStreamHandler, a (de)serialization round trip does not reconstruct
     * an inconsistent but nonnull authority from host and port.
     */
    public void testUrlSerializationRoundTrip_customHandler_nonnullAuthorityNotReconstructed()
            throws Exception {
        withCustomURLStreamHandlerFactory(() -> {
            URL url = new URL("android://example.com/file");
            getUrlField("authority").set(url, "evil.com:1234");
            URL reserializedUrl = (URL) SerializationTester.reserialize(url);

            assertFields(url, "android", "example.com", -1, "evil.com:1234", "/file");
            assertFields(reserializedUrl, "android", "example.com", -1, "evil.com:1234", "/file");
            return null;
        });
    }

    /**
     * For a custom URLStreamHandler, a (de)serialization round trip does not
     * reconstruct host and port from the authority, even if host is null.
     */
    public void testUrlSerializationRoundTrip_customHandler_hostAndPortNotReconstructed()
            throws Exception {
        checkUrlSerializationRoundTrip_customHandler_hostAndPortNotReconstructed(null /* host */);
        checkUrlSerializationRoundTrip_customHandler_hostAndPortNotReconstructed("evil.com");
    }

    private void checkUrlSerializationRoundTrip_customHandler_hostAndPortNotReconstructed(
            final String hostOrNull) throws Exception {
        withCustomURLStreamHandlerFactory(() -> {
            URL url = new URL("android://example.com:80/file");
            getUrlField("host").set(url, hostOrNull);
            getUrlField("port").set(url, 12345);
            URL reserializedUrl = (URL) SerializationTester.reserialize(url);

            assertFields(url, "android", hostOrNull, 12345, "example.com:80", "/file");
            assertFields(reserializedUrl, "android", hostOrNull, 12345, "example.com:80", "/file");
            return null;
        });
    }

    /**
     * Temporarily registers a {@link URLStreamHandlerFactory} that accepts any protocol,
     * and then, while that factory is registered, runs the given {@code callable} on this
     * thread.
     * @throw Exception any Exception thrown by the Callable will be thrown on to the caller.
     */
    private static void withCustomURLStreamHandlerFactory(Callable<Void> callable)
            throws Exception {
        Field factoryField = getUrlField("factory");
        assertTrue(Modifier.isStatic(factoryField.getModifiers()));
        URLStreamHandlerFactory oldFactory = (URLStreamHandlerFactory) factoryField.get(null);
        try {
            URL.setURLStreamHandlerFactory(
                    protocol -> new libcore.java.net.customstreamhandler.http.Handler());
            callable.call();
        } finally {
            factoryField.set(null, null);
            URL.setURLStreamHandlerFactory(oldFactory);
        }
    }

    /**
     * Host and port are reconstructed from the authority during deserialization.
     */
    public void testUrlSerializationRoundTrip_builtinHandler_hostAndPortReconstructed()
            throws Exception {
        URL url = new URL("http://example.com:42/file");
        getUrlField("host").set(url, "wronghost.com");
        getUrlField("port").setInt(url, 1234);
        URL reserializedUrl = (URL) SerializationTester.reserialize(url);
        assertFields(url, "http", "wronghost.com", 1234, "example.com:42", "/file");
        assertFields(reserializedUrl, "http", "example.com", 42, "example.com:42", "/file");

        // Check that the normalization occurs during deserialization rather than during
        // serialization.
        assertFalse(Arrays.equals(
                SerializationTester.serialize(url),
                SerializationTester.serialize(reserializedUrl)
        ));
        assertTrue(Arrays.equals(
                SerializationTester.serialize(reserializedUrl),
                SerializationTester.serialize(reserializedUrl)
        ));
    }

    /**
     * The authority is not reconstructed from host and port, but the other way around.
     */
    public void testUrlSerializationRoundTrip_builtinHandler_authorityNotReconstructed()
            throws Exception {
        URL url = new URL("http://example.com/file");
        getUrlField("authority").set(url, "newhost.com:80");
        URL reserializedUrl = (URL) SerializationTester.reserialize(url);

        assertFields(url, "http", "example.com", -1, "newhost.com:80", "/file");
        assertFields(reserializedUrl, "http", "newhost.com", 80, "newhost.com:80", "/file");
    }

    /**
     * The boundary where the authority part ends and the file part starts is
     * reconstructed during deserialization.
     */
    public void testUrlSerializationRoundTrip_builtinHandler_authorityAndFileReconstructed()
            throws Exception {
        URL url = new URL("http://temporaryhost.com/temporaryfile");
        getUrlField("authority").set(url, "exam");
        getUrlField("file").set(url, "ple.com:80/file");
        URL reserializedUrl = (URL) SerializationTester.reserialize(url);
        assertFields(reserializedUrl, "http", "example.com", 80, "example.com:80", "/file");
    }

    private static Field getUrlField(String fieldName) throws Exception {
        Field result = URL.class.getDeclaredField(fieldName);
        result.setAccessible(true);
        return result;
    }

    private static void assertFields(URL url,
            String protocol, String host, int port, String authority, String file) {
        assertEquals(
                asList(protocol, host, port, authority, file),
                asList(url.getProtocol(), url.getHost(), url.getPort(), url.getAuthority(),
                        url.getFile()));
    }

    /**
     * The serialized form of a URL includes its hash code. But the hash code
     * is not documented. Check that we don't return a deserialized hash code
     * from a deserialized value.
     */
    public void testUrlSerializationWithHashCode() throws Exception {
        String s = "aced00057372000c6a6176612e6e65742e55524c962537361afce47203000749000868617368436"
                + "f6465490004706f72744c0009617574686f726974797400124c6a6176612f6c616e672f537472696"
                + "e673b4c000466696c6571007e00014c0004686f737471007e00014c000870726f746f636f6c71007"
                + "e00014c000372656671007e00017870cdf0efacffffffff74000e757365723a7061737340686f737"
                + "47400102f706174682f66696c653f7175657279740004686f7374740004687474707400046861736"
                + "878";
        final URL url = new URL("http://user:pass@host/path/file?query#hash");
        new SerializationTester<URL>(url, s) {
            @Override protected void verify(URL deserialized) {
                assertEquals(url.hashCode(), deserialized.hashCode());
            }
        }.test();
    }

    public void testOnlySupportedProtocols() {
        try {
            new URL("abcd://host");
            fail();
        } catch (MalformedURLException expected) {
        }
    }

    public void testOmittedHost() throws Exception {
        URL url = new URL("http:///path");
        assertEquals("", url.getHost());
        assertEquals("/path", url.getFile());
        assertEquals("/path", url.getPath());
    }

    public void testNoHost() throws Exception {
        URL url = new URL("http:/path");
        assertEquals("http", url.getProtocol());
        assertEquals(null, url.getAuthority());
        assertEquals(null, url.getUserInfo());
        assertEquals("", url.getHost());
        assertEquals(-1, url.getPort());
        assertEquals(80, url.getDefaultPort());
        assertEquals("/path", url.getFile());
        assertEquals("/path", url.getPath());
        assertEquals(null, url.getQuery());
        assertEquals(null, url.getRef());
    }

    public void testNoPath() throws Exception {
        URL url = new URL("http://host");
        assertEquals("host", url.getHost());
        assertEquals("", url.getFile());
        assertEquals("", url.getPath());
    }

    public void testEmptyHostAndNoPath() throws Exception {
        URL url = new URL("http://");
        assertEquals("http", url.getProtocol());
        assertEquals("", url.getAuthority());
        assertEquals(null, url.getUserInfo());
        assertEquals("", url.getHost());
        assertEquals(-1, url.getPort());
        assertEquals(80, url.getDefaultPort());
        assertEquals("", url.getFile());
        assertEquals("", url.getPath());
        assertEquals(null, url.getQuery());
        assertEquals(null, url.getRef());
    }

    public void testNoHostAndNoPath() throws Exception {
        URL url = new URL("http:");
        assertEquals("http", url.getProtocol());
        assertEquals(null, url.getAuthority());
        assertEquals(null, url.getUserInfo());
        assertEquals("", url.getHost());
        assertEquals(-1, url.getPort());
        assertEquals(80, url.getDefaultPort());
        assertEquals("", url.getFile());
        assertEquals("", url.getPath());
        assertEquals(null, url.getQuery());
        assertEquals(null, url.getRef());
    }

    // This behavior of URLs with invalid user info changed due to bug http://b/33351987 after
    // Android security level 1 April 2017.
    public void testAtSignInUserInfo() throws Exception {
        URL url = new URL("http://user@userhost.com:password@host");
        assertNull(url.getUserInfo());
        assertTrue(url.getHost().isEmpty());
    }

    public void testUserNoPassword() throws Exception {
        URL url = new URL("http://user@host");
        assertEquals("user@host", url.getAuthority());
        assertEquals("user", url.getUserInfo());
        assertEquals("host", url.getHost());
    }

    public void testUserNoPasswordExplicitPort() throws Exception {
        URL url = new URL("http://user@host:8080");
        assertEquals("user@host:8080", url.getAuthority());
        assertEquals("user", url.getUserInfo());
        assertEquals("host", url.getHost());
        assertEquals(8080, url.getPort());
    }

    public void testUserPasswordHostPort() throws Exception {
        URL url = new URL("http://user:password@host:8080");
        assertEquals("user:password@host:8080", url.getAuthority());
        assertEquals("user:password", url.getUserInfo());
        assertEquals("host", url.getHost());
        assertEquals(8080, url.getPort());
    }

    public void testUserPasswordEmptyHostPort() throws Exception {
        URL url = new URL("http://user:password@:8080");
        assertEquals("user:password@:8080", url.getAuthority());
        assertEquals("user:password", url.getUserInfo());
        assertEquals("", url.getHost());
        assertEquals(8080, url.getPort());
    }

    public void testUserPasswordEmptyHostEmptyPort() throws Exception {
        URL url = new URL("http://user:password@");
        assertEquals("user:password@", url.getAuthority());
        assertEquals("user:password", url.getUserInfo());
        assertEquals("", url.getHost());
        assertEquals(-1, url.getPort());
    }

    public void testPathOnly() throws Exception {
        URL url = new URL("http://host/path");
        assertEquals("/path", url.getFile());
        assertEquals("/path", url.getPath());
    }

    public void testQueryOnly() throws Exception {
        URL url = new URL("http://host?query");
        assertEquals("?query", url.getFile());
        assertEquals("", url.getPath());
        assertEquals("query", url.getQuery());
    }

    public void testFragmentOnly() throws Exception {
        URL url = new URL("http://host#fragment");
        assertEquals("", url.getFile());
        assertEquals("", url.getPath());
        assertEquals("fragment", url.getRef());
    }

    public void testAtSignInPath() throws Exception {
        URL url = new URL("http://host/file@foo");
        assertEquals("/file@foo", url.getFile());
        assertEquals("/file@foo", url.getPath());
        assertEquals(null, url.getUserInfo());
    }

    public void testColonInPath() throws Exception {
        URL url = new URL("http://host/file:colon");
        assertEquals("/file:colon", url.getFile());
        assertEquals("/file:colon", url.getPath());
    }

    public void testSlashInQuery() throws Exception {
        URL url = new URL("http://host/file?query/path");
        assertEquals("/file?query/path", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("query/path", url.getQuery());
    }

    public void testQuestionMarkInQuery() throws Exception {
        URL url = new URL("http://host/file?query?another");
        assertEquals("/file?query?another", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("query?another", url.getQuery());
    }

    public void testAtSignInQuery() throws Exception {
        URL url = new URL("http://host/file?query@at");
        assertEquals("/file?query@at", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("query@at", url.getQuery());
    }

    public void testCommonProtocolsAreHandledByBuiltinHandlers() throws Exception {
        Method getURLStreamHandler = URL.class.getDeclaredMethod(
                "getURLStreamHandler", String.class);
        getURLStreamHandler.setAccessible(true);
        Set<String> builtinHandlers =
                (Set<String>) getUrlField("BUILTIN_HANDLER_CLASS_NAMES").get(null);
        Set<String> commonHandlers = new HashSet<>();
        for (String protocol : Arrays.asList("file", "ftp", "jar", "http", "https")) {
            URLStreamHandler handler =
                    (URLStreamHandler) getURLStreamHandler.invoke(null, protocol);
            assertNotNull("Handler for protocol " + protocol + " should exist", handler);
            commonHandlers.add(handler.getClass().getName());
        }
        assertTrue("Built-in handlers " + builtinHandlers + " should contain all of the handlers "
                + commonHandlers + " for common protocols.",
                builtinHandlers.containsAll(commonHandlers));
    }

    public void testColonInQuery() throws Exception {
        URL url = new URL("http://host/file?query:colon");
        assertEquals("/file?query:colon", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("query:colon", url.getQuery());
    }

    public void testQuestionMarkInFragment() throws Exception {
        URL url = new URL("http://host/file#fragment?query");
        assertEquals("/file", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals(null, url.getQuery());
        assertEquals("fragment?query", url.getRef());
    }

    public void testColonInFragment() throws Exception {
        URL url = new URL("http://host/file#fragment:80");
        assertEquals("/file", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals(-1, url.getPort());
        assertEquals("fragment:80", url.getRef());
    }

    public void testSlashInFragment() throws Exception {
        URL url = new URL("http://host/file#fragment/path");
        assertEquals("/file", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("fragment/path", url.getRef());
    }

    public void testSlashInFragmentCombiningConstructor() throws Exception {
        URL url = new URL("http", "host", "/file#fragment/path");
        assertEquals("/file", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("fragment/path", url.getRef());
    }

    public void testHashInFragment() throws Exception {
        URL url = new URL("http://host/file#fragment#another");
        assertEquals("/file", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("fragment#another", url.getRef());
    }

    public void testEmptyPort() throws Exception {
        URL url = new URL("http://host:/");
        assertEquals(-1, url.getPort());
    }

    public void testNonNumericPort() throws Exception {
        try {
            new URL("http://host:x/");
            fail();
        } catch (MalformedURLException expected) {
        }
    }

    public void testPortWithMinusSign() throws Exception {
        try {
            new URL("http://host:-2/");
            fail();
        } catch (MalformedURLException expected) {
        }
    }

    public void testPortWithPlusSign() throws Exception {
        try {
            new URL("http://host:+2/");
            fail();
        } catch (MalformedURLException expected) {
        }
    }

    public void testPortNonASCII() throws Exception {
        try {
            new URL("http://host:١٢٣/"); // 123 in arabic
            fail();
        } catch (MalformedURLException expected) {
        }
    }

    public void testNegativePortEqualsPlaceholder() throws Exception {
        try {
            new URL("http://host:-1/");
            fail(); // RI fails this
        } catch (MalformedURLException expected) {
        }
    }

    public void testRelativePathOnQuery() throws Exception {
        URL base = new URL("http://host/file?query/x");
        URL url = new URL(base, "another");
        assertEquals("http://host/another", url.toString());
        assertEquals("/another", url.getFile());
        assertEquals("/another", url.getPath());
        assertEquals(null, url.getQuery());
        assertEquals(null, url.getRef());
    }

    public void testRelativeFragmentOnQuery() throws Exception {
        URL base = new URL("http://host/file?query/x#fragment");
        URL url = new URL(base, "#another");
        assertEquals("http://host/file?query/x#another", url.toString());
        assertEquals("/file?query/x", url.getFile());
        assertEquals("/file", url.getPath());
        assertEquals("query/x", url.getQuery());
        assertEquals("another", url.getRef());
    }

    public void testPathContainsRelativeParts() throws Exception {
        URL url = new URL("http://host/a/b/../c");
        assertEquals("http://host/a/c", url.toString()); // RI doesn't canonicalize
    }

    public void testRelativePathAndFragment() throws Exception {
        URL base = new URL("http://host/file");
        assertEquals("http://host/another#fragment", new URL(base, "another#fragment").toString());
    }

    public void testRelativeParentDirectory() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://host/a/d", new URL(base, "../d").toString());
    }

    public void testRelativeChildDirectory() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://host/a/b/d/e", new URL(base, "d/e").toString());
    }

    public void testRelativeRootDirectory() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://host/d", new URL(base, "/d").toString());
    }

    public void testRelativeFullUrl() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://host2/d/e", new URL(base, "http://host2/d/e").toString());
        assertEquals("https://host2/d/e", new URL(base, "https://host2/d/e").toString());
    }

    public void testRelativeDifferentScheme() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("https://host2/d/e", new URL(base, "https://host2/d/e").toString());
    }

    public void testRelativeDifferentAuthority() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://another/d/e", new URL(base, "//another/d/e").toString());
    }

    public void testRelativeWithScheme() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://host/a/b/c", new URL(base, "http:").toString());
        assertEquals("http://host/", new URL(base, "http:/").toString());
    }

    public void testMalformedUrlsRefusedByFirefoxAndChrome() throws Exception {
        URL base = new URL("http://host/a/b/c");
        assertEquals("http://", new URL(base, "http://").toString()); // fails on RI; path retained
        assertEquals("http://", new URL(base, "//").toString()); // fails on RI
        assertEquals("https:", new URL(base, "https:").toString());
        assertEquals("https:/", new URL(base, "https:/").toString());
        assertEquals("https://", new URL(base, "https://").toString());
    }

    public void testRfc1808NormalExamples() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        assertEquals("https:h", new URL(base, "https:h").toString());
        assertEquals("http://a/b/c/g", new URL(base, "g").toString());
        assertEquals("http://a/b/c/g", new URL(base, "./g").toString());
        assertEquals("http://a/b/c/g/", new URL(base, "g/").toString());
        assertEquals("http://a/g", new URL(base, "/g").toString());
        assertEquals("http://g", new URL(base, "//g").toString());
        assertEquals("http://a/b/c/d;p?y", new URL(base, "?y").toString()); // RI fails; file lost
        assertEquals("http://a/b/c/g?y", new URL(base, "g?y").toString());
        assertEquals("http://a/b/c/d;p?q#s", new URL(base, "#s").toString());
        assertEquals("http://a/b/c/g#s", new URL(base, "g#s").toString());
        assertEquals("http://a/b/c/g?y#s", new URL(base, "g?y#s").toString());
        assertEquals("http://a/b/c/;x", new URL(base, ";x").toString());
        assertEquals("http://a/b/c/g;x", new URL(base, "g;x").toString());
        assertEquals("http://a/b/c/g;x?y#s", new URL(base, "g;x?y#s").toString());
        assertEquals("http://a/b/c/d;p?q", new URL(base, "").toString());
        assertEquals("http://a/b/c/", new URL(base, ".").toString());
        assertEquals("http://a/b/c/", new URL(base, "./").toString());
        assertEquals("http://a/b/", new URL(base, "..").toString());
        assertEquals("http://a/b/", new URL(base, "../").toString());
        assertEquals("http://a/b/g", new URL(base, "../g").toString());
        assertEquals("http://a/", new URL(base, "../..").toString());
        assertEquals("http://a/", new URL(base, "../../").toString());
        assertEquals("http://a/g", new URL(base, "../../g").toString());
    }

    public void testRfc1808AbnormalExampleTooManyDotDotSequences() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        assertEquals("http://a/g", new URL(base, "../../../g").toString()); // RI doesn't normalize
        assertEquals("http://a/g", new URL(base, "../../../../g").toString());
    }

    public void testRfc1808AbnormalExampleRemoveDotSegments() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        assertEquals("http://a/g", new URL(base, "/./g").toString()); // RI doesn't normalize
        assertEquals("http://a/g", new URL(base, "/../g").toString()); // RI doesn't normalize
        assertEquals("http://a/b/c/g.", new URL(base, "g.").toString());
        assertEquals("http://a/b/c/.g", new URL(base, ".g").toString());
        assertEquals("http://a/b/c/g..", new URL(base, "g..").toString());
        assertEquals("http://a/b/c/..g", new URL(base, "..g").toString());
    }

    public void testRfc1808AbnormalExampleNonsensicalDots() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        assertEquals("http://a/b/g", new URL(base, "./../g").toString());
        assertEquals("http://a/b/c/g/", new URL(base, "./g/.").toString());
        assertEquals("http://a/b/c/g/h", new URL(base, "g/./h").toString());
        assertEquals("http://a/b/c/h", new URL(base, "g/../h").toString());
        assertEquals("http://a/b/c/g;x=1/y", new URL(base, "g;x=1/./y").toString());
        assertEquals("http://a/b/c/y", new URL(base, "g;x=1/../y").toString());
    }

    public void testRfc1808AbnormalExampleRelativeScheme() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        // this result is permitted; strict parsers prefer "http:g"
        assertEquals("http://a/b/c/g", new URL(base, "http:g").toString());
    }

    public void testRfc1808AbnormalExampleQueryOrFragmentDots() throws Exception {
        URL base = new URL("http://a/b/c/d;p?q");
        assertEquals("http://a/b/c/g?y/./x", new URL(base, "g?y/./x").toString());
        assertEquals("http://a/b/c/g?y/../x", new URL(base, "g?y/../x").toString());
        assertEquals("http://a/b/c/g#s/./x", new URL(base, "g#s/./x").toString());
        assertEquals("http://a/b/c/g#s/../x", new URL(base, "g#s/../x").toString());
    }

    public void testSquareBracketsInUserInfo() throws Exception {
        URL url = new URL("http://user:[::1]@host");
        assertEquals("user:[::1]", url.getUserInfo());
        assertEquals("host", url.getHost());
    }

    public void testComposeUrl() throws Exception {
        URL url = new URL("http", "host", "a");
        assertEquals("http", url.getProtocol());
        assertEquals("host", url.getAuthority());
        assertEquals("host", url.getHost());
        assertEquals("/a", url.getFile()); // RI fails; doesn't insert '/' separator
        assertEquals("http://host/a", url.toString()); // fails on RI
    }

    public void testComposeUrlWithNullHost() throws Exception {
        URL url = new URL("http", null, "a");
        assertEquals("http", url.getProtocol());
        assertEquals(null, url.getAuthority());
        assertEquals(null, url.getHost());
        assertEquals("a", url.getFile());
        assertEquals("http:a", url.toString()); // fails on RI
    }

    public void testFileUrlExtraLeadingSlashes() throws Exception {
        URL url = new URL("file:////foo");
        assertEquals("", url.getAuthority()); // RI returns null
        assertEquals("//foo", url.getPath());
        assertEquals("file:////foo", url.toString());
    }

    public void testFileUrlWithAuthority() throws Exception {
        URL url = new URL("file://x/foo");
        assertEquals("x", url.getAuthority());
        assertEquals("/foo", url.getPath());
        assertEquals("file://x/foo", url.toString());
    }

    /**
     * The RI is not self-consistent on missing authorities, returning either
     * null or the empty string depending on the number of slashes in the path.
     * We always treat '//' as the beginning of an authority.
     */
    public void testEmptyAuthority() throws Exception {
        URL url = new URL("http:///foo");
        assertEquals("", url.getAuthority());
        assertEquals("/foo", url.getPath());
        assertEquals("http:///foo", url.toString()); // RI drops '//'
    }

    public void testHttpUrlExtraLeadingSlashes() throws Exception {
        URL url = new URL("http:////foo");
        assertEquals("", url.getAuthority()); // RI returns null
        assertEquals("//foo", url.getPath());
        assertEquals("http:////foo", url.toString());
    }

    public void testFileUrlRelativePath() throws Exception {
        URL base = new URL("file:a/b/c");
        assertEquals("file:a/b/d", new URL(base, "d").toString());
    }

    public void testFileUrlDottedPath() throws Exception {
        URL url = new URL("file:../a/b");
        assertEquals("../a/b", url.getPath());
        assertEquals("file:../a/b", url.toString());
    }

    public void testParsingDotAsHostname() throws Exception {
        URL url = new URL("http://./");
        assertEquals(".", url.getAuthority());
        assertEquals(".", url.getHost());
    }

    public void testSquareBracketsWithIPv4() throws Exception {
        try {
            new URL("http://[192.168.0.1]/");
            fail();
        } catch (MalformedURLException expected) {
        }
        URL url = new URL("http", "[192.168.0.1]", "/");
        assertEquals("[192.168.0.1]", url.getHost());
    }

    public void testSquareBracketsWithHostname() throws Exception {
        try {
            new URL("http://[www.android.com]/");
            fail();
        } catch (MalformedURLException expected) {
        }
        URL url = new URL("http", "[www.android.com]", "/");
        assertEquals("[www.android.com]", url.getHost());
    }

    public void testIPv6WithoutSquareBrackets() throws Exception {
        try {
            new URL("http://fe80::1234/");
            fail();
        } catch (MalformedURLException expected) {
        }
        URL url = new URL("http", "fe80::1234", "/");
        assertEquals("[fe80::1234]", url.getHost());
    }

    public void testIpv6WithSquareBrackets() throws Exception {
        URL url = new URL("http://[::1]:2/");
        assertEquals("[::1]", url.getHost());
        assertEquals(2, url.getPort());
    }

    public void testEqualityWithNoPath() throws Exception {
        assertFalse(new URL("http://android.com").equals(new URL("http://android.com/")));
    }

    public void testUrlDoesNotEncodeParts() throws Exception {
        URL url = new URL("http", "host", 80, "/doc|search?q=green robots#over 6\"");
        assertEquals("http", url.getProtocol());
        assertEquals("host:80", url.getAuthority());
        assertEquals("/doc|search", url.getPath());
        assertEquals("q=green robots", url.getQuery());
        assertEquals("over 6\"", url.getRef());
        assertEquals("http://host:80/doc|search?q=green robots#over 6\"", url.toString());
    }

    public void testSchemeCaseIsCanonicalized() throws Exception {
        URL url = new URL("HTTP://host/path");
        assertEquals("http", url.getProtocol());
    }

    public void testEmptyAuthorityWithPath() throws Exception {
        URL url = new URL("http:///path");
        assertEquals("", url.getAuthority());
        assertEquals("/path", url.getPath());
    }

    public void testEmptyAuthorityWithQuery() throws Exception {
        URL url = new URL("http://?query");
        assertEquals("", url.getAuthority());
        assertEquals("", url.getPath());
        assertEquals("query", url.getQuery());
    }

    public void testEmptyAuthorityWithFragment() throws Exception {
        URL url = new URL("http://#fragment");
        assertEquals("", url.getAuthority());
        assertEquals("", url.getPath());
        assertEquals("fragment", url.getRef());
    }

    public void testCombiningConstructorsMakeRelativePathsAbsolute() throws Exception {
        assertEquals("/relative", new URL("http", "host", "relative").getPath());
        assertEquals("/relative", new URL("http", "host", -1, "relative").getPath());
        assertEquals("/relative", new URL("http", "host", -1, "relative", null).getPath());
    }

    public void testCombiningConstructorsDoNotMakeEmptyPathsAbsolute() throws Exception {
        assertEquals("", new URL("http", "host", "").getPath());
        assertEquals("", new URL("http", "host", -1, "").getPath());
        assertEquals("", new URL("http", "host", -1, "", null).getPath());
    }

    public void testPartContainsSpace() throws Exception {
        try {
            new URL("ht tp://host/");
            fail();
        } catch (MalformedURLException expected) {
        }
        assertEquals("user name", new URL("http://user name@host/").getUserInfo());
        assertEquals("ho st", new URL("http://ho st/").getHost());
        try {
            new URL("http://host:80 80/");
            fail();
        } catch (MalformedURLException expected) {
        }
        assertEquals("/fi le", new URL("http://host/fi le").getFile());
        assertEquals("que ry", new URL("http://host/file?que ry").getQuery());
        assertEquals("re f", new URL("http://host/file?query#re f").getRef());
    }

    // http://code.google.com/p/android/issues/detail?id=37577
    public void testUnderscore() throws Exception {
        URL url = new URL("http://a_b.c.d.net/");
        assertEquals("a_b.c.d.net", url.getAuthority());
        // The RFC's don't permit underscores in hostnames, but URL accepts them (unlike URI).
        assertEquals("a_b.c.d.net", url.getHost());
    }

    // http://b/26895969
    // http://b/26798800
    public void testHashCodeAndEqualsDoesNotPerformNetworkIo() throws Exception {
        final BlockGuard.Policy oldPolicy = BlockGuard.getThreadPolicy();
        BlockGuard.setThreadPolicy(new BlockGuard.Policy() {
            @Override
            public void onWriteToDisk() {
                fail("Blockguard.Policy.onWriteToDisk");
            }

            @Override
            public void onReadFromDisk() {
                fail("Blockguard.Policy.onReadFromDisk");
            }

            @Override
            public void onNetwork() {
                fail("Blockguard.Policy.onNetwork");
            }

            @Override
            public void onUnbufferedIO() {
                fail("Blockguard.Policy.onUnbufferedIO");
            }

            @Override
            public int getPolicyMask() {
                return 0;
            }
        });

        try {
            URL url = new URL("http://www.google.com/");
            URL url2 = new URL("http://www.nest.com/");

            url.equals(url2);
            url2.hashCode();
        } finally {
            BlockGuard.setThreadPolicy(oldPolicy);
        }
    }

    // http://27444667
    public void testEmptyQueryAndAnchor() throws Exception {
        assertEquals("/some/path", new URL("http://foobar.com/some/path?").getFile());
        assertEquals("/some/path", new URL("http://foobar.com/some/path#").getFile());
        assertEquals("/some/path", new URL("http://foobar.com/some/path?#").getFile());
    }

    // http://b/31858037
    public void testFragmentWithSlash() throws Exception {
        final String host = "example.com";
        final String fragment = "@not-a-host-name/a";
        URL url = new URL(String.format("http://%s#%s", host, fragment));
        assertNull(url.getUserInfo());
        assertEquals(host, url.getAuthority());
        assertEquals(host, url.getHost());
        assertEquals(fragment, url.getRef());
    }

    // http://b/31858037
    public void testFragmentWithQuery() throws Exception {
        final String host = "example.com";
        final String fragment = "@not-a-host-name?a";
        URL url = new URL(String.format("http://%s#%s", host, fragment));
        assertNull(url.getUserInfo());
        assertEquals(host, url.getAuthority());
        assertEquals(host, url.getHost());
        assertEquals(fragment, url.getRef());
    }

    // http://b/33351987
    public void testMultipleUserField() throws Exception {
        final String host = "http://multiple@users@url.com";
        URL url = new URL(host);
        assertNull(url.getUserInfo());
        assertTrue(url.getHost().isEmpty());
    }
}
