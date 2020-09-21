/*
 * Copyright (C) 2008 The Android Open Source Project
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
package android.content.res.cts;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.fail;

import android.content.Context;
import android.content.cts.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.cts.util.XmlUtils;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.TypedValue;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.HashSet;

@RunWith(AndroidJUnit4.class)
public class AssetManagerTest {
    private AssetManager mAssets;
    private Context mContext;

    private Context getContext() {
        return mContext;
    }

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getContext();
        mAssets = mContext.getAssets();
    }

    @SmallTest
    @Test
    public void testAssetOperations() throws Exception {
        final String fileName = "text.txt";
        final String expect = "OneTwoThreeFourFiveSixSevenEightNineTen";

        final TypedValue value = new TypedValue();
        final Resources res = getContext().getResources();
        res.getValue(R.raw.text, value, true);

        InputStream inputStream = mAssets.open(fileName);
        assertThat(inputStream).isNotNull();
        assertContextEquals(expect, inputStream);

        inputStream = mAssets.open(fileName, AssetManager.ACCESS_BUFFER);
        assertThat(inputStream).isNotNull();
        assertContextEquals(expect, inputStream);

        AssetFileDescriptor assetFileDes = mAssets.openFd(fileName);
        assertThat(assetFileDes).isNotNull();
        assertContextEquals(expect, assetFileDes.createInputStream());

        assetFileDes = mAssets.openNonAssetFd(value.string.toString());
        assertThat(assetFileDes).isNotNull();
        assertContextEquals(expect, assetFileDes.createInputStream());

        assetFileDes = mAssets.openNonAssetFd(value.assetCookie, value.string.toString());
        assertThat(assetFileDes).isNotNull();
        assertContextEquals(expect, assetFileDes.createInputStream());

        XmlResourceParser parser = mAssets.openXmlResourceParser("AndroidManifest.xml");
        assertThat(parser).isNotNull();
        XmlUtils.beginDocument(parser, "manifest");
        parser = mAssets.openXmlResourceParser(0, "AndroidManifest.xml");
        assertThat(parser).isNotNull();
        beginDocument(parser, "manifest");

        String[] files = mAssets.list("");
        assertThat(files).isNotNull();

        // We don't do an exact match because the framework can add asset files and this test
        // would be too brittle.
        assertThat(files).asList().containsAllOf(fileName, "subdir");

        files = mAssets.list("subdir");
        assertThat(files).isNotNull();
        assertThat(files).asList().contains("subdir_text.txt");

        // This directory doesn't exist.
        assertThat(mAssets.list("__foo__bar__dir__")).asList().isEmpty();

        try {
            mAssets.open("notExistFile.txt", AssetManager.ACCESS_BUFFER);
            fail("test open(String, int) failed");
        } catch (IOException e) {
            // expected
        }

        try {
            mAssets.openFd("notExistFile.txt");
            fail("test openFd(String) failed");
        } catch (IOException e) {
            // expected
        }

        try {
            mAssets.openNonAssetFd(0, "notExistFile.txt");
            fail("test openNonAssetFd(int, String) failed");
        } catch (IOException e) {
            // expected
        }

        try {
            mAssets.openXmlResourceParser(0, "notExistFile.txt");
            fail("test openXmlResourceParser(int, String) failed");
        } catch (IOException e) {
            // expected
        }

        assertThat(mAssets.getLocales()).isNotNull();
    }

    @SmallTest
    @Test
    public void testClose() throws Exception {
        final AssetManager assets = new AssetManager();
        assets.close();

        // Should no-op.
        assets.close();

        try {
            assets.openXmlResourceParser("AndroidManifest.xml");
            fail("Expected RuntimeException");
        } catch (RuntimeException e) {
            // Expected.
        }
    }

    @SmallTest
    @Test
    public void testGetNonSystemLocales() {
        // This is the list of locales built into this test package. It is basically the locales
        // specified in the Android.mk files (assuming they have corresponding resources), plus the
        // special cases for Filipino.
        final String KNOWN_LOCALES[] = {
            "cs",
            "fil",
            "fil-PH",
            "fil-SA",
            "fr",
            "fr-FR",
            "iw",
            "iw-IL",
            "kok",
            "kok-419",
            "kok-419-variant",
            "kok-IN",
            "kok-Knda",
            "kok-Knda-419",
            "kok-Knda-419-variant",
            "kok-variant",
            "mk",
            "mk-MK",
            "tgl",
            "tgl-PH",
            "tlh",
            "xx",
            "xx-YY"
        };

        final String PSEUDO_OR_EMPTY_LOCALES[] = {
            "",
            "en-XA",
            "ar-XB"
        };

        String locales[] = mAssets.getNonSystemLocales();
        HashSet<String> localesSet = new HashSet<String>(Arrays.asList(locales));
        localesSet.removeAll(Arrays.asList(PSEUDO_OR_EMPTY_LOCALES));
        assertThat(localesSet).containsExactlyElementsIn(Arrays.asList(KNOWN_LOCALES));
    }

    private void assertContextEquals(final String expect, final InputStream inputStream)
            throws IOException {
        try (final BufferedReader bf = new BufferedReader(new InputStreamReader(inputStream))) {
            assertThat(bf.readLine()).isEqualTo(expect);
        }
    }

    private void beginDocument(final XmlPullParser parser, final String firstElementName)
            throws XmlPullParserException, IOException {
        int type;
        while ((type = parser.next()) != XmlPullParser.START_TAG) {
        }

        if (type != XmlPullParser.START_TAG) {
            fail("No start tag found");
        }
        assertThat(firstElementName).isEqualTo(parser.getName());
    }
}
