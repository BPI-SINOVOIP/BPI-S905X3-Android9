/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.content.res;

import com.android.SdkConstants;
import com.android.ide.common.rendering.api.ArrayResourceValue;
import com.android.ide.common.rendering.api.DensityBasedResourceValue;
import com.android.ide.common.rendering.api.LayoutLog;
import com.android.ide.common.rendering.api.LayoutlibCallback;
import com.android.ide.common.rendering.api.PluralsResourceValue;
import com.android.ide.common.rendering.api.RenderResources;
import com.android.ide.common.rendering.api.ResourceValue;
import com.android.layoutlib.bridge.Bridge;
import com.android.layoutlib.bridge.BridgeConstants;
import com.android.layoutlib.bridge.android.BridgeContext;
import com.android.layoutlib.bridge.android.BridgeXmlBlockParser;
import com.android.layoutlib.bridge.impl.ParserFactory;
import com.android.layoutlib.bridge.impl.ResourceHelper;
import com.android.layoutlib.bridge.util.NinePatchInputStream;
import com.android.ninepatch.NinePatch;
import com.android.resources.ResourceType;
import com.android.resources.ResourceUrl;
import com.android.tools.layoutlib.annotations.LayoutlibDelegate;
import com.android.tools.layoutlib.annotations.VisibleForTesting;
import com.android.util.Pair;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.res.Resources.NotFoundException;
import android.content.res.Resources.Theme;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.icu.text.PluralRules;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.LruCache;
import android.util.TypedValue;
import android.view.DisplayAdjustments;
import android.view.ViewGroup.LayoutParams;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Iterator;
import java.util.Objects;
import java.util.WeakHashMap;

import static com.android.SdkConstants.ANDROID_PKG;
import static com.android.SdkConstants.PREFIX_RESOURCE_REF;

@SuppressWarnings("deprecation")
public class Resources_Delegate {
    private static WeakHashMap<Resources, LayoutlibCallback> sLayoutlibCallbacks = new
            WeakHashMap<>();
    private static WeakHashMap<Resources, BridgeContext> sContexts = new
            WeakHashMap<>();

    private static boolean[] mPlatformResourceFlag = new boolean[1];
    // TODO: This cache is cleared every time a render session is disposed. Look into making this
    // more long lived.
    private static LruCache<String, Drawable.ConstantState> sDrawableCache = new LruCache<>(50);

    public static Resources initSystem(@NonNull BridgeContext context,
            @NonNull AssetManager assets,
            @NonNull DisplayMetrics metrics,
            @NonNull Configuration config,
            @NonNull LayoutlibCallback layoutlibCallback) {
        assert Resources.mSystem == null  :
                "Resources_Delegate.initSystem called twice before disposeSystem was called";
        Resources resources = new Resources(Resources_Delegate.class.getClassLoader());
        resources.setImpl(new ResourcesImpl(assets, metrics, config, new DisplayAdjustments()));
        sContexts.put(resources, Objects.requireNonNull(context));
        sLayoutlibCallbacks.put(resources, Objects.requireNonNull(layoutlibCallback));
        return Resources.mSystem = resources;
    }

    /** Returns the {@link BridgeContext} associated to the given {@link Resources} */
    @VisibleForTesting
    @NonNull
    public static BridgeContext getContext(@NonNull Resources resources) {
        assert sContexts.containsKey(resources) :
                "Resources_Delegate.getContext called before initSystem";
        return sContexts.get(resources);
    }

    /** Returns the {@link LayoutlibCallback} associated to the given {@link Resources} */
    @VisibleForTesting
    @NonNull
    public static LayoutlibCallback getLayoutlibCallback(@NonNull Resources resources) {
        assert sLayoutlibCallbacks.containsKey(resources) :
                "Resources_Delegate.getLayoutlibCallback called before initSystem";
        return sLayoutlibCallbacks.get(resources);
    }

