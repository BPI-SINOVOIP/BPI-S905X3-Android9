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

package com.android.tradefed.util.net;

import static org.mockito.Mockito.doNothing;

import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.net.IHttpHelper.DataSizeException;

import junit.framework.TestCase;

import org.mockito.Mockito;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * Unit tests for {@link HttpHelper}.
 */
public class HttpHelperTest extends TestCase {
    private static final String TEST_URL_STRING = "http://foo";
    private static final String TEST_POST_DATA = "this is post data";
    private static final String TEST_DATA = "this is test data";
    private TestHttpHelper mHelper;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mHelper = new TestHttpHelper();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mHelper.close();
    }

    /**
     * Test {@link HttpHelper#buildParameters(MultiMap)}.
     */
    public void testBuildParams() {
        MultiMap<String, String> paramMap = new MultiMap<String, String>();
        paramMap.put("key", "value");
        assertEquals("key=value", mHelper.buildParameters(paramMap));

        paramMap.clear();
        paramMap.put("key1", "value1");
        paramMap.put("key2", "value2");
        String params = mHelper.buildParameters(paramMap);
        assertTrue(params.contains("key1=value1"));
        assertTrue(params.contains("key2=value2"));
        assertTrue(params.contains("&"));

        paramMap.clear();
        paramMap.put("key", "value1");
        paramMap.put("key", "value2");
        assertEquals("key=value1&key=value2", mHelper.buildParameters(paramMap));

        paramMap.clear();
        paramMap.put("key+f?o=o;", "value");
        assertEquals("key%2Bf%3Fo%3Do%3B=value", mHelper.buildParameters(paramMap));
    }

    /**
     * Test {@link HttpHelper#buildUrl(String, MultiMap)} with simple parameters.
     */
    public void testBuildUrl() {
        assertEquals("http://foo", mHelper.buildUrl(TEST_URL_STRING, null));

        MultiMap<String, String> paramMap = new MultiMap<String, String>();
        assertEquals("http://foo", mHelper.buildUrl(TEST_URL_STRING, paramMap));

        paramMap.put("key", "value");
        assertEquals("http://foo?key=value", mHelper.buildUrl(TEST_URL_STRING, paramMap));
    }

    /**
     * Normal case test for {@link HttpHelper#doGet(String)}
     */
    public void testDoGet() throws IOException, DataSizeException {
        assertEquals(TEST_DATA, mHelper.doGet(TEST_URL_STRING));
    }

    /**
     * Test that {@link HttpHelper#doGet(String)} throws {@link DataSizeException} when the
     * remote stream returns too much data.
     */
    public void testDoGet_datasize() throws IOException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            @Override
            InputStream getRemoteUrlStream(URL url) {
                // test with 64K + 1
                return new ByteArrayInputStream(new byte[IHttpHelper.MAX_DATA_SIZE + 1]);
            }
        };

        try {
            mHelper.doGet(TEST_URL_STRING);
            fail("DataSizeException not thrown");
        } catch (DataSizeException e) {
            // expected
        }
    }

    /**
     * Normal case test for {@link HttpHelper#doGet(String, OutputStream)}
     */
    public void testDoGetStream() throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();

        mHelper.doGet(TEST_URL_STRING, out);
        StreamUtil.flushAndCloseStream(out);

        assertEquals(TEST_DATA, out.toString());
    }

    /**
     * Normal case test for {@link HttpHelper#doGetWithRetry(String)}.
     */
    public void testDoGetWithRetry() throws IOException, DataSizeException {
        assertEquals(TEST_DATA, mHelper.doGetWithRetry(TEST_URL_STRING));
    }

    /**
     * Test that {@link HttpHelper#doGetWithRetry(String)} throws a {@link DataSizeException} when
     * the remote stream returns too much data.
     */
    public void testDoGetWithRetry_datasize() throws IOException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            @Override
            InputStream getRemoteUrlStream(URL url) {
                // test with 64K + 1
                return new ByteArrayInputStream(new byte[IHttpHelper.MAX_DATA_SIZE + 1]);
            }
        };

        try {
            mHelper.doGetWithRetry(TEST_URL_STRING);
            fail("DataSizeException not thrown");
        } catch (DataSizeException e) {
            // expected
        }
    }

    /**
     * Test that {@link HttpHelper#doGetWithRetry(String)} throws a {@link IOException} if an
     * {@link IOException} is thrown on each attempt.
     */
    public void testDoGetWithRetry_ioexception() throws DataSizeException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            @Override
            public String doGet(String url) throws IOException {
                throw new IOException();
            }
        };

        try {
            mHelper.doGetWithRetry(TEST_URL_STRING);
            fail("IOException not thrown");
        } catch (IOException e) {
            // expected
        }
    }

    /**
     * Test that {@link HttpHelper#doGetWithRetry(String)} returns data if an {@link IOException} is
     * thrown on the first attempt, but is fine on the second attempt.
     */
    public void testDoGetWithRetry_retry() throws IOException, DataSizeException {
        mHelper.close();
        RunUtil mockRunUtil = Mockito.spy(RunUtil.class);
        mHelper =
                new TestHttpHelper() {
                    boolean mExceptionThrown = false;

                    @Override
                    public IRunUtil getRunUtil() {
                        return mockRunUtil;
                    }

                    @Override
                    public String doGet(String url) throws IOException, DataSizeException {
                        if (!mExceptionThrown) {
                            mExceptionThrown = true;
                            throw new IOException();
                        }
                        return super.doGet(url);
                    }
                };
        mHelper.setMaxTime(5000);
        // Avoid doing actual sleep in the retry
        doNothing().when(mockRunUtil).sleep(Mockito.anyLong());
        assertEquals(TEST_DATA, mHelper.doGetWithRetry(TEST_URL_STRING));
    }

    /**
     * Normal case test for {@link HttpHelper#doPostWithRetry(String, String)}.
     */
    public void testDoPostWithRetry() throws IOException, DataSizeException {
        assertEquals(TEST_DATA, mHelper.doPostWithRetry(TEST_URL_STRING, TEST_POST_DATA));
        assertEquals(TEST_POST_DATA, mHelper.getOutputStream().toString());
    }

    /**
     * Test that {@link HttpHelper#doPostWithRetry(String, String)} throws a
     * {@link DataSizeException} when the remote stream returns too much data.
     */
    public void testDoPostWithRetry_datasize() throws IOException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            @Override
            InputStream getConnectionInputStream(HttpURLConnection conn) {
                // test with 64K + 1
                return new ByteArrayInputStream(new byte[IHttpHelper.MAX_DATA_SIZE + 1]);
            }
        };

        try {
            mHelper.doPostWithRetry(TEST_URL_STRING, TEST_POST_DATA);
            fail("DataSizeException not thrown");
        } catch (DataSizeException e) {
            // expected
        }
    }

    /**
     * Test that {@link HttpHelper#doPostWithRetry(String, String)} throws a {@link IOException} if
     * an {@link IOException} is thrown on each attempt.
     */
    public void testDoPostWithRetry_ioexception() throws DataSizeException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            @Override
            public HttpURLConnection createConnection(URL url, String method, String contentType)
                    throws IOException {
                throw new IOException();
            }
        };

        try {
            mHelper.doPostWithRetry(TEST_URL_STRING, TEST_POST_DATA);
            fail("IOException not thrown");
        } catch (IOException e) {
            // expected
        }
    }

    /**
     * Test that {@link HttpHelper#doPostWithRetry(String, String)} returns data if an
     * {@link IOException} is thrown on the first attempt, but is fine on the second attempt.
     */
    public void testDoPostWithRetry_retry() throws IOException, DataSizeException {
        mHelper.close();
        mHelper = new TestHttpHelper() {
            boolean mExceptionThrown = false;

            @Override
            public HttpURLConnection createConnection(URL url, String method, String contentType)
                    throws IOException {
                if (!mExceptionThrown) {
                    mExceptionThrown = true;
                    throw new IOException();
                }
                return super.createConnection(url, method, contentType);
            }
        };

        assertEquals(TEST_DATA, mHelper.doPostWithRetry(TEST_URL_STRING, TEST_POST_DATA));
        assertEquals(TEST_POST_DATA, mHelper.getOutputStream().toString());
    }

    /**
     * A class which extends {@link HttpHelper} for testing without using the network.
     */
    private class TestHttpHelper extends HttpHelper {
        InputStream mInputStream = new ByteArrayInputStream(TEST_DATA.getBytes());
        OutputStream mOutputStream = new ByteArrayOutputStream();

        /**
         * Create a {@link TestHttpHelper}
         */
        public TestHttpHelper() {
            // Override all the polling related values to make this unit test run as fast as
            // possible while still delivering consistent results.
            setOpTimeout(300);
            setInitialPollInterval(10);
            setMaxPollInterval(50);
            setMaxTime(50);
        }

        /**
         * Close any open streams.
         */
        public void close() {
            StreamUtil.close(mInputStream);
            StreamUtil.close(mOutputStream);
        }

        /**
         * Get the output stream used in {@link #doPostWithRetry(String, String)}.
         */
        public OutputStream getOutputStream() {
            return mOutputStream;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        InputStream getRemoteUrlStream(URL url) {
            return mInputStream;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public HttpURLConnection createConnection(URL url, String method, String contentType)
                throws IOException {
            return null;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        InputStream getConnectionInputStream(HttpURLConnection conn) throws IOException {
            return mInputStream;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        OutputStream getConnectionOutputStream(HttpURLConnection conn) throws IOException {
            return mOutputStream;
        }
    }
}
