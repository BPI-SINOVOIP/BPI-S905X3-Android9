/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.graphics.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.Typeface.Builder;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class TypefaceTest {
    // generic family name for monospaced fonts
    private static final String MONO = "monospace";
    private static final String DEFAULT = (String)null;
    private static final String INVALID = "invalid-family-name";

    private static final float GLYPH_1EM_WIDTH;
    private static final float GLYPH_3EM_WIDTH;

    private static float measureText(String text, Typeface typeface) {
        final Paint paint = new Paint();
        // Fix the locale so that fix the locale based fallback.
        paint.setTextLocale(Locale.US);
        paint.setTypeface(typeface);
        return paint.measureText(text);
    }

    static {
        // 3em.ttf supports "a", "b", "c". The width of "a" is 3em, others are 1em.
        final Context ctx = InstrumentationRegistry.getTargetContext();
        final Typeface typeface = ctx.getResources().getFont(R.font.a3em);
        GLYPH_3EM_WIDTH = measureText("a", typeface);
        GLYPH_1EM_WIDTH = measureText("b", typeface);
    }

    // list of family names to try when attempting to find a typeface with a given style
    private static final String[] FAMILIES =
            { (String) null, "monospace", "serif", "sans-serif", "cursive", "arial", "times" };

    private Context mContext;

    /**
     * Create a typeface of the given style. If the default font does not support the style,
     * a number of generic families are tried.
     * @return The typeface or null, if no typeface with the given style can be found.
     */
    private static Typeface createTypeface(int style) {
        for (String family : FAMILIES) {
            Typeface tf = Typeface.create(family, style);
            if (tf.getStyle() == style) {
                return tf;
            }
        }
        return null;
    }

    @Before
    public void setup() {
        mContext = InstrumentationRegistry.getTargetContext();
    }

    @Test
    public void testIsBold() {
        Typeface typeface = createTypeface(Typeface.BOLD);
        if (typeface != null) {
            assertEquals(Typeface.BOLD, typeface.getStyle());
            assertTrue(typeface.isBold());
            assertFalse(typeface.isItalic());
        }

        typeface = createTypeface(Typeface.ITALIC);
        if (typeface != null) {
            assertEquals(Typeface.ITALIC, typeface.getStyle());
            assertFalse(typeface.isBold());
            assertTrue(typeface.isItalic());
        }

        typeface = createTypeface(Typeface.BOLD_ITALIC);
        if (typeface != null) {
            assertEquals(Typeface.BOLD_ITALIC, typeface.getStyle());
            assertTrue(typeface.isBold());
            assertTrue(typeface.isItalic());
        }

        typeface = createTypeface(Typeface.NORMAL);
        if (typeface != null) {
            assertEquals(Typeface.NORMAL, typeface.getStyle());
            assertFalse(typeface.isBold());
            assertFalse(typeface.isItalic());
        }
    }

    @Test
    public void testCreate() {
        Typeface typeface = Typeface.create(DEFAULT, Typeface.NORMAL);
        assertNotNull(typeface);
        typeface = Typeface.create(MONO, Typeface.BOLD);
        assertNotNull(typeface);
        typeface = Typeface.create(INVALID, Typeface.ITALIC);
        assertNotNull(typeface);

        typeface = Typeface.create(typeface, Typeface.NORMAL);
        assertNotNull(typeface);
        typeface = Typeface.create(typeface, Typeface.BOLD);
        assertNotNull(typeface);
    }

    @Test
    public void testDefaultFromStyle() {
        Typeface typeface = Typeface.defaultFromStyle(Typeface.NORMAL);
        assertNotNull(typeface);
        typeface = Typeface.defaultFromStyle(Typeface.BOLD);
        assertNotNull(typeface);
        typeface = Typeface.defaultFromStyle(Typeface.ITALIC);
        assertNotNull(typeface);
        typeface = Typeface.defaultFromStyle(Typeface.BOLD_ITALIC);
        assertNotNull(typeface);
    }

    @Test
    public void testConstants() {
        assertNotNull(Typeface.DEFAULT);
        assertNotNull(Typeface.DEFAULT_BOLD);
        assertNotNull(Typeface.MONOSPACE);
        assertNotNull(Typeface.SANS_SERIF);
        assertNotNull(Typeface.SERIF);
    }

    @Test(expected=NullPointerException.class)
    public void testCreateFromAssetNull() {
        // input abnormal params.
        Typeface.createFromAsset(null, null);
    }

    @Test(expected=NullPointerException.class)
    public void testCreateFromAssetNullPath() {
        // input abnormal params.
        Typeface.createFromAsset(mContext.getAssets(), null);
    }

    @Test(expected=RuntimeException.class)
    public void testCreateFromAssetInvalidPath() {
        // input abnormal params.
        Typeface.createFromAsset(mContext.getAssets(), "invalid path");
    }

    @Test
    public void testCreateFromAsset() {
        Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), "samplefont.ttf");
        assertNotNull(typeface);
    }

    @Test(expected=NullPointerException.class)
    public void testCreateFromFileByFileReferenceNull() {
        // input abnormal params.
        Typeface.createFromFile((File) null);
    }

    @Test
    public void testCreateFromFileByFileReference() throws IOException {
        File file = new File(obtainPath());
        Typeface typeface = Typeface.createFromFile(file);
        assertNotNull(typeface);
    }

    @Test(expected=RuntimeException.class)
    public void testCreateFromFileWithInvalidPath() throws IOException {
        File file = new File("/invalid/path");
        Typeface.createFromFile(file);
    }

    @Test(expected=NullPointerException.class)
    public void testCreateFromFileByFileNameNull() throws IOException {
        // input abnormal params.
        Typeface.createFromFile((String) null);
    }

    @Test(expected=RuntimeException.class)
    public void testCreateFromFileByInvalidFileName() throws IOException {
        // input abnormal params.
        Typeface.createFromFile("/invalid/path");
    }

    @Test
    public void testCreateFromFileByFileName() throws IOException {
        Typeface typeface = Typeface.createFromFile(obtainPath());
        assertNotNull(typeface);
    }

    private String obtainPath() throws IOException {
        File dir = mContext.getFilesDir();
        dir.mkdirs();
        File file = new File(dir, "test.jpg");
        if (!file.createNewFile()) {
            if (!file.exists()) {
                fail("Failed to create new File!");
            }
        }
        InputStream is = mContext.getAssets().open("samplefont.ttf");
        FileOutputStream fOutput = new FileOutputStream(file);
        byte[] dataBuffer = new byte[1024];
        int readLength = 0;
        while ((readLength = is.read(dataBuffer)) != -1) {
            fOutput.write(dataBuffer, 0, readLength);
        }
        is.close();
        fOutput.close();
        return (file.getPath());
    }

    @Test
    public void testInvalidCmapFont() {
        Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), "bombfont.ttf");
        assertNotNull(typeface);
        final String testString = "abcde";
        float widthDefaultTypeface = measureText(testString, Typeface.DEFAULT);
        float widthCustomTypeface = measureText(testString, typeface);
        assertEquals(widthDefaultTypeface, widthCustomTypeface, 1.0f);
    }

    @Test
    public void testInvalidCmapFont2() {
        Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), "bombfont2.ttf");
        assertNotNull(typeface);
        final String testString = "abcde";
        float widthDefaultTypeface = measureText(testString, Typeface.DEFAULT);
        float widthCustomTypeface = measureText(testString, typeface);
        assertEquals(widthDefaultTypeface, widthCustomTypeface, 1.0f);
    }

    @Test
    public void testInvalidCmapFont_tooLargeCodePoints() {
        // Following three font doen't have any coverage between U+0000..U+10FFFF. Just make sure
        // they don't crash us.
        final String[] INVALID_CMAP_FONTS = {
            "out_of_unicode_start_cmap12.ttf",
            "out_of_unicode_end_cmap12.ttf",
            "too_large_start_cmap12.ttf",
            "too_large_end_cmap12.ttf",
        };
        for (final String file : INVALID_CMAP_FONTS) {
            final Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), file);
            assertNotNull(typeface);
        }
    }

    @Test
    public void testInvalidCmapFont_unsortedEntries() {
        // Following two font files have glyph for U+0400 and U+0100 but the fonts must not be used
        // due to invalid cmap data. For more details, see each ttx source file.
        final String[] INVALID_CMAP_FONTS = { "unsorted_cmap4.ttf", "unsorted_cmap12.ttf" };
        for (final String file : INVALID_CMAP_FONTS) {
            final Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), file);
            assertNotNull(typeface);
            final String testString = "\u0100\u0400";
            final float widthDefaultTypeface = measureText(testString, Typeface.DEFAULT);
            final float widthCustomTypeface = measureText(testString, typeface);
            assertEquals(widthDefaultTypeface, widthCustomTypeface, 0.0f);
        }

        // Following two font files have glyph for U+0400 U+FE00 and U+0100 U+FE00 but the fonts
        // must not be used due to invalid cmap data. For more details, see each ttx source file.
        final String[] INVALID_CMAP_VS_FONTS = {
            "unsorted_cmap14_default_uvs.ttf",
            "unsorted_cmap14_non_default_uvs.ttf"
        };
        for (final String file : INVALID_CMAP_VS_FONTS) {
            final Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), file);
            assertNotNull(typeface);
            final String testString = "\u0100\uFE00\u0400\uFE00";
            final float widthDefaultTypeface = measureText(testString, Typeface.DEFAULT);
            final float widthCustomTypeface = measureText(testString, typeface);
            assertEquals(widthDefaultTypeface, widthCustomTypeface, 0.0f);
        }
    }

    @Test
    public void testCreateFromAsset_cachesTypeface() {
        Typeface typeface1 = Typeface.createFromAsset(mContext.getAssets(), "samplefont.ttf");
        assertNotNull(typeface1);

        Typeface typeface2 = Typeface.createFromAsset(mContext.getAssets(), "samplefont.ttf");
        assertNotNull(typeface2);
        assertSame("Same font asset should return same Typeface object", typeface1, typeface2);

        Typeface typeface3 = Typeface.createFromAsset(mContext.getAssets(), "samplefont2.ttf");
        assertNotNull(typeface3);
        assertNotSame("Different font asset should return different Typeface object",
                typeface2, typeface3);

        Typeface typeface4 = Typeface.createFromAsset(mContext.getAssets(), "samplefont3.ttf");
        assertNotNull(typeface4);
        assertNotSame("Different font asset should return different Typeface object",
                typeface2, typeface4);
        assertNotSame("Different font asset should return different Typeface object",
                typeface3, typeface4);
    }

    @Test
    public void testBadFont() {
        Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), "ft45987.ttf");
        assertNotNull(typeface);
    }

    @Test
    public void testTypefaceBuilder_AssetSource() {
        Typeface typeface1 = new Typeface.Builder(mContext.getAssets(), "samplefont.ttf").build();
        assertNotNull(typeface1);

        Typeface typeface2 = new Typeface.Builder(mContext.getAssets(), "samplefont.ttf").build();
        assertNotNull(typeface2);
        assertSame("Same font asset should return same Typeface object", typeface1, typeface2);

        Typeface typeface3 = new Typeface.Builder(mContext.getAssets(), "samplefont2.ttf").build();
        assertNotNull(typeface3);
        assertNotSame("Different font asset should return different Typeface object",
                typeface2, typeface3);

        Typeface typeface4 = new Typeface.Builder(mContext.getAssets(), "samplefont3.ttf").build();
        assertNotNull(typeface4);
        assertNotSame("Different font asset should return different Typeface object",
                typeface2, typeface4);
        assertNotSame("Different font asset should return different Typeface object",
                typeface3, typeface4);

        Typeface typeface5 = new Typeface.Builder(mContext.getAssets(), "samplefont.ttf")
                .setFontVariationSettings("'wdth' 1.0").build();
        assertNotNull(typeface5);
        assertNotSame("Different font font variation should return different Typeface object",
                typeface2, typeface5);

        Typeface typeface6 = new Typeface.Builder(mContext.getAssets(), "samplefont.ttf")
                .setFontVariationSettings("'wdth' 2.0").build();
        assertNotNull(typeface6);
        assertNotSame("Different font font variation should return different Typeface object",
                typeface2, typeface6);
        assertNotSame("Different font font variation should return different Typeface object",
                typeface5, typeface6);

        // TODO: Add ttc index case. Need TTC file for CTS. (b/36731640)
    }

    @Test
    public void testTypefaceBuilder_FileSource() {
        try {
            File file = new File(obtainPath());
            Typeface typeface1 = new Typeface.Builder(obtainPath()).build();
            assertNotNull(typeface1);

            Typeface typeface2 = new Typeface.Builder(file).build();
            assertNotNull(typeface2);

            Typeface typeface3 = new Typeface.Builder(file)
                    .setFontVariationSettings("'wdth' 1.0")
                    .build();
            assertNotNull(typeface3);
            assertNotSame(typeface1, typeface3);
            assertNotSame(typeface2, typeface3);

            // TODO: Add ttc index case. Need TTC file for CTS.
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    @Test
    public void testTypefaceBuilder_fallback() throws IOException {
        final File validFile = new File(obtainPath());
        final File invalidFile = new File("/some/invalid/path/to/font/file");
        final AssetManager assets = mContext.getAssets();
        // By default, returns null if no fallback font is specified.
        assertNull(new Typeface.Builder(invalidFile).build());

        assertNull(new Typeface.Builder(validFile)
                .setTtcIndex(100 /* non-existing ttc index */).build());

        assertNull(new Typeface.Builder(assets, "invalid path").build());

        assertNull(new Typeface.Builder(assets, "samplefont.ttf")
                .setTtcIndex(100 /* non-existing ttc index */).build());

        // If fallback is set, the builder never returns null.
        assertNotNull(new Typeface.Builder(invalidFile).setFallback("").build());

        assertNotNull(new Typeface.Builder(invalidFile).setFallback("invalid name").build());

        Typeface sansSerifTypeface = new Typeface.Builder(invalidFile)
                .setFallback("sans-serif").build();
        assertNotNull(sansSerifTypeface);

        Typeface serifTypeface = new Typeface.Builder(invalidFile).setFallback("serif").build();
        assertNotNull(serifTypeface);

        Typeface boldSansSerifTypeface = new Typeface.Builder(invalidFile)
                .setFallback("sans-serif").setWeight(700).build();
        assertNotNull(boldSansSerifTypeface);

        Typeface boldSerifTypeface = new Typeface.Builder(invalidFile)
                .setFallback("serif").setWeight(700).build();
        assertNotNull(boldSerifTypeface);

        Typeface italicSansSerifTypeface = new Typeface.Builder(invalidFile)
                .setFallback("sans-serif").setItalic(true).build();
        assertNotNull(italicSansSerifTypeface);

        Typeface italicSerifTypeface = new Typeface.Builder(invalidFile)
                .setFallback("serif").setItalic(true).build();
        assertNotNull(italicSerifTypeface);

        // All fallbacks should be different each other.
        assertNotSame(sansSerifTypeface, serifTypeface);
        assertNotSame(sansSerifTypeface, boldSansSerifTypeface);
        assertNotSame(sansSerifTypeface, boldSerifTypeface);
        assertNotSame(sansSerifTypeface, italicSansSerifTypeface);
        assertNotSame(sansSerifTypeface, italicSerifTypeface);
        assertNotSame(serifTypeface, boldSansSerifTypeface);
        assertNotSame(serifTypeface, boldSerifTypeface);
        assertNotSame(serifTypeface, italicSansSerifTypeface);
        assertNotSame(serifTypeface, italicSerifTypeface);
        assertNotSame(boldSansSerifTypeface, boldSerifTypeface);
        assertNotSame(boldSansSerifTypeface, italicSansSerifTypeface);
        assertNotSame(boldSansSerifTypeface, italicSerifTypeface);
        assertNotSame(boldSerifTypeface, italicSansSerifTypeface);
        assertNotSame(boldSerifTypeface, italicSerifTypeface);
        assertNotSame(italicSansSerifTypeface, italicSerifTypeface);

        // Cache should work for the same fallback.
        assertSame(sansSerifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("sans-serif")
                        .setTtcIndex(100 /* non-existing ttc index */).build());
        assertSame(serifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("serif")
                        .setTtcIndex(100 /* non-existing ttc index */).build());
        assertSame(boldSansSerifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("sans-serif")
                        .setTtcIndex(100 /* non-existing ttc index */).setWeight(700).build());
        assertSame(boldSerifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("serif")
                        .setTtcIndex(100 /* non-existing ttc index */).setWeight(700).build());
        assertSame(italicSansSerifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("sans-serif")
                        .setTtcIndex(100 /* non-existing ttc index */).setItalic(true).build());
        assertSame(italicSerifTypeface,
                new Typeface.Builder(assets, "samplefont.ttf").setFallback("serif")
                        .setTtcIndex(100 /* non-existing ttc index */).setItalic(true).build());
    }

    @Test
    public void testTypefaceBuilder_FileSourceFD() {
        try (FileInputStream fis = new FileInputStream(obtainPath())) {
            assertNotNull(new Typeface.Builder(fis.getFD()).build());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    @Test
    public void testTypeface_SupportedCmapEncodingTest() {
        // We support the following combinations of cmap platfrom/endcoding pairs.
        String[] fontPaths = {
            "CmapPlatform0Encoding0.ttf",  // Platform ID == 0, Encoding ID == 0
            "CmapPlatform0Encoding1.ttf",  // Platform ID == 0, Encoding ID == 1
            "CmapPlatform0Encoding2.ttf",  // Platform ID == 0, Encoding ID == 2
            "CmapPlatform0Encoding3.ttf",  // Platform ID == 0, Encoding ID == 3
            "CmapPlatform0Encoding4.ttf",  // Platform ID == 0, Encoding ID == 4
            "CmapPlatform0Encoding6.ttf",  // Platform ID == 0, Encoding ID == 6
            "CmapPlatform3Encoding1.ttf",  // Platform ID == 3, Encoding ID == 1
            "CmapPlatform3Encoding10.ttf",  // Platform ID == 3, Encoding ID == 10
        };

        for (String fontPath : fontPaths) {
            Typeface typeface = Typeface.createFromAsset(mContext.getAssets(), fontPath);
            assertNotNull(typeface);
            final String testString = "a";
            float widthDefaultTypeface = measureText(testString, Typeface.DEFAULT);
            float widthCustomTypeface = measureText(testString, typeface);
            // The width of the glyph "a" from above fonts are 2em.
            // So the width should be different from the default one.
            assertNotEquals(widthDefaultTypeface, widthCustomTypeface, 1.0f);
        }
    }

    @Test
    public void testTypefaceBuilder_customFallback() {
        final String fontPath = "samplefont2.ttf";
        final Typeface regularTypeface = new Typeface.Builder(mContext.getAssets(), fontPath)
                .setWeight(400).build();
        final Typeface blackTypeface = new Typeface.Builder(mContext.getAssets(), fontPath)
                .setWeight(900).build();

        // W is not supported by samplefont2.ttf
        final String testString = "WWWWWWWWWWWWWWWWWWWWW";

        final Paint p = new Paint();
        p.setTextLocale(Locale.US);
        p.setTextSize(128);

        p.setTypeface(regularTypeface);
        final float widthFromRegular = p.measureText(testString);

        p.setTypeface(blackTypeface);
        final float widthFromBlack = p.measureText(testString);

        assertNotEquals(widthFromRegular, widthFromBlack, 1.0f);
    }

    @Test
    public void testTypefaceCreate_withExactWeight() {
        // multiweight_family has following fonts.
        // - a3em.ttf with weight = 100, style=normal configuration.
        //   This font supports "a", "b", "c". The weight "a" is 3em, others are 1em.
        // - b3em.ttf with weight = 400, style=normal configuration.
        //   This font supports "a", "b", "c". The weight "b" is 3em, others are 1em.
        // - c3em.ttf with weight = 700, style=normal configuration.
        //   This font supports "a", "b", "c". The weight "c" is 3em, others are 1em.
        final Typeface family = mContext.getResources().getFont(R.font.multiweight_family);
        assertNotNull(family);

        // By default, the font which weight is 400 is selected.
        assertEquals(GLYPH_1EM_WIDTH, measureText("a", family), 0f);
        assertEquals(GLYPH_3EM_WIDTH, measureText("b", family), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("c", family), 0f);

        // Draw with the font which weight is 100.
        final Typeface thinFamily = Typeface.create(family, 100 /* weight */, false /* italic */);
        assertEquals(GLYPH_3EM_WIDTH, measureText("a", thinFamily), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("b", thinFamily), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("c", thinFamily), 0f);

        // Draw with the font which weight is 700.
        final Typeface boldFamily = Typeface.create(family, 700 /* weight */, false /* italic */);
        assertEquals(GLYPH_1EM_WIDTH, measureText("a", boldFamily), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("b", boldFamily), 0f);
        assertEquals(GLYPH_3EM_WIDTH, measureText("c", boldFamily), 0f);
    }

    @Test
    public void testTypefaceCreate_withExactStyle() {
        // multiweight_family has following fonts.
        // - a3em.ttf with weight = 400, style=normal configuration.
        //   This font supports "a", "b", "c". The weight "a" is 3em, others are 1em.
        // - b3em.ttf with weight = 400, style=italic configuration.
        //   This font supports "a", "b", "c". The weight "b" is 3em, others are 1em.
        // - c3em.ttf with weight = 700, style=italic configuration.
        //   This font supports "a", "b", "c". The weight "c" is 3em, others are 1em.
        final Typeface family = mContext.getResources().getFont(R.font.multistyle_family);
        assertNotNull(family);

        // By default, the normal style font which weight is 400 is selected.
        assertEquals(GLYPH_3EM_WIDTH, measureText("a", family), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("b", family), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("c", family), 0f);

        // Draw with the italic font.
        final Typeface italicFamily = Typeface.create(family, 400 /* weight */, true /* italic */);
        assertEquals(GLYPH_1EM_WIDTH, measureText("a", italicFamily), 0f);
        assertEquals(GLYPH_3EM_WIDTH, measureText("b", italicFamily), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("c", italicFamily), 0f);

        // Draw with the italic font which weigth is 700.
        final Typeface boldItalicFamily =
                Typeface.create(family, 700 /* weight */, true /* italic */);
        assertEquals(GLYPH_1EM_WIDTH, measureText("a", boldItalicFamily), 0f);
        assertEquals(GLYPH_1EM_WIDTH, measureText("b", boldItalicFamily), 0f);
        assertEquals(GLYPH_3EM_WIDTH, measureText("c", boldItalicFamily), 0f);
    }

    @Test
    public void testFontVariationSettings() {
        // WeightEqualsEmVariableFont is a special font generating the outlines a glyph of 1/1000
        // width of the given wght axis. For example, if 300 is given as the wght value to the font,
        // the font will generate 0.3em of the glyph for the 'a'..'z' characters.
        // The minimum, default, maximum value of 'wght' is 0, 0, 1000.
        // No other axes are supported.

        final AssetManager am = mContext.getAssets();
        final Paint paint = new Paint();
        paint.setTextSize(100);  // Make 1em = 100px

        // By default, WeightEqualsEmVariableFont has 0 'wght' value.
        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf").build());
        assertEquals(0.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 100").build());
        assertEquals(10.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 300").build());
        assertEquals(30.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 800").build());
        assertEquals(80.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 550").build());
        assertEquals(55.0f, paint.measureText("a"), 0.0f);
    }

    @Test
    public void testFontVariationSettings_UnsupportedAxes() {
        // WeightEqualsEmVariableFont is a special font generating the outlines a glyph of 1/1000
        // width of the given wght axis. For example, if 300 is given as the wght value to the font,
        // the font will generate 0.3em of the glyph for the 'a'..'z' characters.
        // The minimum, default, maximum value of 'wght' is 0, 0, 1000.
        // No other axes are supported.

        final AssetManager am = mContext.getAssets();
        final Paint paint = new Paint();
        paint.setTextSize(100);  // Make 1em = 100px

        // Unsupported axes do not affect the result.
        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 300, 'wdth' 10").build());
        assertEquals(30.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wdth' 10, 'wght' 300").build());
        assertEquals(30.0f, paint.measureText("a"), 0.0f);
    }

    @Test
    public void testFontVariationSettings_OutOfRangeValue() {
        // WeightEqualsEmVariableFont is a special font generating the outlines a glyph of 1/1000
        // width of the given wght axis. For example, if 300 is given as the wght value to the font,
        // the font will generate 0.3em of the glyph for the 'a'..'z' characters.
        // The minimum, default, maximum value of 'wght' is 0, 0, 1000.
        // No other axes are supported.

        final AssetManager am = mContext.getAssets();
        final Paint paint = new Paint();
        paint.setTextSize(100);  // Make 1em = 100px

        // Out of range value needs to be clipped at the minimum or maximum values.
        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' -100").build());
        assertEquals(0.0f, paint.measureText("a"), 0.0f);

        paint.setTypeface(new Typeface.Builder(am, "WeightEqualsEmVariableFont.ttf")
                .setFontVariationSettings("'wght' 1300").build());
        assertEquals(100.0f, paint.measureText("a"), 0.0f);
    }

    @Test
    public void testTypefaceCreate_getWeight() {
        Typeface typeface = Typeface.DEFAULT;
        assertEquals(400, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 100, false /* italic */);
        assertEquals(100, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 100, true /* italic */);
        assertEquals(100, typeface.getWeight());
        assertTrue(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 400, false /* italic */);
        assertEquals(400, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 400, true /* italic */);
        assertEquals(400, typeface.getWeight());
        assertTrue(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 700, false /* italic */);
        assertEquals(700, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 700, true /* italic */);
        assertEquals(700, typeface.getWeight());
        assertTrue(typeface.isItalic());

        // Non-standard weight.
        typeface = Typeface.create(Typeface.DEFAULT, 250, false /* italic */);
        assertEquals(250, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = Typeface.create(Typeface.DEFAULT, 250, true /* italic */);
        assertEquals(250, typeface.getWeight());
        assertTrue(typeface.isItalic());

    }

    @Test
    public void testTypefaceCreate_customFont_getWeight() {
        final AssetManager am = mContext.getAssets();

        Typeface typeface = new Builder(am, "ascii_a3em_weight100_upright.ttf").build();
        assertEquals(100, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = new Builder(am, "ascii_b3em_weight100_italic.ttf").build();
        assertEquals(100, typeface.getWeight());
        assertTrue(typeface.isItalic());

    }

    @Test
    public void testTypefaceCreate_customFont_customWeight() {
        final AssetManager am = mContext.getAssets();
        Typeface typeface = new Builder(am, "ascii_a3em_weight100_upright.ttf")
                .setWeight(400).build();
        assertEquals(400, typeface.getWeight());
        assertFalse(typeface.isItalic());

        typeface = new Builder(am, "ascii_b3em_weight100_italic.ttf").setWeight(400).build();
        assertEquals(400, typeface.getWeight());
        assertTrue(typeface.isItalic());
    }

    @Test
    public void testTypefaceCreate_customFont_customItalic() {
        final AssetManager am = mContext.getAssets();

        Typeface typeface = new Builder(am, "ascii_a3em_weight100_upright.ttf")
                .setItalic(true).build();
        assertEquals(100, typeface.getWeight());
        assertTrue(typeface.isItalic());

        typeface = new Builder(am, "ascii_b3em_weight100_italic.ttf").setItalic(false).build();
        assertEquals(100, typeface.getWeight());
        assertFalse(typeface.isItalic());
    }
}
