/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.net.IHttpHelper.DataSizeException;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Helper class for making multipart HTTP post requests. This class is used to upload files
 * using multipart HTTP post (RFC 2388).
 *
 * To send multipart posts create this object passing it the url to send the requests to.
 * Then set necessary parameters using the addParameter method and specify a file to upload
 * using addFile method. After everything is set, send the request using the send method.
 *
 * Currently the implementation only supports 'text/plain' content types.
 */
public class HttpMultipartPost {

    private static final String CONTENT_TYPE = "text/plain";
    private static final String BOUNDARY = "xXxXx";
    private static final String HYPHENS = "--";
    private static final String CRLF = "\r\n";

    private StringBuilder mBuilder;
    private String mUrl;
    private IHttpHelper mHelper;

    public HttpMultipartPost(String url, IHttpHelper httpHelper) {
        mBuilder = new StringBuilder();
        mUrl = url;
        mHelper = httpHelper;
    }

    public HttpMultipartPost(String url) {
        this(url, new HttpHelper());
    }

    /**
     * Adds a string parameter to the request.
     * @param name name of the parameter.
     * @param value value of the parameter.
     * @throws IOException
     */
    public void addParameter(String name, String value) throws IOException {
        mBuilder.append(HYPHENS + BOUNDARY + CRLF);
        mBuilder.append(String.format(
                "Content-Disposition: form-data; name=\"%s\"%s%s", name, CRLF,
                CRLF));
        mBuilder.append(value);
        mBuilder.append(CRLF);
    }

    /**
     * Add a file parameter to the request. Opens the file, reads its contents
     * and sends them as part of the request. Currently the implementation
     * only supports 'text/plain' content type.
     * @param name name of the parameter.
     * @param file file whose contents will be uploaded as part of the request.
     * @throws IOException
     */
    public void addTextFile(String name, File file) throws IOException {
        FileInputStream in = new FileInputStream(file);
        String fileName = file.getAbsolutePath();

        addTextFile(name, fileName, in);

        in.close();
    }

    /**
     * Add a file parameter to the request. The contents of the file to upload
     * will come from reading the input stream. Currently the implementation only
     * supports 'text/plain' content type.
     * @param name name of the parameter.
     * @param fileName file name to report for the data in the stream.
     * @param in stream whose contents are being uploaded.
     * @throws IOException
     */
    public void addTextFile(String name, String fileName, InputStream in)
            throws IOException {

        mBuilder.append(HYPHENS + BOUNDARY + CRLF);
        mBuilder.append(String.format(
                "Content-Disposition: form-data; name=\"%s\";filename=\"%s\"%s",
                name, fileName, CRLF));
        mBuilder.append(String.format("Content-Type: %s%s", CONTENT_TYPE, CRLF));
        mBuilder.append(CRLF);

        mBuilder.append(StreamUtil.getStringFromStream(in));

        mBuilder.append(CRLF);
    }

    /**
     * Sends the request to the server.
     * @throws IOException
     * @throws DataSizeException
     */
    public void send() throws IOException, DataSizeException {
        mBuilder.append(HYPHENS + BOUNDARY + HYPHENS);
        mBuilder.append(CRLF + CRLF);
        mHelper.doPostWithRetry(mUrl, mBuilder.toString(),
                "multipart/form-data;boundary=" + BOUNDARY);
    }
}
