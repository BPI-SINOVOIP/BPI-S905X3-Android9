/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License"); you
 * may not use this file except in compliance with the License. You may obtain a
 * copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.tradefed.util.xml;

import com.android.tradefed.log.LogUtil.CLog;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.File;
import java.io.IOException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

/**
 * Helper class for modifying an AndroidManifest.
 * <p/>
 * copied from
 * <android source>/platform/sdk/.../adt-tests/com.android.ide.eclipse.adt.internal.project.AndroidManifestHelper.
 * TODO: Find a way of sharing this code with adt-tests
 */
public class AndroidManifestWriter {

    private final Document mDoc;
    private final String mOsManifestFilePath;

    private static final String NODE_USES_SDK = "uses-sdk";
    private static final String ATTRIBUTE_MIN_SDK_VERSION = "minSdkVersion";
    /** Namespace for the resource XML, i.e. "http://schemas.android.com/apk/res/android" */
    private final static String NS_RESOURCES = "http://schemas.android.com/apk/res/android";

    private AndroidManifestWriter(Document doc, String osManifestFilePath) {
        mDoc = doc;
        mOsManifestFilePath = osManifestFilePath;
    }

    /**
     * Sets the minimum SDK version for this manifest.
     *
     * @param minSdkVersion - the minimim sdk version to use
     * @return <code>true</code> on success, false otherwise
     */
    public boolean setMinSdkVersion(String minSdkVersion) {
        Element usesSdkElement = null;
        NodeList nodeList = mDoc.getElementsByTagName(NODE_USES_SDK);
        if (nodeList.getLength() > 0) {
            usesSdkElement = (Element) nodeList.item(0);
        } else {
            usesSdkElement = mDoc.createElement(NODE_USES_SDK);
            mDoc.getDocumentElement().appendChild(usesSdkElement);
        }
        Attr minSdkAttr = mDoc.createAttributeNS(NS_RESOURCES, ATTRIBUTE_MIN_SDK_VERSION);
        String prefix = mDoc.lookupPrefix(NS_RESOURCES);
        minSdkAttr.setPrefix(prefix);
        minSdkAttr.setValue(minSdkVersion);
        usesSdkElement.setAttributeNodeNS(minSdkAttr);
        return saveXmlToFile();
    }

    private boolean saveXmlToFile() {
        try {
            // Prepare the DOM document for writing
            Source source = new DOMSource(mDoc);

            // Prepare the output file
            File file = new File(mOsManifestFilePath);
            Result result = new StreamResult(file);

            // Write the DOM document to the file
            Transformer xformer = TransformerFactory.newInstance().newTransformer();
            xformer.transform(source, result);
        } catch (TransformerConfigurationException e) {
            CLog.e("Failed to write xml file %s", mOsManifestFilePath);
            CLog.e(e);
            return false;
        } catch (TransformerException e) {
            CLog.e("Failed to write xml file %s", mOsManifestFilePath);
            CLog.e(e);
            return false;
        }
        return true;
    }

    /**
     * Parses the manifest file, and collects data.
     *
     * @param osManifestFilePath The OS path of the manifest file to parse.
     * @return an {@link AndroidManifestWriter} or null if parsing failed
     */
    public static AndroidManifestWriter parse(String osManifestFilePath) {
        try {
            DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
            docFactory.setNamespaceAware(true);
            DocumentBuilder docBuilder = docFactory.newDocumentBuilder();
            docBuilder.setErrorHandler(new DefaultHandler());
            Document doc = docBuilder.parse(osManifestFilePath);
            return new AndroidManifestWriter(doc, osManifestFilePath);
        } catch (ParserConfigurationException e) {
            CLog.e("Error parsing file %s", osManifestFilePath);
            CLog.e(e);
            return null;
        } catch (SAXException e) {
            CLog.e("Error parsing file %s", osManifestFilePath);
            CLog.e(e);
            return null;
        } catch (IOException e) {
            CLog.e("Error parsing file %s", osManifestFilePath);
            CLog.e(e);
            return null;
        }
    }
}
