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

package com.android.tools.layoutlib.create;

import com.android.tools.layoutlib.annotations.LayoutlibDelegate;
import com.android.tools.layoutlib.java.AutoCloseable;
import com.android.tools.layoutlib.java.Charsets;
import com.android.tools.layoutlib.java.IntegralToString;
import com.android.tools.layoutlib.java.LinkedHashMap_Delegate;
import com.android.tools.layoutlib.java.Objects;
import com.android.tools.layoutlib.java.System_Delegate;
import com.android.tools.layoutlib.java.UnsafeByteSequence;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Describes the work to be done by {@link AsmGenerator}.
 */
public final class CreateInfo implements ICreateInfo {

    @Override
    public Class<?>[] getInjectedClasses() {
        return INJECTED_CLASSES;
    }

    @Override
    public String[] getDelegateMethods() {
        return DELEGATE_METHODS;
    }

    @Override
    public String[] getDelegateClassNatives() {
        return DELEGATE_CLASS_NATIVES;
    }

    @Override
    public String[] getRenamedClasses() {
        return RENAMED_CLASSES;
    }

    @Override
    public String[] getDeleteReturns() {
        return DELETE_RETURNS;
    }

    @Override
    public String[] getJavaPkgClasses() {
      return JAVA_PKG_CLASSES;
    }

    @Override
    public String[] getRefactoredClasses() {
        return REFACTOR_CLASSES;
    }

    @Override
    public Set<String> getExcludedClasses() {
        String[] refactoredClasses = getJavaPkgClasses();
        int count = refactoredClasses.length / 2 + EXCLUDED_CLASSES.length;
        Set<String> excludedClasses = new HashSet<>(count);
        for (int i = 0; i < refactoredClasses.length; i+=2) {
            excludedClasses.add(refactoredClasses[i]);
        }
        excludedClasses.addAll(Arrays.asList(EXCLUDED_CLASSES));
        return excludedClasses;
    }

    @Override
    public String[] getPromotedFields() {
        return PROMOTED_FIELDS;
    }

    @Override
    public String[] getPromotedClasses() {
        return PROMOTED_CLASSES;
    }

    @Override
    public Map<String, InjectMethodRunnable> getInjectedMethodsMap() {
        return INJECTED_METHODS;
    }

    //-----

    /**
     * The list of class from layoutlib_create to inject in layoutlib.
     */
    private final static Class<?>[] INJECTED_CLASSES = new Class<?>[] {
            OverrideMethod.class,
            MethodListener.class,
            MethodAdapter.class,
            ICreateInfo.class,
            CreateInfo.class,
            LayoutlibDelegate.class,
            InjectMethodRunnable.class,
            InjectMethodRunnables.class,
            /* Java package classes */
            IntegralToString.class,
            UnsafeByteSequence.class,
            System_Delegate.class,
            LinkedHashMap_Delegate.class,
        };

