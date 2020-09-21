/**
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

/** UrlUtil, a helper class for formatting and validating URLs. */
public class UrlUtil {
    private static final String HTTPS = "https";

    public static class LinkDisplay {
        public final String name;
        public final String url;

        /**
         * Create a link display object.
         *
         * @param uri The hyperlink URI.
         */
        public LinkDisplay(URI uri) {
            this.url = uri.toString();

            // Parse the name from the URI path
            int lastSeparator = uri.getPath().lastIndexOf('/');
            if (lastSeparator < 0) {
                this.name = uri.toString();
            } else {
                this.name = uri.getPath().substring(lastSeparator + 1);
            }
        }
    }

    /**
     * Validates and formats a URL.
     *
     * <p>Ensures that the protocol is HTTPs and the URL is properly formatted. This avoids link
     * issues in the UI and possible vulnerabilities due to embedded JS in URLs.
     *
     * @param urlString The url string to validate.
     * @returns The validated LinkDisplay object.
     */
    public static LinkDisplay processUrl(String urlString) {
        try {
            URL url = new URL(urlString);
            String scheme = url.getProtocol();
            String userInfo = url.getUserInfo();
            String host = url.getHost();
            int port = url.getPort();
            String path = url.getPath();
            String query = url.getQuery();
            String fragment = url.getRef();
            if (!url.getProtocol().equals(HTTPS))
                throw new MalformedURLException();
            URI uri = new URI(scheme, userInfo, host, port, path, query, fragment);
            return new LinkDisplay(uri);
        } catch (MalformedURLException | URISyntaxException e) {
            return null;
        }
    }
}
