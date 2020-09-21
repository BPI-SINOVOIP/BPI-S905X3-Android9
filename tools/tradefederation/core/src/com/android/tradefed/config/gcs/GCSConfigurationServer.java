/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.config.gcs;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfigurationServer;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.util.GCSFileDownloader;

import com.google.common.annotations.VisibleForTesting;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Scanner;

/** Config server loads configs from Google Cloud Storage (GCS). */
@OptionClass(alias = "gcs-config-server", global_namespace = false)
public class GCSConfigurationServer implements IConfigurationServer {

    private static final String DEFAULT_MAPPING = "host-config.txt";

    @Option(
        name = "bucket-name",
        description = "Google Cloud Storage bucket name.",
        mandatory = true
    )
    private String mBucketName;

    @Option(
        name = "host-config-mapping-filename",
        description = "Mapping file maps hostname to its config."
    )
    private String mHostConfigMapping = DEFAULT_MAPPING;

    private GCSFileDownloader mGCSFileDownloader = null;
    private String mCurrentHostConfig = null;

    /** {@inheritDoc} */
    @Override
    public String getCurrentHostConfig() throws ConfigurationException {
        if (mCurrentHostConfig == null) {
            String currentHostname = currentHostname();
            System.out.format("Downloading %s.\n", mHostConfigMapping);
            InputStream hostConfigMapping = downloadFile(mHostConfigMapping);
            mCurrentHostConfig = getHostConfig(currentHostname, hostConfigMapping);
        }
        return mCurrentHostConfig;
    }

    /**
     * Get a host's config file name from the mapping file. Each line in the host config mapping
     * file has the format: "hostname,instance-name,host-config-file-name,host-command-file".
     *
     * @param hostname is the hostname to get host config for
     * @param hostConfigMappingFile is a mapping file that maps hostname to host config name.
     * @return host config name is the host's config name.
     * @throws ConfigurationException
     */
    private String getHostConfig(String hostname, InputStream hostConfigMappingFile)
            throws ConfigurationException {
        Scanner scanner = new Scanner(hostConfigMappingFile);
        try {
            while (scanner.hasNextLine()) {
                // "hostname,instance-name,host-config-file-name,host-command-file"
                String line = scanner.nextLine().trim();
                if (line.startsWith("#") || line.isEmpty() || line.startsWith("[")) {
                    // Ignore comments, empty line and cluster name line.
                    continue;
                }
                String[] hostMap = line.split(",");
                if (hostMap.length < 3) {
                    // Ignore invalid lines.
                    continue;
                }
                if (sameHost(hostname, hostMap[0])) {
                    System.out.format("%s's config is %s\n", hostname, hostMap[2]);
                    return hostMap[2];
                }
            }
            throw new ConfigurationException(
                    String.format("Host %s doesn't have configure.", hostname));
        } finally {
            scanner.close();
        }
    }

    private boolean sameHost(String currentHostname, String hostname) {
        return currentHostname.startsWith(hostname);
    }

    /**
     * Get current host name.
     *
     * @return hostname
     * @throws ConfigurationException
     */
    @VisibleForTesting
    String currentHostname() throws ConfigurationException {
        try {
            return InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            throw new ConfigurationException(e.getMessage(), e.getCause());
        }
    }

    /** {@inheritDoc} */
    @Override
    public InputStream getConfig(String name) throws ConfigurationException {
        System.out.format("Downloading %s.\n", name);
        return downloadFile(name);
    }

    /**
     * Download file from GCS
     *
     * @param name file name
     * @return an {@link InputStream} is the file's content.
     * @throws ConfigurationException
     */
    @VisibleForTesting
    InputStream downloadFile(String name) throws ConfigurationException {
        try {
            return getFileDownloader().downloadFile(mBucketName, name);
        } catch (IOException e) {
            throw new ConfigurationException(e.getMessage(), e.getCause());
        }
    }

    private GCSFileDownloader getFileDownloader() {
        if (mGCSFileDownloader == null) {
            mGCSFileDownloader = new GCSFileDownloader();
        }
        return mGCSFileDownloader;
    }
}
