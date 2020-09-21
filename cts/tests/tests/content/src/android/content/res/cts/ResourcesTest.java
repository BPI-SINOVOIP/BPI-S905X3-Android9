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

package android.content.res.cts;

import android.content.cts.R;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.cts.util.XmlUtils;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.AdaptiveIconDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.LocaleList;
import android.test.AndroidTestCase;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.util.Xml;
import android.view.Display;
import android.view.WindowManager;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

public class ResourcesTest extends AndroidTestCase {
    private static final String CONFIG_VARYING = "configVarying";
    private static final String SIMPLE = "simple";
    private static final String CONFIG_VARYING_SIMPLE = "configVarying/simple";
    private static final String PACKAGE_NAME = "android.content.cts";
    private static final String COM_ANDROID_CTS_STUB_IDENTIFIER =
                "android.content.cts:configVarying/simple";
    private Resources mResources;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mResources = getContext().getResources();
    }

    public void testResources() {
        final AssetManager am = new AssetManager();
        final Configuration cfg = new Configuration();
        cfg.keyboard = Configuration.KEYBOARDHIDDEN_YES;
        final DisplayMetrics dm = new DisplayMetrics();
        dm.setToDefaults();

        final Resources r = new Resources(am, dm, cfg);
        final Configuration c = r.getConfiguration();
        assertEquals(Configuration.KEYBOARDHIDDEN_YES, c.keyboard);
    }

    public void testGetString() {
        try {
            mResources.getString(-1, "%s");
            fail("Failed at testGetString2");
        } catch (NotFoundException e) {
          //expected
        }

        final String strGo = mResources.getString(R.string.go, "%1$s%%", 12);
        assertEquals("Go", strGo);
    }

    public void testObtainAttributes() throws XmlPullParserException, IOException {
        final XmlPullParser parser = mResources.getXml(R.xml.test_color);
        XmlUtils.beginDocument(parser, "resources");
        final AttributeSet set = Xml.asAttributeSet(parser);
        final TypedArray testTypedArray = mResources.obtainAttributes(set, R.styleable.Style1);
        assertNotNull(testTypedArray);
        assertEquals(2, testTypedArray.length());
        assertEquals(0, testTypedArray.getColor(0, 0));
        assertEquals(128, testTypedArray.getColor(1, 128));
        assertEquals(mResources, testTypedArray.getResources());
        testTypedArray.recycle();
    }

    public void testObtainTypedArray() {
        try {
            mResources.obtainTypedArray(-1);
            fail("Failed at testObtainTypedArray");
        } catch (NotFoundException e) {
            //expected
        }

        final TypedArray ta = mResources.obtainTypedArray(R.array.string);
        assertEquals(3, ta.length());
        assertEquals("Test String 1", ta.getString(0));
        assertEquals("Test String 2", ta.getString(1));
        assertEquals("Test String 3", ta.getString(2));
        assertEquals(mResources, ta.getResources());
    }

    private Resources getResources(final Configuration config, final int mcc, final int mnc,
            final int touchscreen, final int keyboard, final int keysHidden, final int navigation,
            final int width, final int height) {
        final AssetManager assmgr = new AssetManager();
        assmgr.addAssetPath(mContext.getPackageResourcePath());
        final DisplayMetrics metrics = new DisplayMetrics();
        final WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        final Display d = wm.getDefaultDisplay();
        d.getMetrics(metrics);
        config.mcc = mcc;
        config.mnc = mnc;
        config.touchscreen = touchscreen;
        config.keyboard = keyboard;
        config.keyboardHidden = keysHidden;
        config.navigation = navigation;
        metrics.widthPixels = width;
        metrics.heightPixels = height;
        return new Resources(assmgr, metrics, config);
    }

    private static void checkGetText1(final Resources res, final int resId,
            final String expectedValue) {
        final String actual = res.getText(resId).toString();
        assertNotNull("Returned wrong configuration-based simple value: expected <nothing>, "
                + "got '" + actual + "' from resource 0x" + Integer.toHexString(resId),
                expectedValue);
        assertEquals("Returned wrong configuration-based simple value: expected " + expectedValue
                + ", got '" + actual + "' from resource 0x" + Integer.toHexString(resId),
                expectedValue, actual);
    }

    private static void checkGetText2(final Resources res, final int resId,
            final String expectedValue) {
        final String actual = res.getText(resId, null).toString();
        assertNotNull("Returned wrong configuration-based simple value: expected <nothing>, "
                + "got '" + actual + "' from resource 0x" + Integer.toHexString(resId),
                expectedValue);
        assertEquals("Returned wrong configuration-based simple value: expected " + expectedValue
                + ", got '" + actual + "' from resource 0x" + Integer.toHexString(resId),
                expectedValue, actual);
    }

    public void testGetMovie() {
        try {
            mResources.getMovie(-1);
            fail("Failed at testGetMovie");
        } catch (NotFoundException e) {
            //expected
        }
    }

    public void testGetDimension() {
        try {
            mResources.getDimension(-1);
            fail("Failed at testGetDimension");
        } catch (NotFoundException e) {
            //expected
        }

        // app_icon_size is 48px, as defined in cts/tests/res/values/resources_test.xml
        final float dim = mResources.getDimension(R.dimen.app_icon_size);
        assertEquals(48.0f, dim);
    }

    public void testGetDimensionPixelOffset() {
        try {
            mResources.getDimensionPixelOffset(-1);
            fail("Failed at testGetDimensionPixelOffset");
        } catch (NotFoundException e) {
            //expected
        }

        // app_icon_size is 48px, as defined in cts/tests/res/values/resources_test.xml
        final int dim = mResources.getDimensionPixelOffset(R.dimen.app_icon_size);
        assertEquals(48, dim);
    }

    public void testGetColorStateList() {
        try {
            mResources.getColorStateList(-1);
            fail("Failed at testGetColorStateList");
        } catch (NotFoundException e) {
            //expected
        }

        final ColorStateList colorStateList = mResources.getColorStateList(R.color.color1);
        final int[] focusedState = {android.R.attr.state_focused};
        final int focusColor = colorStateList.getColorForState(focusedState, R.color.failColor);
        assertEquals(mResources.getColor(R.color.testcolor1), focusColor);
    }

    public void testGetColorStateListThrows() {
        try {
            // XML that's not a selector or gradient throws
            mResources.getColorStateList(R.drawable.density_test);
            fail("Failed at testGetColorStateList");
        } catch (NotFoundException expected) {
            // expected
        }
    }

    public void testGetColor() {
        try {
            mResources.getColor(-1);
            fail("Failed at testGetColor");
        } catch (NotFoundException e) {
            //expected
        }

        final int color = mResources.getColor(R.color.testcolor1);
        assertEquals(0xff00ff00, color);
    }

    public Resources createNewResources() {
        final DisplayMetrics dm = new DisplayMetrics();
        dm.setToDefaults();
        final Configuration cfg = new Configuration();
        cfg.setToDefaults();
        return new Resources(new AssetManager(), dm, cfg);
    }

    public void testUpdateConfiguration() {
        Resources res = createNewResources();
        final Configuration cfg = new Configuration(res.getConfiguration());
        assertTrue(cfg.fontScale != 5);

        cfg.fontScale = 5;
        res.updateConfiguration(cfg, null);
        assertEquals(5.0f, res.getConfiguration().fontScale, 0.001f);
    }

    public void testUpdateConfiguration_emptyLocaleIsOverridden() {
        Resources res = createNewResources();
        res.getConfiguration().setLocales(null);
        assertTrue(res.getConfiguration().getLocales().isEmpty());

        final Configuration cfg = new Configuration();
        cfg.setToDefaults();
        assertTrue(cfg.getLocales().isEmpty());

        res.updateConfiguration(cfg, null);
        assertEquals(LocaleList.getDefault(), res.getConfiguration().getLocales());
    }

    public void testUpdateConfiguration_copyLocales() {
        Resources res = createNewResources();
        final Configuration cfg = new Configuration(res.getConfiguration());

        cfg.setLocales(LocaleList.forLanguageTags("az-Arab,ru"));

        res.updateConfiguration(cfg, null);

        // Depending on the locales available in the framework resources, the LocaleList may be
        // re-arranged. Check that any valid permutation is present.
        final LocaleList locales = res.getConfiguration().getLocales();
        assertTrue(LocaleList.forLanguageTags("az-Arab,ru").equals(locales) ||
                LocaleList.forLanguageTags("ru,az-Arab").equals(locales));
    }

    public void testUpdateConfiguration_emptyAfterUpdate() {
        Resources res = createNewResources();
        final Configuration cfg = new Configuration(res.getConfiguration());
        cfg.setLocales(LocaleList.forLanguageTags("az-Arab"));

        res.updateConfiguration(cfg, null);
        assertEquals(LocaleList.forLanguageTags("az-Arab"), res.getConfiguration().getLocales());

        res.getConfiguration().setLocales(null);
        cfg.setLocales(null);
        res.updateConfiguration(cfg, null);
        assertEquals(LocaleList.getDefault(), res.getConfiguration().getLocales());
    }

    public void testGetDimensionPixelSize() {
        try {
            mResources.getDimensionPixelSize(-1);
            fail("Failed at testGetDimensionPixelSize");
        } catch (NotFoundException e) {
            //expected
        }

        // app_icon_size is 48px, as defined in cts/tests/res/values/resources_test.xml
        final int size = mResources.getDimensionPixelSize(R.dimen.app_icon_size);
        assertEquals(48, size);
        assertEquals(1, mResources.getDimensionPixelSize(R.dimen.pos_dimen_149));
        assertEquals(2, mResources.getDimensionPixelSize(R.dimen.pos_dimen_151));
        assertEquals(-1, mResources.getDimensionPixelSize(R.dimen.neg_dimen_149));
        assertEquals(-2, mResources.getDimensionPixelSize(R.dimen.neg_dimen_151));
    }

    public void testGetDrawable() {
        try {
            mResources.getDrawable(-1);
            fail("Failed at testGetDrawable");
        } catch (NotFoundException e) {
            //expected
        }

        // testimage is defined in cts/tests/res/drawable/testimage.jpg and measures 212px x 142px
        final Drawable draw = mResources.getDrawable(R.drawable.testimage);
        int targetDensity = mResources.getDisplayMetrics().densityDpi;
        int defaultDensity = DisplayMetrics.DENSITY_DEFAULT;
        assertNotNull(draw);
        assertEquals(212 * targetDensity / defaultDensity, draw.getIntrinsicWidth(), 1);
        assertEquals(142 * targetDensity / defaultDensity, draw.getIntrinsicHeight(), 1);

        // Some apps rely on the fact that this will return null (rather than throwing).
        assertNull(mResources.getDrawable(R.drawable.fake_image_will_not_decode));
    }

    public void testGetDrawable_StackOverflowErrorDrawable() {
        try {
            mResources.getDrawable(R.drawable.drawable_recursive);
            fail("Failed at testGetDrawable_StackOverflowErrorDrawable");
        } catch (NotFoundException e) {
            //expected
        }
    }

    public void testGetDrawable_StackOverflowErrorDrawable_mipmap() {
        try {
            mResources.getDrawable(R.mipmap.icon_recursive);
            fail("Failed at testGetDrawable_StackOverflowErrorDrawable_mipmap");
        } catch (NotFoundException e) {
            //expected
        }
    }

    public void testGetDrawableForDensity() {
        final Drawable ldpi = mResources.getDrawableForDensity(
                R.drawable.density_test, DisplayMetrics.DENSITY_LOW);
        assertEquals(300, ldpi.getIntrinsicWidth());

        final Drawable mdpi = mResources.getDrawableForDensity(
                R.drawable.density_test, DisplayMetrics.DENSITY_MEDIUM);
        assertEquals(200, mdpi.getIntrinsicWidth());

        final Drawable hdpi = mResources.getDrawableForDensity(
                R.drawable.density_test, DisplayMetrics.DENSITY_HIGH);
        assertEquals(100, hdpi.getIntrinsicWidth());
    }

    public void testGetDrawableForDensityWithZeroDensityIsSameAsGetDrawable() {
        final Drawable defaultDrawable = mResources.getDrawable(R.drawable.density_test, null);
        assertNotNull(defaultDrawable);

        final Drawable densityDrawable = mResources.getDrawableForDensity(R.drawable.density_test,
                0 /*density*/, null);
        assertNotNull(densityDrawable);

        assertEquals(defaultDrawable.getIntrinsicWidth(), densityDrawable.getIntrinsicWidth());
    }

    private Drawable extractForegroundFromAdaptiveIconDrawable(int id, int density) {
        final Drawable drawable = mResources.getDrawableForDensity(id, density, null);
        assertTrue(drawable instanceof AdaptiveIconDrawable);
        return ((AdaptiveIconDrawable) drawable).getForeground();
    }

    public void testGetDrawableForDensityWithAdaptiveIconDrawable() {
        final Drawable ldpi = extractForegroundFromAdaptiveIconDrawable(R.drawable.adaptive_icon,
                DisplayMetrics.DENSITY_LOW);
        assertNotNull(ldpi);
        assertEquals(300, ldpi.getIntrinsicWidth());

        final Drawable mdpi = extractForegroundFromAdaptiveIconDrawable(R.drawable.adaptive_icon,
                DisplayMetrics.DENSITY_MEDIUM);
        assertNotNull(mdpi);
        assertEquals(200, mdpi.getIntrinsicWidth());

        final Drawable hdpi = extractForegroundFromAdaptiveIconDrawable(R.drawable.adaptive_icon,
                DisplayMetrics.DENSITY_HIGH);
        assertNotNull(hdpi);
        assertEquals(100, hdpi.getIntrinsicWidth());
    }

    public void testGetAnimation() throws Exception {
        try {
            mResources.getAnimation(-1);
            fail("Failed at testGetAnimation");
        } catch (NotFoundException e) {
            //expected
        }

        final XmlResourceParser ani = mResources.getAnimation(R.anim.anim_rotate);
        assertNotNull(ani);
        XmlUtils.beginDocument(ani, "rotate");
        assertEquals(7, ani.getAttributeCount());
        assertEquals("Binary XML file line #18", ani.getPositionDescription());
        assertEquals("interpolator", ani.getAttributeName(0));
        assertEquals("@17432582", ani.getAttributeValue(0));
    }

    public void testGetQuantityString1() {
        try {
            mResources.getQuantityString(-1, 1, "");
            fail("Failed at testGetQuantityString1");
        } catch (NotFoundException e) {
            //expected
        }

        final String strGo = mResources.getQuantityString(R.plurals.plurals_test, 1, "");
        assertEquals("A dog", strGo);
    }

    public void testGetQuantityString2() {
        try {
            mResources.getQuantityString(-1, 1);
            fail("Failed at testGetQuantityString2");
        } catch (NotFoundException e) {
            //expected
        }

        final String strGo = mResources.getQuantityString(R.plurals.plurals_test, 1);
        assertEquals("A dog", strGo);
    }

    public void testGetInteger() {
        try {
            mResources.getInteger(-1);
            fail("Failed at testGetInteger");
        } catch (NotFoundException e) {
            //expected
        }

        final int i = mResources.getInteger(R.integer.resource_test_int);
        assertEquals(10, i);
    }

    public void testGetValue() {
        final TypedValue tv = new TypedValue();

        try {
            mResources.getValue("null", tv, false);
            fail("Failed at testGetValue");
        } catch (NotFoundException e) {
            //expected
        }

        mResources.getValue("android.content.cts:raw/text", tv, false);
        assertNotNull(tv);
        assertEquals("res/raw/text.txt", tv.coerceToString());
    }

    public void testGetValueForDensity() {
        final TypedValue tv = new TypedValue();

        mResources.getValueForDensity(R.string.density_string,
                DisplayMetrics.DENSITY_LOW, tv, false);
        assertEquals("ldpi", tv.coerceToString());

        mResources.getValueForDensity(R.string.density_string,
                DisplayMetrics.DENSITY_MEDIUM, tv, false);
        assertEquals("mdpi", tv.coerceToString());

        mResources.getValueForDensity(R.string.density_string,
                DisplayMetrics.DENSITY_HIGH, tv, false);
        assertEquals("hdpi", tv.coerceToString());
    }

    public void testGetValueForDensityWithZeroDensityIsSameAsGetValue() {
        final TypedValue defaultTv = new TypedValue();
        mResources.getValue(R.string.density_string, defaultTv, false);

        final TypedValue densityTv = new TypedValue();
        mResources.getValueForDensity(R.string.density_string, 0 /*density*/, densityTv, false);

        assertEquals(defaultTv.assetCookie, densityTv.assetCookie);
        assertEquals(defaultTv.data, densityTv.data);
        assertEquals(defaultTv.type, densityTv.type);
        assertEquals(defaultTv.string, densityTv.string);
    }

    public void testGetAssets() {
        final AssetManager aM = mResources.getAssets();
        assertNotNull(aM);
        assertTrue(aM.isUpToDate());
    }

    public void testGetSystem() {
        assertNotNull(Resources.getSystem());
    }

    public void testGetLayout() throws Exception {
        try {
            mResources.getLayout(-1);
            fail("Failed at testGetLayout");
        } catch (NotFoundException e) {
            //expected
        }

        final XmlResourceParser layout = mResources.getLayout(R.layout.abslistview_layout);
        assertNotNull(layout);
        XmlUtils.beginDocument(layout, "ViewGroup_Layout");
        assertEquals(3, layout.getAttributeCount());
        assertEquals("id", layout.getAttributeName(0));
        assertEquals("@" + R.id.abslistview_root, layout.getAttributeValue(0));
    }

    public void testGetBoolean() {
        try {
            mResources.getBoolean(-1);
            fail("Failed at testGetBoolean");
        } catch (NotFoundException e) {
            //expected
        }

        final boolean b = mResources.getBoolean(R.integer.resource_test_int);
        assertTrue(b);
    }

    public void testgetFraction() {
        assertEquals(1, (int)mResources.getFraction(R.dimen.frac100perc, 1, 1));
        assertEquals(100, (int)mResources.getFraction(R.dimen.frac100perc, 100, 1));
    }

    public void testParseBundleExtras() throws XmlPullParserException, IOException {
        final Bundle b = new Bundle();
        XmlResourceParser parser = mResources.getXml(R.xml.extra);
        XmlUtils.beginDocument(parser, "tag");

        assertEquals(0, b.size());
        mResources.parseBundleExtras(parser, b);
        assertEquals(1, b.size());
        assertEquals("android", b.getString("google"));
    }

    public void testParseBundleExtra() throws XmlPullParserException, IOException {
        final Bundle b = new Bundle();
        XmlResourceParser parser = mResources.getXml(R.xml.extra);

        XmlUtils.beginDocument(parser, "tag");
        assertEquals(0, b.size());
        mResources.parseBundleExtra("test", parser, b);
        assertEquals(1, b.size());
        assertEquals("Lee", b.getString("Bruce"));
    }

    public void testGetIdentifier() {

        int resid = mResources.getIdentifier(COM_ANDROID_CTS_STUB_IDENTIFIER, null, null);
        assertEquals(R.configVarying.simple, resid);

        resid = mResources.getIdentifier(CONFIG_VARYING_SIMPLE, null, PACKAGE_NAME);
        assertEquals(R.configVarying.simple, resid);

        resid = mResources.getIdentifier(SIMPLE, CONFIG_VARYING, PACKAGE_NAME);
        assertEquals(R.configVarying.simple, resid);
    }

    public void testGetIntArray() {
        final int NO_EXIST_ID = -1;
        try {
            mResources.getIntArray(NO_EXIST_ID);
            fail("should throw out NotFoundException");
        } catch (NotFoundException e) {
            // expected
        }
        // expected value is defined in res/value/arrays.xml
        final int[] expectedArray1 = new int[] {
                0, 0, 0
        };
        final int[] expectedArray2 = new int[] {
                0, 1, 101
        };
        int[]array1 = mResources.getIntArray(R.array.strings);
        int[]array2 = mResources.getIntArray(R.array.integers);

        checkArrayEqual(expectedArray1, array1);
        checkArrayEqual(expectedArray2, array2);

    }

    private void checkArrayEqual(int[] array1, int[] array2) {
        assertNotNull(array2);
        assertEquals(array1.length, array2.length);
        for (int i = 0; i < array1.length; i++) {
            assertEquals(array1[i], array2[i]);
        }
    }

    public void testGetQuantityText() {
        CharSequence cs;
        final Resources res = resourcesForLanguage("cs");

        cs = res.getQuantityText(R.plurals.plurals_test, 0);
        assertEquals("Some Czech dogs", cs.toString());

        cs = res.getQuantityText(R.plurals.plurals_test, 1);
        assertEquals("A Czech dog", cs.toString());

        cs = res.getQuantityText(R.plurals.plurals_test, 2);
        assertEquals("Few Czech dogs", cs.toString());

        cs = res.getQuantityText(R.plurals.plurals_test, 5);
        assertEquals("Some Czech dogs", cs.toString());

        cs = res.getQuantityText(R.plurals.plurals_test, 500);
        assertEquals("Some Czech dogs", cs.toString());

    }

    public void testChangingConfiguration() {
        ColorDrawable dr1 = (ColorDrawable) mResources.getDrawable(R.color.varies_uimode);
        assertEquals(ActivityInfo.CONFIG_UI_MODE, dr1.getChangingConfigurations());

        // Test again with a drawable obtained from the cache.
        ColorDrawable dr2 = (ColorDrawable) mResources.getDrawable(R.color.varies_uimode);
        assertEquals(ActivityInfo.CONFIG_UI_MODE, dr2.getChangingConfigurations());
    }

    private Resources resourcesForLanguage(final String lang) {
        final Configuration config = new Configuration();
        config.updateFrom(mResources.getConfiguration());
        config.setLocale(new Locale(lang));
        return new Resources(mResources.getAssets(), mResources.getDisplayMetrics(), config);
    }

    public void testGetResourceEntryName() {
        assertEquals(SIMPLE, mResources.getResourceEntryName(R.configVarying.simple));
    }

    public void testGetResourceName() {
        final String fullName = mResources.getResourceName(R.configVarying.simple);
        assertEquals(COM_ANDROID_CTS_STUB_IDENTIFIER, fullName);

        final String packageName = mResources.getResourcePackageName(R.configVarying.simple);
        assertEquals(PACKAGE_NAME, packageName);

        final String typeName = mResources.getResourceTypeName(R.configVarying.simple);
        assertEquals(CONFIG_VARYING, typeName);
    }

    public void testGetStringWithIntParam() {
        checkString(R.string.formattedStringNone,
                mResources.getString(R.string.formattedStringNone),
                "Format[]");
        checkString(R.string.formattedStringOne,
                mResources.getString(R.string.formattedStringOne),
                "Format[%d]");
        checkString(R.string.formattedStringTwo, mResources.getString(R.string.formattedStringTwo),
                "Format[%3$d,%2$s]");
        // Make sure the formatted one works
        checkString(R.string.formattedStringNone,
                mResources.getString(R.string.formattedStringNone),
                "Format[]");
        checkString(R.string.formattedStringOne,
                mResources.getString(R.string.formattedStringOne, 42),
                "Format[42]");
        checkString(R.string.formattedStringTwo,
                mResources.getString(R.string.formattedStringTwo, "unused", "hi", 43),
                "Format[43,hi]");
    }

    private static void checkString(final int resid, final String actual, final String expected) {
        assertEquals("Expecting string value \"" + expected + "\" got \""
                + actual + "\" in resources 0x" + Integer.toHexString(resid),
                expected, actual);
    }

    public void testGetStringArray() {
        checkStringArray(R.array.strings, new String[] {
                "zero", "1", "here"
        });
        checkTextArray(R.array.strings, new String[] {
                "zero", "1", "here"
        });
        checkStringArray(R.array.integers, new String[] {
                null, null, null
        });
        checkTextArray(R.array.integers, new String[] {
                null, null, null
        });
    }

    private void checkStringArray(final int resid, final String[] expected) {
        final String[] res = mResources.getStringArray(resid);
        assertEquals(res.length, expected.length);
        for (int i = 0; i < expected.length; i++) {
            checkEntry(resid, i, res[i], expected[i]);
        }
    }

    private void checkEntry(final int resid, final int index, final Object res,
            final Object expected) {
        assertEquals("in resource 0x" + Integer.toHexString(resid)
                + " at index " + index, expected, res);
    }

    private void checkTextArray(final int resid, final String[] expected) {
        final CharSequence[] res = mResources.getTextArray(resid);
        assertEquals(res.length, expected.length);
        for (int i = 0; i < expected.length; i++) {
            checkEntry(resid, i, res[i], expected[i]);
        }
    }

    public void testGetValueWithID() {
        tryBoolean(R.bool.trueRes, true);
        tryBoolean(R.bool.falseRes, false);

        tryString(R.string.coerceIntegerToString, "100");
        tryString(R.string.coerceBooleanToString, "true");
        tryString(R.string.coerceColorToString, "#fff");
        tryString(R.string.coerceFloatToString, "100.0");
        tryString(R.string.coerceDimensionToString, "100px");
        tryString(R.string.coerceFractionToString, "100%");
    }

    private void tryBoolean(final int resid, final boolean expected) {
        final TypedValue v = new TypedValue();
        mContext.getResources().getValue(resid, v, true);
        assertEquals(TypedValue.TYPE_INT_BOOLEAN, v.type);
        assertEquals("Expecting boolean value " + expected + " got " + v
                + " from TypedValue: in resource 0x" + Integer.toHexString(resid),
                expected, v.data != 0);
        assertEquals("Expecting boolean value " + expected + " got " + v
                + " from getBoolean(): in resource 0x" + Integer.toHexString(resid),
                expected, mContext.getResources().getBoolean(resid));
    }

    private void tryString(final int resid, final String expected) {
        final TypedValue v = new TypedValue();
        mContext.getResources().getValue(resid, v, true);
        assertEquals(TypedValue.TYPE_STRING, v.type);
        assertEquals("Expecting string value " + expected + " got " + v
                + ": in resource 0x" + Integer.toHexString(resid),
                expected, v.string);
    }

    public void testRawResource() throws Exception {
        assertNotNull(mResources.newTheme());

        InputStream is = mResources.openRawResource(R.raw.text);
        verifyTextAsset(is);

        is = mResources.openRawResource(R.raw.text, new TypedValue());
        verifyTextAsset(is);

        assertNotNull(mResources.openRawResourceFd(R.raw.text));
    }

    static void verifyTextAsset(final InputStream is) throws IOException {
        final String expectedString = "OneTwoThreeFourFiveSixSevenEightNineTen";
        final byte[] buffer = new byte[10];

        int readCount;
        int curIndex = 0;
        while ((readCount = is.read(buffer, 0, buffer.length)) > 0) {
            for (int i = 0; i < readCount; i++) {
                assertEquals("At index " + curIndex
                            + " expected " + expectedString.charAt(curIndex)
                            + " but found " + ((char) buffer[i]),
                        buffer[i], expectedString.charAt(curIndex));
                curIndex++;
            }
        }

        readCount = is.read(buffer, 0, buffer.length);
        assertEquals("Reading end of buffer: expected readCount=-1 but got " + readCount,
                -1, readCount);

        readCount = is.read(buffer, buffer.length, 0);
        assertEquals("Reading end of buffer length 0: expected readCount=0 but got " + readCount,
                0, readCount);

        is.close();
    }

    public void testGetFont_invalidResourceId() {
        try {
            mResources.getFont(-1);
            fail("Font resource -1 should not be found.");
        } catch (NotFoundException e) {
            //expected
        }
    }

    public void testGetFont_fontFile() {
        Typeface font = mResources.getFont(R.font.sample_regular_font);

        assertNotNull(font);
        assertNotSame(Typeface.DEFAULT, font);
    }

    public void testGetFont_xmlFile() {
        Typeface font = mResources.getFont(R.font.samplexmlfont);

        assertNotNull(font);
        assertNotSame(Typeface.DEFAULT, font);
    }

    private Typeface getLargerTypeface(String text, Typeface typeface1, Typeface typeface2) {
        Paint p1 = new Paint();
        p1.setTypeface(typeface1);
        float width1 = p1.measureText(text);
        Paint p2 = new Paint();
        p2.setTypeface(typeface2);
        float width2 = p2.measureText(text);

        if (width1 > width2) {
            return typeface1;
        } else if (width1 < width2) {
            return typeface2;
        } else {
            fail("The widths of the text should not be the same");
            return null;
        }
    }

    public void testGetFont_xmlFileWithTtc() {
        // Here we test that building typefaces by indexing in font collections works correctly.
        // We want to ensure that the built typefaces correspond to the fonts with the right index.
        // sample_font_collection.ttc contains two fonts (with indices 0 and 1). The first one has
        // glyph "a" of 3em width, and all the other glyphs 1em. The second one has glyph "b" of
        // 3em width, and all the other glyphs 1em. Hence, we can compare the width of these
        // glyphs to assert that ttc indexing works.
        Typeface normalFont = mResources.getFont(R.font.sample_ttc_family);
        assertNotNull(normalFont);
        Typeface italicFont = Typeface.create(normalFont, Typeface.ITALIC);
        assertNotNull(italicFont);

        assertEquals(getLargerTypeface("a", normalFont, italicFont), normalFont);
        assertEquals(getLargerTypeface("b", normalFont, italicFont), italicFont);
    }

    public void testGetFont_xmlFileWithVariationSettings() {
        // Here we test that specifying variation settings for fonts in XMLs works.
        // We build typefaces from two families containing one font each, using the same font
        // resource, but having different values for the 'wdth' tag. Then we measure the painted
        // text to ensure that the tag affects the text width. The font resource used supports
        // the 'wdth' axis for the dash (-) character.
        Typeface typeface1 = mResources.getFont(R.font.sample_variation_settings_family1);
        assertNotNull(typeface1);
        Typeface typeface2 = mResources.getFont(R.font.sample_variation_settings_family2);
        assertNotNull(typeface2);

        assertNotSame(typeface1, typeface2);
        assertEquals(getLargerTypeface("-", typeface1, typeface2), typeface2);
    }

    public void testGetFont_invalidXmlFile() {
        try {
            assertNull(mResources.getFont(R.font.invalid_xmlfamily));
        } catch (NotFoundException e) {
            // pass
        }

        try {
            assertNull(mResources.getFont(R.font.invalid_xmlempty));
        } catch (NotFoundException e) {
            // pass
        }
    }

    public void testGetFont_invalidFontFiles() {
        try {
            mResources.getFont(R.font.invalid_xmlfont);
            fail();
        } catch (RuntimeException e) {
            // pass
        }

        try {
            mResources.getFont(R.font.invalid_font);
            fail();
        } catch (RuntimeException e) {
            // pass
        }

        try {
            mResources.getFont(R.font.invalid_xmlfont_contains_invalid_font_file);
            fail();
        } catch (RuntimeException e) {
            // pass
        }

        try {
            mResources.getFont(R.font.invalid_xmlfont_nosource);
            fail();
        } catch (RuntimeException e) {
            // pass
        }

    }

    public void testGetFont_brokenFontFiles() {
        try {
            mResources.getFont(R.font.brokenfont);
            fail();
        } catch (RuntimeException e) {
            // pass
        }

        try {
            mResources.getFont(R.font.broken_xmlfont);
            fail();
        } catch (RuntimeException e) {
            // pass
        }
    }

    public void testGetFont_fontFileIsCached() {
        Typeface font = mResources.getFont(R.font.sample_regular_font);
        Typeface font2 = mResources.getFont(R.font.sample_regular_font);

        assertEquals(font, font2);
    }

    public void testGetFont_xmlFileIsCached() {
        Typeface font = mResources.getFont(R.font.samplexmlfont);
        Typeface font2 = mResources.getFont(R.font.samplexmlfont);

        assertEquals(font, font2);
    }

    public void testGetFont_resolveByFontTable() {
        assertEquals(Typeface.NORMAL, mResources.getFont(R.font.sample_regular_font).getStyle());
        assertEquals(Typeface.BOLD, mResources.getFont(R.font.sample_bold_font).getStyle());
        assertEquals(Typeface.ITALIC, mResources.getFont(R.font.sample_italic_font).getStyle());
        assertEquals(Typeface.BOLD_ITALIC,
                mResources.getFont(R.font.sample_bolditalic_font).getStyle());

        assertEquals(Typeface.NORMAL, mResources.getFont(R.font.sample_regular_family).getStyle());
        assertEquals(Typeface.BOLD, mResources.getFont(R.font.sample_bold_family).getStyle());
        assertEquals(Typeface.ITALIC, mResources.getFont(R.font.sample_italic_family).getStyle());
        assertEquals(Typeface.BOLD_ITALIC,
                mResources.getFont(R.font.sample_bolditalic_family).getStyle());
    }
}