    /**
     * Disposes the static {@link Resources#mSystem} to make sure we don't leave objects around that
     * would prevent us from unloading the library.
     */
    public static void disposeSystem() {
        sDrawableCache.evictAll();
        sContexts.clear();
        sLayoutlibCallbacks.clear();
        Resources.mSystem = null;
    }

    public static BridgeTypedArray newTypeArray(Resources resources, int numEntries,
            boolean platformFile) {
        return new BridgeTypedArray(resources, getContext(resources), numEntries, platformFile);
    }

    private static Pair<ResourceType, String> getResourceInfo(Resources resources, int id,
            boolean[] platformResFlag_out) {
        // first get the String related to this id in the framework
        Pair<ResourceType, String> resourceInfo = Bridge.resolveResourceId(id);

        assert Resources.mSystem != null : "Resources_Delegate.initSystem wasn't called";
        // Set the layoutlib callback and context for resources
        if (resources != Resources.mSystem &&
                (!sContexts.containsKey(resources) || !sLayoutlibCallbacks.containsKey(resources))) {
            sLayoutlibCallbacks.put(resources, getLayoutlibCallback(Resources.mSystem));
            sContexts.put(resources, getContext(Resources.mSystem));
        }

        if (resourceInfo != null) {
            platformResFlag_out[0] = true;
            return resourceInfo;
        }

        // didn't find a match in the framework? look in the project.
        resourceInfo = getLayoutlibCallback(resources).resolveResourceId(id);

        if (resourceInfo != null) {
            platformResFlag_out[0] = false;
            return resourceInfo;
        }
        return null;
    }

    private static Pair<String, ResourceValue> getResourceValue(Resources resources, int id,
            boolean[] platformResFlag_out) {
        Pair<ResourceType, String> resourceInfo =
                getResourceInfo(resources, id, platformResFlag_out);

        if (resourceInfo != null) {
            String attributeName = resourceInfo.getSecond();
            RenderResources renderResources = getContext(resources).getRenderResources();
            ResourceValue value = platformResFlag_out[0] ?
                    renderResources.getFrameworkResource(resourceInfo.getFirst(), attributeName) :
                    renderResources.getProjectResource(resourceInfo.getFirst(), attributeName);

            if (value == null) {
                // Unable to resolve the attribute, just leave the unresolved value
                value = new ResourceValue(resourceInfo.getFirst(), attributeName, attributeName,
                        platformResFlag_out[0]);
            }
            return Pair.of(attributeName, value);
        }

        return null;
    }

    @LayoutlibDelegate
    static Drawable getDrawable(Resources resources, int id) {
        return getDrawable(resources, id, null);
    }

    @LayoutlibDelegate
    static Drawable getDrawable(Resources resources, int id, Theme theme) {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);
        if (value != null) {
            String key = value.getSecond().getValue();

            Drawable.ConstantState constantState = key != null ? sDrawableCache.get(key) : null;
            Drawable drawable;
            if (constantState != null) {
                drawable = constantState.newDrawable(resources, theme);
            } else {
                drawable =
                        ResourceHelper.getDrawable(value.getSecond(), getContext(resources), theme);

                if (key != null) {
                    sDrawableCache.put(key, drawable.getConstantState());
                }
            }

            return drawable;
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static int getColor(Resources resources, int id) {
        return getColor(resources, id, null);
    }

    @LayoutlibDelegate
    static int getColor(Resources resources, int id, Theme theme) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resourceValue = value.getSecond();
            try {
                return ResourceHelper.getColor(resourceValue.getValue());
            } catch (NumberFormatException e) {
                // Check if the value passed is a file. If it is, mostly likely, user is referencing
                // a color state list from a place where they should reference only a pure color.
                String message;
                if (new File(resourceValue.getValue()).isFile()) {
                    String resource = (resourceValue.isFramework() ? "@android:" : "@") + "color/"
                            + resourceValue.getName();
                    message = "Hexadecimal color expected, found Color State List for " + resource;
                } else {
                    message = e.getMessage();
                }
                Bridge.getLog().error(LayoutLog.TAG_RESOURCES_FORMAT, message, e, null);
                return 0;
            }
        }