    /**
     * The list of methods to rewrite as delegates.
     */
    public final static String[] DELEGATE_METHODS = new String[] {
        "android.app.Fragment#instantiate", //(Landroid/content/Context;Ljava/lang/String;Landroid/os/Bundle;)Landroid/app/Fragment;",
        "android.content.res.Resources#getAnimation",
        "android.content.res.Resources#getBoolean",
        "android.content.res.Resources#getColor",
        "android.content.res.Resources#getColorStateList",
        "android.content.res.Resources#getDimension",
        "android.content.res.Resources#getDimensionPixelOffset",
        "android.content.res.Resources#getDimensionPixelSize",
        "android.content.res.Resources#getDrawable",
        "android.content.res.Resources#getFloat",
        "android.content.res.Resources#getFont",
        "android.content.res.Resources#getIdentifier",
        "android.content.res.Resources#getIntArray",
        "android.content.res.Resources#getInteger",
        "android.content.res.Resources#getLayout",
        "android.content.res.Resources#getQuantityString",
        "android.content.res.Resources#getQuantityText",
        "android.content.res.Resources#getResourceEntryName",
        "android.content.res.Resources#getResourceName",
        "android.content.res.Resources#getResourcePackageName",
        "android.content.res.Resources#getResourceTypeName",
        "android.content.res.Resources#getString",
        "android.content.res.Resources#getStringArray",
        "android.content.res.Resources#getText",
        "android.content.res.Resources#getTextArray",
        "android.content.res.Resources#getValue",
        "android.content.res.Resources#getValueForDensity",
        "android.content.res.Resources#getXml",
        "android.content.res.Resources#loadXmlResourceParser",
        "android.content.res.Resources#obtainAttributes",
        "android.content.res.Resources#obtainTypedArray",
        "android.content.res.Resources#openRawResource",
        "android.content.res.Resources#openRawResourceFd",
        "android.content.res.Resources$Theme#obtainStyledAttributes",
        "android.content.res.Resources$Theme#resolveAttribute",
        "android.content.res.Resources$Theme#resolveAttributes",
        "android.content.res.AssetManager#open",
        "android.content.res.AssetManager#newTheme",
        "android.content.res.AssetManager#deleteTheme",
        "android.content.res.AssetManager#getAssignedPackageIdentifiers",
        "android.content.res.TypedArray#getValueAt",
        "android.content.res.TypedArray#obtain",
        "android.graphics.BitmapFactory#finishDecode",
        "android.graphics.BitmapFactory#setDensityFromOptions",
        "android.graphics.drawable.AnimatedVectorDrawable$VectorDrawableAnimatorRT#useLastSeenTarget",
        "android.graphics.drawable.AnimatedVectorDrawable$VectorDrawableAnimatorRT#onDraw",
        "android.graphics.drawable.GradientDrawable#buildRing",
        "android.graphics.drawable.AdaptiveIconDrawable#<init>",
        "android.graphics.FontFamily#addFont",
        "android.graphics.Typeface#buildSystemFallback",
        "android.graphics.Typeface#create",
        "android.graphics.Typeface#createFontFamily",
        "android.os.Binder#getNativeBBinderHolder",
        "android.os.Binder#getNativeFinalizer",
        "android.os.Handler#sendMessageAtTime",
        "android.os.HandlerThread#run",
        "android.preference.Preference#getView",
        "android.text.format.DateFormat#is24HourFormat",
        "android.util.Xml#newPullParser",
        "android.view.Choreographer#getInstance",
        "android.view.Choreographer#getRefreshRate",
        "android.view.Choreographer#scheduleVsyncLocked",
        "android.view.Display#updateDisplayInfoLocked",
        "android.view.Display#getWindowManager",
        "android.view.HandlerActionQueue#postDelayed",
        "android.view.LayoutInflater#rInflate",
        "android.view.LayoutInflater#parseInclude",
        "android.view.View#draw",
        "android.view.View#layout",
        "android.view.View#measure",
        "android.view.View#getWindowToken",
        "android.view.View#isInEditMode",
        "android.view.ViewRootImpl#isInTouchMode",
        "android.view.WindowManagerGlobal#getWindowManagerService",
        "android.view.inputmethod.InputMethodManager#getInstance",
        "android.view.MenuInflater#registerMenu",
        "android.view.RenderNode#getMatrix",
        "android.view.RenderNode#nCreate",
        "android.view.RenderNode#nGetNativeFinalizer",
        "android.view.RenderNode#nSetElevation",
        "android.view.RenderNode#nGetElevation",
        "android.view.RenderNode#nSetTranslationX",
        "android.view.RenderNode#nGetTranslationX",
        "android.view.RenderNode#nSetTranslationY",
        "android.view.RenderNode#nGetTranslationY",
        "android.view.RenderNode#nSetTranslationZ",
        "android.view.RenderNode#nGetTranslationZ",
        "android.view.RenderNode#nSetRotation",
        "android.view.RenderNode#nGetRotation",
        "android.view.RenderNode#nSetLeft",
        "android.view.RenderNode#nSetTop",
        "android.view.RenderNode#nSetRight",
        "android.view.RenderNode#nSetBottom",
        "android.view.RenderNode#nSetLeftTopRightBottom",
        "android.view.RenderNode#nSetPivotX",
        "android.view.RenderNode#nGetPivotX",
        "android.view.RenderNode#nSetPivotY",
        "android.view.RenderNode#nGetPivotY",
        "android.view.RenderNode#nSetScaleX",
        "android.view.RenderNode#nGetScaleX",
        "android.view.RenderNode#nSetScaleY",
        "android.view.RenderNode#nGetScaleY",
        "android.view.RenderNode#nIsPivotExplicitlySet",
        "android.view.PointerIcon#loadResource",
        "android.view.ViewGroup#drawChild",
        "com.android.internal.view.menu.MenuBuilder#createNewMenuItem",
        "com.android.internal.util.XmlUtils#convertValueToInt",
        "dalvik.system.VMRuntime#newUnpaddedArray",
        "libcore.io.MemoryMappedFile#mmapRO",
        "libcore.io.MemoryMappedFile#close",
        "libcore.io.MemoryMappedFile#bigEndianIterator",
        "libcore.util.NativeAllocationRegistry#applyFreeFunction",
    };

