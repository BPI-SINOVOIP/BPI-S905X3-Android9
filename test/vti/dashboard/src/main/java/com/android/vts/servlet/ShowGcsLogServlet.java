/*
 * Copyright (c) 2018 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.servlet;

import com.google.auth.oauth2.ServiceAccountCredentials;
import com.google.cloud.storage.Blob;
import com.google.cloud.storage.Bucket;
import com.google.cloud.storage.Storage;
import com.google.cloud.storage.Storage.BlobListOption;
import com.google.cloud.storage.StorageOptions;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * A GCS log servlet read log zip file from Google Cloud Storage bucket and show the content in it
 * from the zip file by unarchiving it
 */
@SuppressWarnings("serial")
public class ShowGcsLogServlet extends BaseServlet {

    private static final String GCS_LOG_JSP = "WEB-INF/jsp/show_gcs_log.jsp";

    private static final String GCS_PROJECT_ID = System.getProperty("GCS_PROJECT_ID");
    private static final String GCS_KEY_FILE = System.getProperty("GCS_KEY_FILE");
    private static final String GCS_BUCKET_NAME = System.getProperty("GCS_BUCKET_NAME");

    /**
     * This is the key file to access vtslab-gcs project. It will allow the dashboard to have a full
     * controll of the bucket.
     */
    private InputStream keyFileInputStream;

    /** This is the instance of java google storage library */
    private Storage storage;

    @Override
    public void init(ServletConfig cfg) throws ServletException {
        super.init(cfg);
        keyFileInputStream =
                this.getServletContext().getResourceAsStream("/WEB-INF/keys/" + GCS_KEY_FILE);

        try {
            storage =
                    StorageOptions.newBuilder()
                            .setProjectId(GCS_PROJECT_ID)
                            .setCredentials(
                                    ServiceAccountCredentials.fromStream(keyFileInputStream))
                            .build()
                            .getService();
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error on creating storage instance!");
        }
    }

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        return null;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {

        String path = request.getParameter("path") == null ? "" : request.getParameter("path");
        Path pathInfo = Paths.get(path);

        Bucket vtsReportBucket = storage.get(GCS_BUCKET_NAME);

        List<String> dirList = new ArrayList<>();
        List<String> fileList = new ArrayList<>();
        if (pathInfo.endsWith(".zip")) {
            Blob blobFile = vtsReportBucket.get(path);
        } else {

            logger.log(Level.INFO, "path info => " + pathInfo);
            logger.log(Level.INFO, "path name count => " + pathInfo.getNameCount());

            BlobListOption[] listOptions;
            if (pathInfo.getNameCount() == 0) {
                listOptions = new BlobListOption[] {BlobListOption.currentDirectory()};
            } else {
                if (pathInfo.getNameCount() <= 1) {
                    dirList.add("/");
                } else {
                    dirList.add(pathInfo.getParent().toString());
                }
                listOptions =
                        new BlobListOption[] {
                            BlobListOption.currentDirectory(),
                            BlobListOption.prefix(pathInfo.toString() + "/")
                        };
            }

            Iterator<Blob> blobIterator = vtsReportBucket.list(listOptions).iterateAll();
            while (blobIterator.hasNext()) {
                Blob blob = blobIterator.next();
                logger.log(Level.INFO, "blob name => " + blob);
                if (blob.isDirectory()) {
                    logger.log(Level.INFO, "directory name => " + blob.getName());
                    dirList.add(blob.getName());
                } else {
                    logger.log(Level.INFO, "file name => " + blob.getName());
                    fileList.add(blob.getName());
                }
            }
        }

        response.setStatus(HttpServletResponse.SC_OK);
        request.setAttribute("dirList", dirList);
        request.setAttribute("fileList", fileList);
        request.setAttribute("path", path);
        RequestDispatcher dispatcher = request.getRequestDispatcher(GCS_LOG_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