        // Suppress possible NPE. getColorStateList will never return null, it will instead
        // throw an exception, but intelliJ can't figure that out
        //noinspection ConstantConditions
        return getColorStateList(resources, id, theme).getDefaultColor();
    }

    @LayoutlibDelegate
    static ColorStateList getColorStateList(Resources resources, int id) throws NotFoundException {
        return getColorStateList(resources, id, null);
    }

    @LayoutlibDelegate
    static ColorStateList getColorStateList(Resources resources, int id, Theme theme)
            throws NotFoundException {
        Pair<String, ResourceValue> resValue =
                getResourceValue(resources, id, mPlatformResourceFlag);

        if (resValue != null) {
            ColorStateList stateList = ResourceHelper.getColorStateList(resValue.getSecond(),
                    getContext(resources), theme);
            if (stateList != null) {
                return stateList;
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static CharSequence getText(Resources resources, int id, CharSequence def) {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    return v;
                }
            }
        }

        return def;
    }

    @LayoutlibDelegate
    static CharSequence getText(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    return v;
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static CharSequence[] getTextArray(Resources resources, int id) throws NotFoundException {
        ResourceValue resValue = getArrayResourceValue(resources, id);
        if (resValue == null) {
            // Error already logged by getArrayResourceValue.
            return new CharSequence[0];
        } else if (!(resValue instanceof ArrayResourceValue)) {
            return new CharSequence[]{
                    resolveReference(resources, resValue.getValue(), resValue.isFramework())};
        }
        ArrayResourceValue arv = ((ArrayResourceValue) resValue);
        return fillValues(resources, arv, new CharSequence[arv.getElementCount()]);
    }

    @LayoutlibDelegate
    static String[] getStringArray(Resources resources, int id) throws NotFoundException {
        ResourceValue resValue = getArrayResourceValue(resources, id);
        if (resValue == null) {
            // Error already logged by getArrayResourceValue.
            return new String[0];
        } else if (!(resValue instanceof ArrayResourceValue)) {
            return new String[]{
                    resolveReference(resources, resValue.getValue(), resValue.isFramework())};
        }
        ArrayResourceValue arv = ((ArrayResourceValue) resValue);
        return fillValues(resources, arv, new String[arv.getElementCount()]);
    }

    /**
     * Resolve each element in resValue and copy them to {@code values}. The values copied are
     * always Strings. The ideal signature for the method should be &lt;T super String&gt;, but java
     * generics don't support it.
     */
    static <T extends CharSequence> T[] fillValues(Resources resources, ArrayResourceValue resValue,
            T[] values) {
        int i = 0;
        for (Iterator<String> iterator = resValue.iterator(); iterator.hasNext(); i++) {
            @SuppressWarnings("unchecked")
            T s = (T) resolveReference(resources, iterator.next(), resValue.isFramework());
            values[i] = s;
        }
        return values;
    }

    @LayoutlibDelegate
    static int[] getIntArray(Resources resources, int id) throws NotFoundException {
        ResourceValue rv = getArrayResourceValue(resources, id);
        if (rv == null) {
            // Error already logged by getArrayResourceValue.
            return new int[0];
        } else if (!(rv instanceof ArrayResourceValue)) {
            // This is an older IDE that can only give us the first element of the array.
            String firstValue = resolveReference(resources, rv.getValue(), rv.isFramework());
            try {
                return new int[]{getInt(firstValue)};
            } catch (NumberFormatException e) {
                Bridge.getLog().error(LayoutLog.TAG_RESOURCES_FORMAT,
                        "Integer resource array contains non-integer value: " +
                                firstValue, null);
                return new int[1];
            }
        }
        ArrayResourceValue resValue = ((ArrayResourceValue) rv);
        int[] values = new int[resValue.getElementCount()];
        int i = 0;
        for (Iterator<String> iterator = resValue.iterator(); iterator.hasNext(); i++) {
            String element = resolveReference(resources, iterator.next(), resValue.isFramework());
            try {
                if (element.startsWith("#")) {
                    // This integer represents a color (starts with #)
                    values[i] = Color.parseColor(element);
                } else {
                    values[i] = getInt(element);
                }
            } catch (NumberFormatException e) {
                Bridge.getLog().error(LayoutLog.TAG_RESOURCES_FORMAT,
                        "Integer resource array contains non-integer value: " + element, null);
            } catch (IllegalArgumentException e2) {
                Bridge.getLog().error(LayoutLog.TAG_RESOURCES_FORMAT,
                        "Integer resource array contains wrong color format: " + element, null);
            }
        }
        return values;
    }

    /**
     * Try to find the ArrayResourceValue for the given id.
     * <p/>
     * If the ResourceValue found is not of type {@link ResourceType#ARRAY}, the method logs an
     * error and return null. However, if the ResourceValue found has type {@code
     * ResourceType.ARRAY}, but the value is not an instance of {@link ArrayResourceValue}, the
     * method returns the ResourceValue. This happens on older versions of the IDE, which did not
     * parse the array resources properly.
     * <p/>
     *
     * @throws NotFoundException if no resource if found
     */
    @Nullable
    private static ResourceValue getArrayResourceValue(Resources resources, int id)
            throws NotFoundException {
        Pair<String, ResourceValue> v = getResourceValue(resources, id, mPlatformResourceFlag);

        if (v != null) {
            ResourceValue resValue = v.getSecond();

            assert resValue != null;
            if (resValue != null) {
                final ResourceType type = resValue.getResourceType();
                if (type != ResourceType.ARRAY) {
                    Bridge.getLog().error(LayoutLog.TAG_RESOURCES_RESOLVE,
                            String.format(
                                    "Resource with id 0x%1$X is not an array resource, but %2$s",
                                    id, type == null ? "null" : type.getDisplayName()),
                            null);
                    return null;
                }
                if (!(resValue instanceof ArrayResourceValue)) {
                    Bridge.getLog().warning(LayoutLog.TAG_UNSUPPORTED,
                            "Obtaining resource arrays via getTextArray, getStringArray or getIntArray is not fully supported in this version of the IDE.",
                            null);
                }
                return resValue;
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @NonNull
    private static String resolveReference(Resources resources, @NonNull String ref,
            boolean forceFrameworkOnly) {
        if (ref.startsWith(PREFIX_RESOURCE_REF) || ref.startsWith
                (SdkConstants.PREFIX_THEME_REF)) {
            ResourceValue rv =
                    getContext(resources).getRenderResources().findResValue(ref, forceFrameworkOnly);
            rv = getContext(resources).getRenderResources().resolveResValue(rv);
            if (rv != null) {
                return rv.getValue();
            }
        }
        // Not a reference.
        return ref;
    }

    @LayoutlibDelegate
    static XmlResourceParser getLayout(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> v = getResourceValue(resources, id, mPlatformResourceFlag);

        if (v != null) {
            ResourceValue value = v.getSecond();

            try {
                return ResourceHelper.getXmlBlockParser(getContext(resources), value);
            } catch (XmlPullParserException e) {
                Bridge.getLog().error(LayoutLog.TAG_BROKEN,
                        "Failed to configure parser for " + value.getValue(), e, null /*data*/);
                // we'll return null below.
            } catch (FileNotFoundException e) {
                // this shouldn't happen since we check above.
            }

        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static XmlResourceParser getAnimation(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> v = getResourceValue(resources, id, mPlatformResourceFlag);

        if (v != null) {
            ResourceValue value = v.getSecond();

            try {
                return ResourceHelper.getXmlBlockParser(getContext(resources), value);
            } catch (XmlPullParserException e) {
                Bridge.getLog().error(LayoutLog.TAG_BROKEN,
                        "Failed to configure parser for " + value.getValue(), e, null /*data*/);
                // we'll return null below.
            } catch (FileNotFoundException e) {
                // this shouldn't happen since we check above.
            }

        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static TypedArray obtainAttributes(Resources resources, AttributeSet set, int[] attrs) {
        return getContext(resources).obtainStyledAttributes(set, attrs);
    }

    @LayoutlibDelegate
    static TypedArray obtainAttributes(Resources resources, Resources.Theme theme, AttributeSet
            set, int[] attrs) {
        return Resources.obtainAttributes_Original(resources, theme, set, attrs);
    }

    @LayoutlibDelegate
    static TypedArray obtainTypedArray(Resources resources, int id) throws NotFoundException {
        throw new UnsupportedOperationException();
    }

    @LayoutlibDelegate
    static float getDimension(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    if (v.equals(BridgeConstants.MATCH_PARENT) ||
                            v.equals(BridgeConstants.FILL_PARENT)) {
                        return LayoutParams.MATCH_PARENT;
                    } else if (v.equals(BridgeConstants.WRAP_CONTENT)) {
                        return LayoutParams.WRAP_CONTENT;
                    }
                    TypedValue tmpValue = new TypedValue();
                    if (ResourceHelper.parseFloatAttribute(
                            value.getFirst(), v, tmpValue, true /*requireUnit*/) &&
                            tmpValue.type == TypedValue.TYPE_DIMENSION) {
                        return tmpValue.getDimension(resources.getDisplayMetrics());
                    }
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return 0;
    }

    @LayoutlibDelegate
    static int getDimensionPixelOffset(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    TypedValue tmpValue = new TypedValue();
                    if (ResourceHelper.parseFloatAttribute(
                            value.getFirst(), v, tmpValue, true /*requireUnit*/) &&
                            tmpValue.type == TypedValue.TYPE_DIMENSION) {
                        return TypedValue.complexToDimensionPixelOffset(tmpValue.data,
                                resources.getDisplayMetrics());
                    }
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return 0;
    }

    @LayoutlibDelegate
    static int getDimensionPixelSize(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    TypedValue tmpValue = new TypedValue();
                    if (ResourceHelper.parseFloatAttribute(
                            value.getFirst(), v, tmpValue, true /*requireUnit*/) &&
                            tmpValue.type == TypedValue.TYPE_DIMENSION) {
                        return TypedValue.complexToDimensionPixelSize(tmpValue.data,
                                resources.getDisplayMetrics());
                    }
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return 0;
    }

    @LayoutlibDelegate
    static int getInteger(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            assert resValue != null;
            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    try {
                        return getInt(v);
                    } catch (NumberFormatException e) {
                        // return exception below
                    }
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return 0;
    }

    @LayoutlibDelegate
    static float getFloat(Resources resources, int id) {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    try {
                        return Float.parseFloat(v);
                    } catch (NumberFormatException ignore) {
                    }
                }
            }
        }
        return 0;
    }

    @LayoutlibDelegate
    static boolean getBoolean(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resValue = value.getSecond();

            if (resValue != null) {
                String v = resValue.getValue();
                if (v != null) {
                    return Boolean.parseBoolean(v);
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return false;
    }

    @LayoutlibDelegate
    static String getResourceEntryName(Resources resources, int resid) throws NotFoundException {
        Pair<ResourceType, String> resourceInfo = getResourceInfo(resources, resid, new boolean[1]);
        if (resourceInfo != null) {
            return resourceInfo.getSecond();
        }
        throwException(resid, null);
        return null;

    }

    @LayoutlibDelegate
    static String getResourceName(Resources resources, int resid) throws NotFoundException {
        boolean[] platformOut = new boolean[1];
        Pair<ResourceType, String> resourceInfo = getResourceInfo(resources, resid, platformOut);
        String packageName;
        if (resourceInfo != null) {
            if (platformOut[0]) {
                packageName = SdkConstants.ANDROID_NS_NAME;
            } else {
                packageName = getContext(resources).getPackageName();
                packageName = packageName == null ? SdkConstants.APP_PREFIX : packageName;
            }
            return packageName + ':' + resourceInfo.getFirst().getName() + '/' +
                    resourceInfo.getSecond();
        }
        throwException(resid, null);
        return null;
    }

    @LayoutlibDelegate
    static String getResourcePackageName(Resources resources, int resid) throws NotFoundException {
        boolean[] platformOut = new boolean[1];
        Pair<ResourceType, String> resourceInfo = getResourceInfo(resources, resid, platformOut);
        if (resourceInfo != null) {
            if (platformOut[0]) {
                return SdkConstants.ANDROID_NS_NAME;
            }
            String packageName = getContext(resources).getPackageName();
            return packageName == null ? SdkConstants.APP_PREFIX : packageName;
        }
        throwException(resid, null);
        return null;
    }

    @LayoutlibDelegate
    static String getResourceTypeName(Resources resources, int resid) throws NotFoundException {
        Pair<ResourceType, String> resourceInfo = getResourceInfo(resources, resid, new boolean[1]);
        if (resourceInfo != null) {
            return resourceInfo.getFirst().getName();
        }
        throwException(resid, null);
        return null;
    }

    @LayoutlibDelegate
    static String getString(Resources resources, int id, Object... formatArgs)
            throws NotFoundException {
        String s = getString(resources, id);
        if (s != null) {
            return String.format(s, formatArgs);

        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static String getString(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null && value.getSecond().getValue() != null) {
            return value.getSecond().getValue();
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static String getQuantityString(Resources resources, int id, int quantity) throws
            NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            if (value.getSecond() instanceof PluralsResourceValue) {
                PluralsResourceValue pluralsResourceValue = (PluralsResourceValue) value.getSecond();
                PluralRules pluralRules = PluralRules.forLocale(resources.getConfiguration().getLocales()
                        .get(0));
                String strValue = pluralsResourceValue.getValue(pluralRules.select(quantity));
                if (strValue == null) {
                    strValue = pluralsResourceValue.getValue(PluralRules.KEYWORD_OTHER);
                }

                return strValue;
            }
            else {
                return value.getSecond().getValue();
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static String getQuantityString(Resources resources, int id, int quantity, Object... formatArgs)
            throws NotFoundException {
        String raw = getQuantityString(resources, id, quantity);
        return String.format(resources.getConfiguration().getLocales().get(0), raw, formatArgs);
    }

    @LayoutlibDelegate
    static CharSequence getQuantityText(Resources resources, int id, int quantity) throws
            NotFoundException {
        return getQuantityString(resources, id, quantity);
    }

    @LayoutlibDelegate
    static Typeface getFont(Resources resources, int id) throws
            NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);
        if (value != null) {
            return ResourceHelper.getFont(value.getSecond(), getContext(resources), null);
        }

        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static Typeface getFont(Resources resources, TypedValue outValue, int id) throws
            NotFoundException {
        Resources_Delegate.getValue(resources, id, outValue, true);
        if (outValue.string != null) {
            return ResourceHelper.getFont(outValue.string.toString(), getContext(resources), null,
                    mPlatformResourceFlag[0]);
        }

        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static void getValue(Resources resources, int id, TypedValue outValue, boolean resolveRefs)
            throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            ResourceValue resVal = value.getSecond();
            String v = resVal != null ? resVal.getValue() : null;

            if (v != null) {
                if (ResourceHelper.parseFloatAttribute(value.getFirst(), v, outValue,
                        false /*requireUnit*/)) {
                    return;
                }
                if (resVal instanceof DensityBasedResourceValue) {
                    outValue.density =
                            ((DensityBasedResourceValue) resVal).getResourceDensity().getDpiValue();
                }

                // else it's a string
                outValue.type = TypedValue.TYPE_STRING;
                outValue.string = v;
                return;
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);
    }

    @LayoutlibDelegate
    static void getValue(Resources resources, String name, TypedValue outValue, boolean resolveRefs)
            throws NotFoundException {
        throw new UnsupportedOperationException();
    }

    @LayoutlibDelegate
    static void getValueForDensity(Resources resources, int id, int density, TypedValue outValue,
            boolean resolveRefs) throws NotFoundException {
        getValue(resources, id, outValue, resolveRefs);
    }

    @LayoutlibDelegate
    static XmlResourceParser getXml(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> v = getResourceValue(resources, id, mPlatformResourceFlag);

        if (v != null) {
            ResourceValue value = v.getSecond();

            try {
                return ResourceHelper.getXmlBlockParser(getContext(resources), value);
            } catch (XmlPullParserException e) {
                Bridge.getLog().error(LayoutLog.TAG_BROKEN,
                        "Failed to configure parser for " + value.getValue(), e, null /*data*/);
                // we'll return null below.
            } catch (FileNotFoundException e) {
                // this shouldn't happen since we check above.
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static XmlResourceParser loadXmlResourceParser(Resources resources, int id,
            String type) throws NotFoundException {
        return resources.loadXmlResourceParser_Original(id, type);
    }

    @LayoutlibDelegate
    static XmlResourceParser loadXmlResourceParser(Resources resources, String file, int id,
            int assetCookie, String type) throws NotFoundException {
        // even though we know the XML file to load directly, we still need to resolve the
        // id so that we can know if it's a platform or project resource.
        // (mPlatformResouceFlag will get the result and will be used later).
        getResourceValue(resources, id, mPlatformResourceFlag);

        File f = new File(file);
        try {
            XmlPullParser parser = ParserFactory.create(f);

            return new BridgeXmlBlockParser(parser, getContext(resources), mPlatformResourceFlag[0]);
        } catch (XmlPullParserException e) {
            NotFoundException newE = new NotFoundException();
            newE.initCause(e);
            throw newE;
        } catch (FileNotFoundException e) {
            NotFoundException newE = new NotFoundException();
            newE.initCause(e);
            throw newE;
        }
    }

    @LayoutlibDelegate
    static InputStream openRawResource(Resources resources, int id) throws NotFoundException {
        Pair<String, ResourceValue> value = getResourceValue(resources, id, mPlatformResourceFlag);

        if (value != null) {
            String path = value.getSecond().getValue();

            if (path != null) {
                // check this is a file
                File f = new File(path);
                if (f.isFile()) {
                    try {
                        // if it's a nine-patch return a custom input stream so that
                        // other methods (mainly bitmap factory) can detect it's a 9-patch
                        // and actually load it as a 9-patch instead of a normal bitmap
                        if (path.toLowerCase().endsWith(NinePatch.EXTENSION_9PATCH)) {
                            return new NinePatchInputStream(f);
                        }
                        return new FileInputStream(f);
                    } catch (FileNotFoundException e) {
                        NotFoundException newE = new NotFoundException();
                        newE.initCause(e);
                        throw newE;
                    }
                }
            }
        }

        // id was not found or not resolved. Throw a NotFoundException.
        throwException(resources, id);

        // this is not used since the method above always throws
        return null;
    }

    @LayoutlibDelegate
    static InputStream openRawResource(Resources resources, int id, TypedValue value) throws
            NotFoundException {
        getValue(resources, id, value, true);

        String path = value.string.toString();

        File f = new File(path);
        if (f.isFile()) {
            try {
                // if it's a nine-patch return a custom input stream so that
                // other methods (mainly bitmap factory) can detect it's a 9-patch
                // and actually load it as a 9-patch instead of a normal bitmap
                if (path.toLowerCase().endsWith(NinePatch.EXTENSION_9PATCH)) {
                    return new NinePatchInputStream(f);
                }
                return new FileInputStream(f);
            } catch (FileNotFoundException e) {
                NotFoundException exception = new NotFoundException();
                exception.initCause(e);
                throw exception;
            }
        }

        throw new NotFoundException();
    }

    @LayoutlibDelegate
    static AssetFileDescriptor openRawResourceFd(Resources resources, int id) throws
            NotFoundException {
        throw new UnsupportedOperationException();
    }

    @VisibleForTesting
    @Nullable
    static ResourceUrl resourceUrlFromName(@NonNull String name, @Nullable String defType,
            @Nullable
            String defPackage) {
        int colonIdx = name.indexOf(':');
        int slashIdx = name.indexOf('/');

        if (colonIdx != -1 && slashIdx != -1) {
            // Easy case
            return ResourceUrl.parse(PREFIX_RESOURCE_REF + name);
        }

        if (colonIdx == -1 && slashIdx == -1) {
            if (defType == null) {
                throw new IllegalArgumentException("name does not define a type an no defType was" +
                        " passed");
            }

            // It does not define package or type
            return ResourceUrl.parse(
                    PREFIX_RESOURCE_REF + (defPackage != null ? defPackage + ":" : "") + defType +
                            "/" + name);
        }

        if (colonIdx != -1) {
            if (defType == null) {
                throw new IllegalArgumentException("name does not define a type an no defType was" +
                        " passed");
            }
            // We have package but no type
            String pkg = name.substring(0, colonIdx);
            ResourceType type = ResourceType.getEnum(defType);
            return type != null ? ResourceUrl.create(pkg, type, name.substring(colonIdx + 1)) :
                    null;
        }

        ResourceType type = ResourceType.getEnum(name.substring(0, slashIdx));
        if (type == null) {
            return null;
        }
        // We have type but no package
        return ResourceUrl.create(defPackage,
                type,
                name.substring(slashIdx + 1));
    }

    @LayoutlibDelegate
    static int getIdentifier(Resources resources, String name, String defType, String defPackage) {
        if (name == null) {
            return 0;
        }

        ResourceUrl url = resourceUrlFromName(name, defType, defPackage);
        Integer id = null;
        if (url != null) {
            id = ANDROID_PKG.equals(url.namespace) ? Bridge.getResourceId(url.type, url.name) :
                    getLayoutlibCallback(resources).getResourceId(url.type, url.name);
        }

        return id != null ? id : 0;
    }

    /**
     * Builds and throws a {@link Resources.NotFoundException} based on a resource id and a resource
     * type.
     *
     * @param id the id of the resource
     *
     * @throws NotFoundException
     */
    private static void throwException(Resources resources, int id) throws NotFoundException {
        throwException(id, getResourceInfo(resources, id, new boolean[1]));
    }

    private static void throwException(int id, @Nullable Pair<ResourceType, String> resourceInfo) {
        String message;
        if (resourceInfo != null) {
            message = String.format(
                    "Could not find %1$s resource matching value 0x%2$X (resolved name: %3$s) in current configuration.",
                    resourceInfo.getFirst(), id, resourceInfo.getSecond());
        } else {
            message = String.format("Could not resolve resource value: 0x%1$X.", id);
        }

        throw new NotFoundException(message);
    }

    private static int getInt(String v) throws NumberFormatException {
        int radix = 10;
        if (v.startsWith("0x")) {
            v = v.substring(2);
            radix = 16;
        } else if (v.startsWith("0")) {
            radix = 8;
        }
        return Integer.parseInt(v, radix);
    }
}