    /**
     * The list of classes on which to delegate all native methods.
     */
    public final static String[] DELEGATE_CLASS_NATIVES = new String[] {
        "android.animation.PropertyValuesHolder",
        "android.graphics.BaseCanvas",
        "android.graphics.Bitmap",
        "android.graphics.BitmapFactory",
        "android.graphics.BitmapShader",
        "android.graphics.BlurMaskFilter",
        "android.graphics.Canvas",
        "android.graphics.ColorFilter",
        "android.graphics.ColorMatrixColorFilter",
        "android.graphics.ComposePathEffect",
        "android.graphics.ComposeShader",
        "android.graphics.CornerPathEffect",
        "android.graphics.DashPathEffect",
        "android.graphics.DiscretePathEffect",
        "android.graphics.DrawFilter",
        "android.graphics.EmbossMaskFilter",
        "android.graphics.FontFamily",
        "android.graphics.LightingColorFilter",
        "android.graphics.LinearGradient",
        "android.graphics.MaskFilter",
        "android.graphics.Matrix",
        "android.graphics.NinePatch",
        "android.graphics.Paint",
        "android.graphics.PaintFlagsDrawFilter",
        "android.graphics.Path",
        "android.graphics.PathDashPathEffect",
        "android.graphics.PathEffect",
        "android.graphics.PathMeasure",
        "android.graphics.PorterDuffColorFilter",
        "android.graphics.RadialGradient",
        "android.graphics.Region",
        "android.graphics.Shader",
        "android.graphics.SumPathEffect",
        "android.graphics.SweepGradient",
        "android.graphics.Typeface",
        "android.graphics.drawable.AnimatedVectorDrawable",
        "android.graphics.drawable.VectorDrawable",
        "android.os.SystemClock",
        "android.os.SystemProperties",
        "android.text.MeasuredParagraph",
        "android.text.StaticLayout",
        "android.util.PathParser",
        "android.view.Display",
        "com.android.internal.util.VirtualRefBasePtr",
        "com.android.internal.view.animation.NativeInterpolatorFactoryHelper",
        "libcore.icu.ICU",
    };

    /**
     *  The list of classes to rename, must be an even list: the binary FQCN
     *  of class to replace followed by the new FQCN.
     */
    private final static String[] RENAMED_CLASSES =
        new String[] {
            "android.os.ServiceManager",                       "android.os._Original_ServiceManager",
            "android.view.textservice.TextServicesManager",    "android.view.textservice._Original_TextServicesManager",
            "android.util.LruCache",                           "android.util._Original_LruCache",
            "android.view.SurfaceView",                        "android.view._Original_SurfaceView",
            "android.view.accessibility.AccessibilityManager", "android.view.accessibility._Original_AccessibilityManager",
            "android.webkit.WebView",                          "android.webkit._Original_WebView",
            "android.graphics.ImageDecoder",                   "android.graphics._Original_ImageDecoder",
        };

    /**
     * The list of class references to update, must be an even list: the binary
     * FQCN of class to replace followed by the new FQCN. The classes to
     * replace are to be excluded from the output.
     */
    private final static String[] JAVA_PKG_CLASSES =
        new String[] {
            "java.nio.charset.Charsets",                       "java.nio.charset.StandardCharsets",
            "java.lang.IntegralToString",                      "com.android.tools.layoutlib.java.IntegralToString",
            "java.lang.UnsafeByteSequence",                    "com.android.tools.layoutlib.java.UnsafeByteSequence",
            // Use android.icu.text versions of DateFormat and SimpleDateFormat since the
            // original ones do not match the Android implementation
            "java.text.DateFormat",                            "android.icu.text.DateFormat",
            "java.text.SimpleDateFormat",                      "android.icu.text.SimpleDateFormat",
        };

    /**
     * List of classes to refactor. This is similar to combining {@link #getRenamedClasses()} and
     * {@link #getJavaPkgClasses()}.
     * Classes included here will be renamed and then all their references in any other classes
     * will be also modified.
     * FQCN of class to refactor followed by its new FQCN.
     */
    private final static String[] REFACTOR_CLASSES =
            new String[] {
                    "android.os.Build",                                "android.os._Original_Build",
            };

    private final static String[] EXCLUDED_CLASSES =
        new String[] {
            "android.preference.PreferenceActivity",
            "org.kxml2.io.KXmlParser",
        };

    /**
     * List of fields for which we will update the visibility to be public. This is sometimes
     * needed when access from the delegate classes is needed.
     */
    private final static String[] PROMOTED_FIELDS = new String[] {
        "android.graphics.drawable.VectorDrawable#mVectorState",
        "android.view.Choreographer#mLastFrameTimeNanos",
        "android.graphics.FontFamily#mBuilderPtr",
        "android.graphics.Typeface#sDynamicTypefaceCache",
        "android.graphics.drawable.AdaptiveIconDrawable#sMask",
    };

    /**
     * List of classes to be promoted to public visibility. Prefer using PROMOTED_FIELDS to this
     * if possible.
     */
    private final static String[] PROMOTED_CLASSES = new String[] {
    };

    /**
     * List of classes for which the methods returning them should be deleted.
     * The array contains a list of null terminated section starting with the name of the class
     * to rename in which the methods are deleted, followed by a list of return types identifying
     * the methods to delete.
     */
    private final static String[] DELETE_RETURNS =
        new String[] {
            null };                         // separator, for next class/methods list.

    private final static Map<String, InjectMethodRunnable> INJECTED_METHODS =
            new HashMap<String, InjectMethodRunnable>(1) {{
                put("android.content.Context",
                        InjectMethodRunnables.CONTEXT_GET_FRAMEWORK_CLASS_LOADER);
            }};
}
