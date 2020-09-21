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

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.IRunnableResult;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.VersionParser;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;

/**
 * Contains helper methods for making http requests
 */
public class HttpHelper implements IHttpHelper {
    // Note: max int timeout, expressed in millis, is 24 days
    /** Time before timing out a request in ms. */
    private int mQueryTimeout = 1 * 60 * 1000;
    /** Initial poll interval in ms. */
    private int mInitialPollInterval = 1 * 1000;
    /** Max poll interval in ms. */
    private int mMaxPollInterval = 10 * 60 * 1000;
    /** Max time for retrying request in ms. */
    private int mMaxTime = 10 * 60 * 1000;
    /** Max number of redirects to follow */
    private int mMaxRedirects = 5;

    private final static String USER_AGENT = "TradeFederation";

    /**
     * {@inheritDoc}
     */
    @Override
    public String buildUrl(String baseUrl, MultiMap<String, String> paramMap) {
        StringBuilder urlBuilder = new StringBuilder(baseUrl);
        if (paramMap != null && !paramMap.isEmpty()) {
            urlBuilder.append("?");
            urlBuilder.append(buildParameters(paramMap));
        }
        return urlBuilder.toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String buildParameters(MultiMap<String, String> paramMap) {
        StringBuilder urlBuilder = new StringBuilder("");
        boolean first = true;
        for (String key : paramMap.keySet()) {
            for (String value : paramMap.get(key)) {
                if (!first) {
                    urlBuilder.append("&");
                } else {
                    first = false;
                }
                try {
                    urlBuilder.append(URLEncoder.encode(key, "UTF-8"));
                    urlBuilder.append("=");
                    urlBuilder.append(URLEncoder.encode(value, "UTF-8"));
                } catch (UnsupportedEncodingException e) {
                    throw new IllegalArgumentException(e);
                }
            }
        }

        return urlBuilder.toString();
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("resource")
    @Override
    public String doGet(String url) throws IOException, DataSizeException {
        CLog.d("Performing GET request for %s", url);
        InputStream remote = null;
        byte[] bufResult = new byte[MAX_DATA_SIZE];
        int currBufPos = 0;

        try {
            remote = getRemoteUrlStream(new URL(url));
            int bytesRead;
            // read data from stream into temporary buffer
            while ((bytesRead = remote.read(bufResult, currBufPos,
                    bufResult.length - currBufPos)) != -1) {
                currBufPos += bytesRead;
                if (currBufPos >= bufResult.length) {
                    // Eclipse compiler incorrectly flags this statement as not 'remote
                    // is not closed at this location'.
                    // So add @SuppressWarnings('resource') to shut it up.
                    throw new DataSizeException();
                }
            }

            return new String(bufResult, 0, currBufPos);
        } finally {
            StreamUtil.close(remote);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void doGet(String url, OutputStream outputStream) throws IOException {
        CLog.d("Performing GET download request for %s", url);
        InputStream remote = null;
        try {
            remote = getRemoteUrlStream(new URL(url));
            StreamUtil.copyStreams(remote, outputStream);
        } finally {
            StreamUtil.close(remote);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void doGetIgnore(String url) throws IOException {
        CLog.d("Performing GET request for %s. Ignoring result.", url);
        InputStream remote = null;
        try {
            remote = getRemoteUrlStream(new URL(url));
        } finally {
            StreamUtil.close(remote);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public HttpURLConnection createConnection(URL url, String method, String contentType)
            throws IOException {
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod(method);
        if (contentType != null) {
            connection.setRequestProperty("Content-Type", contentType);
        }
        connection.setDoInput(true);
        connection.setDoOutput(true);
        connection.setConnectTimeout(getOpTimeout());  // timeout for establishing the connection
        connection.setReadTimeout(getOpTimeout());  // timeout for receiving a read() response
        connection.setRequestProperty("User-Agent",
                String.format("%s/%s", USER_AGENT, VersionParser.fetchVersion()));
        return connection;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public HttpURLConnection createXmlConnection(URL url, String method) throws IOException {
        return createConnection(url, method, "text/xml");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public HttpURLConnection createJsonConnection(URL url, String method) throws IOException {
        return createConnection(url, method, "application/json");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String doGetWithRetry(String url) throws IOException, DataSizeException {
        GetRequestRunnable runnable = new GetRequestRunnable(url, false);
        if (getRunUtil().runEscalatingTimedRetry(getOpTimeout(), getInitialPollInterval(),
                getMaxPollInterval(), getMaxTime(), runnable)) {
            return runnable.getResponse();
        } else if (runnable.getException() instanceof IOException) {
            throw (IOException) runnable.getException();
        } else if (runnable.getException() instanceof DataSizeException) {
            throw (DataSizeException) runnable.getException();
        } else if (runnable.getException() instanceof RuntimeException) {
            throw (RuntimeException) runnable.getException();
        } else {
            throw new IOException("GET request could not be completed");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void doGetIgnoreWithRetry(String url) throws IOException {
        GetRequestRunnable runnable = new GetRequestRunnable(url, true);
        if (getRunUtil().runEscalatingTimedRetry(getOpTimeout(), getInitialPollInterval(),
                getMaxPollInterval(), getMaxTime(), runnable)) {
            return;
        } else if (runnable.getException() instanceof IOException) {
            throw (IOException) runnable.getException();
        } else if (runnable.getException() instanceof RuntimeException) {
            throw (RuntimeException) runnable.getException();
        } else {
            throw new IOException("GET request could not be completed");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String doPostWithRetry(String url, String postData, String contentType)
            throws IOException, DataSizeException {
        PostRequestRunnable runnable = new PostRequestRunnable(url, postData, contentType);
        if (getRunUtil().runEscalatingTimedRetry(getOpTimeout(), getInitialPollInterval(),
                getMaxPollInterval(), getMaxTime(), runnable)) {
            return runnable.getResponse();
        } else if (runnable.getException() instanceof IOException) {
            throw (IOException) runnable.getException();
        } else if (runnable.getException() instanceof DataSizeException) {
            throw (DataSizeException) runnable.getException();
        } else if (runnable.getException() instanceof RuntimeException) {
            throw (RuntimeException) runnable.getException();
        } else {
            throw new IOException("POST request could not be completed");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String doPostWithRetry(String url, String postData) throws IOException,
            DataSizeException {
        return doPostWithRetry(url, postData, null);
    }

    /**
     * Runnable for making requests with
     * {@link IRunUtil#runEscalatingTimedRetry(long, long, long, long, IRunnableResult)}.
     */
    public abstract class RequestRunnable implements IRunnableResult {
        private String mResponse = null;
        private Exception mException = null;
        private final String mUrl;

        public RequestRunnable(String url) {
            mUrl = url;
        }

        public String getUrl() {
            return mUrl;
        }

        public String getResponse() {
            return mResponse;
        }

        protected void setResponse(String response) {
            mResponse = response;
        }

        /**
         * Returns the last {@link Exception} that occurred when performing run().
         */
        public Exception getException() {
            return mException;
        }

        protected void setException(Exception e) {
            mException = e;
        }

        @Override
        public void cancel() {
            // ignore
        }
    }

    /**
     * Runnable for making GET requests with
     * {@link IRunUtil#runEscalatingTimedRetry(long, long, long, long, IRunnableResult)}.
     */
    private class GetRequestRunnable extends RequestRunnable {
        private boolean mIgnoreResult;

        public GetRequestRunnable(String url, boolean ignoreResult) {
            super(url);
            mIgnoreResult = ignoreResult;
        }

        /**
         * Perform a single GET request, storing the response or the associated exception in case of
         * error.
         */
        @Override
        public boolean run() {
            try {
                if (mIgnoreResult) {
                    doGetIgnore(getUrl());
                } else {
                    setResponse(doGet(getUrl()));
                }
                return true;
            } catch (IOException e) {
                CLog.i("IOException %s from %s", e.getMessage(), getUrl());
                setException(e);
            } catch (DataSizeException e) {
                CLog.i("Unexpected oversized response from %s", getUrl());
                setException(e);
            } catch (RuntimeException e) {
                CLog.i("RuntimeException %s", e.getMessage());
                setException(e);
            }

            return false;
        }
    }

    /**
     * Runnable for making POST requests with
     * {@link IRunUtil#runEscalatingTimedRetry(long, long, long, long, IRunnableResult)}.
     */
    private class PostRequestRunnable extends RequestRunnable {
        String mPostData;
        String mContentType;
        public PostRequestRunnable(String url, String postData, String contentType) {
            super(url);
            mPostData = postData;
            mContentType = contentType;
        }

        /**
         * Perform a single POST request, storing the response or the associated exception in case
         * of error.
         */
        @SuppressWarnings("resource")
        @Override
        public boolean run() {
            InputStream inputStream = null;
            OutputStream outputStream = null;
            OutputStreamWriter outputStreamWriter = null;
            try {
                HttpURLConnection conn = createConnection(new URL(getUrl()), "POST", mContentType);

                outputStream = getConnectionOutputStream(conn);
                outputStreamWriter = new OutputStreamWriter(outputStream);
                outputStreamWriter.write(mPostData);
                outputStreamWriter.flush();

                inputStream = getConnectionInputStream(conn);
                byte[] bufResult = new byte[MAX_DATA_SIZE];
                int currBufPos = 0;
                int bytesRead;
                // read data from stream into temporary buffer
                while ((bytesRead = inputStream.read(bufResult, currBufPos,
                        bufResult.length - currBufPos)) != -1) {
                    currBufPos += bytesRead;
                    if (currBufPos >= bufResult.length) {
                        // Eclipse compiler incorrectly flags this statement as not 'stream
                        // is not closed at this location'.
                        // So add @SuppressWarnings('resource') to shut it up.
                        throw new DataSizeException();
                    }
                }
                setResponse(new String(bufResult, 0, currBufPos));
                return true;
            } catch (IOException e) {
                CLog.i("IOException %s from %s", e.getMessage(), getUrl());
                setException(e);
            } catch (DataSizeException e) {
                CLog.i("Unexpected oversized response from %s", getUrl());
                setException(e);
            } catch (RuntimeException e) {
                CLog.i("RuntimeException %s", e.getMessage());
                setException(e);
            } finally {
                StreamUtil.close(outputStream);
                StreamUtil.close(inputStream);
                StreamUtil.close(outputStreamWriter);
            }

            return false;
        }
    }

    /**
     * Factory method for opening an input stream to a remote url. Exposed for unit testing.
     *
     * @param url the {@link URL}
     * @return the {@link InputStream}
     * @throws IOException if stream could not be opened.
     */
    InputStream getRemoteUrlStream(URL url) throws IOException {
        // Redirects are handle by HttpURLConnection, except when the protocol changes.
        // e.g.: http to https, and vice versa.
        boolean redirect;
        int redirectCount = 0;
        HttpURLConnection conn = createConnection(url, "GET", null);
        do {
            redirect = false;
            int status = conn.getResponseCode();
            if (status != HttpURLConnection.HTTP_OK) {
                if (status == HttpURLConnection.HTTP_MOVED_PERM
                        || status == HttpURLConnection.HTTP_MOVED_TEMP
                        || status == HttpURLConnection.HTTP_SEE_OTHER) {
                    redirect = true;
                }
            }
            if (redirect) {
                String location = conn.getHeaderField("Location");
                URL newURL = new URL(location);
                CLog.d("Redirect occured during GET, new url %s", location);
                conn = createConnection(newURL, "GET", null);
            }
        } while(redirect && redirectCount < mMaxRedirects);
        return conn.getInputStream();
    }

    /**
     * Factory method for getting connection input stream. Exposed for unit testing.
     */
    InputStream getConnectionInputStream(HttpURLConnection conn) throws IOException {
        return conn.getInputStream();
    }

    /**
     * Factory method for getting connection output stream. Exposed for unit testing.
     */
    OutputStream getConnectionOutputStream(HttpURLConnection conn) throws IOException {
        return conn.getOutputStream();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getOpTimeout() {
        return mQueryTimeout;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setOpTimeout(int time) {
        mQueryTimeout = time;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getInitialPollInterval() {
        return mInitialPollInterval;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setInitialPollInterval(int time) {
        mInitialPollInterval = time;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getMaxPollInterval() {
        return mMaxPollInterval;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMaxPollInterval(int time) {
        mMaxPollInterval = time;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getMaxTime() {
        return mMaxTime;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMaxTime(int time) {
        mMaxTime = time;
    }

    /**
     * Get {@link IRunUtil} to use. Exposed so unit tests can mock.
     */
    public IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
