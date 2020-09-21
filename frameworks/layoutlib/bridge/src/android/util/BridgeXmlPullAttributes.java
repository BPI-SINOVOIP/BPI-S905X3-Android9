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

package android.util;

import com.android.ide.common.rendering.api.AttrResourceValue;
import com.android.ide.common.rendering.api.RenderResources;
import com.android.ide.common.rendering.api.ResourceValue;
import com.android.internal.util.XmlUtils;
import com.android.layoutlib.bridge.Bridge;
import com.android.layoutlib.bridge.BridgeConstants;
import com.android.layoutlib.bridge.android.BridgeContext;
import com.android.layoutlib.bridge.impl.ResourceHelper;
import com.android.resources.ResourceType;

import org.xmlpull.v1.XmlPullParser;

import android.annotation.NonNull;

import java.util.Map;
import java.util.function.Function;

/**
 * A correct implementation of the {@link AttributeSet} interface on top of a XmlPullParser
 */
public class BridgeXmlPullAttributes extends XmlPullAttributes {

    private final BridgeContext mContext;
    private final boolean mPlatformFile;
    private final Function<String, Map<String, Integer>> mFrameworkEnumValueSupplier;
    private final Function<String, Map<String, Integer>> mProjectEnumValueSupplier;

    // VisibleForTesting
    BridgeXmlPullAttributes(@NonNull XmlPullParser parser, @NonNull BridgeContext context,
            boolean platformFile,
            @NonNull Function<String, Map<String, Integer>> frameworkEnumValueSupplier,
            @NonNull Function<String, Map<String, Integer>> projectEnumValueSupplier) {
        super(parser);
        mContext = context;
        mPlatformFile = platformFile;
        mFrameworkEnumValueSupplier = frameworkEnumValueSupplier;
        mProjectEnumValueSupplier = projectEnumValueSupplier;
    }

    public BridgeXmlPullAttributes(@NonNull XmlPullParser parser, @NonNull BridgeContext context,
            boolean platformFile) {
        this(parser, context, platformFile, Bridge::getEnumValues, attrName -> {
            // get the styleable matching the resolved name
            RenderResources res = context.getRenderResources();
            ResourceValue attr = res.getProjectResource(ResourceType.ATTR, attrName);
            return attr instanceof AttrResourceValue ?
                    ((AttrResourceValue) attr).getAttributeValues() : null;
        });
    }

    /*
     * (non-Javadoc)
     * @see android.util.XmlPullAttributes#getAttributeNameResource(int)
     *
     * This methods must return com.android.internal.R.attr.<name> matching
     * the name of the attribute.
     * It returns 0 if it doesn't find anything.
     */
    @Override
    public int getAttributeNameResource(int index) {
        // get the attribute name.
        String name = getAttributeName(index);

        // get the attribute namespace
        String ns = mParser.getAttributeNamespace(index);

        if (BridgeConstants.NS_RESOURCES.equals(ns)) {
            return Bridge.getResourceId(ResourceType.ATTR, name);

        }

        // this is not an attribute in the android namespace, we query the customviewloader, if
        // the namespaces match.
        if (mContext.getLayoutlibCallback().getNamespace().equals(ns)) {
            Integer v = mContext.getLayoutlibCallback().getResourceId(ResourceType.ATTR, name);
            if (v != null) {
                return v;
            }
        }

        return 0;
    }

    @Override
    public int getAttributeListValue(String namespace, String attribute,
            String[] options, int defaultValue) {
        String value = getAttributeValue(namespace, attribute);
        if (value != null) {
            ResourceValue r = getResourceValue(value);

            if (r != null) {
                value = r.getValue();
            }

            return XmlUtils.convertValueToList(value, options, defaultValue);
        }

        return defaultValue;
    }

    @Override
    public boolean getAttributeBooleanValue(String namespace, String attribute,
            boolean defaultValue) {
        String value = getAttributeValue(namespace, attribute);
        if (value != null) {
            ResourceValue r = getResourceValue(value);

            if (r != null) {
                value = r.getValue();
            }

            return XmlUtils.convertValueToBoolean(value, defaultValue);
        }

        return defaultValue;
    }

    @Override
    public int getAttributeResourceValue(String namespace, String attribute, int defaultValue) {
        String value = getAttributeValue(namespace, attribute);

        return resolveResourceValue(value, defaultValue);
    }

    @Override
    public int getAttributeIntValue(String namespace, String attribute, int defaultValue) {
        String value = getAttributeValue(namespace, attribute);
        if (value == null) {
            return defaultValue;
        }

        ResourceValue r = getResourceValue(value);

        if (r != null) {
            value = r.getValue();
        }

        if (value.charAt(0) == '#') {
            return ResourceHelper.getColor(value);
        }

        try {
            return XmlUtils.convertValueToInt(value, defaultValue);
        } catch (NumberFormatException e) {
            // This is probably an enum
            Map<String, Integer> enumValues = BridgeConstants.NS_RESOURCES.equals(namespace) ?
                    mFrameworkEnumValueSupplier.apply(attribute) :
                    mProjectEnumValueSupplier.apply(attribute);

            Integer enumValue = enumValues != null ? enumValues.get(value) : null;
            if (enumValue != null) {
                return enumValue;
            }

            // We weren't able to find the enum int value
            throw e;
        }
    }

    @Override
    public int getAttributeUnsignedIntValue(String namespace, String attribute,
            int defaultValue) {
        String value = getAttributeValue(namespace, attribute);
        if (value != null) {
            ResourceValue r = getResourceValue(value);

            if (r != null) {
                value = r.getValue();
            }

            return XmlUtils.convertValueToUnsignedInt(value, defaultValue);
        }

        return defaultValue;
    }

    @Override
    public float getAttributeFloatValue(String namespace, String attribute,
            float defaultValue) {
        String s = getAttributeValue(namespace, attribute);
        if (s != null) {
            ResourceValue r = getResourceValue(s);

            if (r != null) {
                s = r.getValue();
            }

            return Float.parseFloat(s);
        }

        return defaultValue;
    }

    @Override
    public int getAttributeListValue(int index,
            String[] options, int defaultValue) {
        return XmlUtils.convertValueToList(
            getAttributeValue(index), options, defaultValue);
    }

    @Override
    public boolean getAttributeBooleanValue(int index, boolean defaultValue) {
        String value = getAttributeValue(index);
        if (value != null) {
            ResourceValue r = getResourceValue(value);

            if (r != null) {
                value = r.getValue();
            }

            return XmlUtils.convertValueToBoolean(value, defaultValue);
        }

        return defaultValue;
    }

    @Override
    public int getAttributeResourceValue(int index, int defaultValue) {
        String value = getAttributeValue(index);

        return resolveResourceValue(value, defaultValue);
    }

    @Override
    public int getAttributeIntValue(int index, int defaultValue) {
        return getAttributeIntValue(mParser.getAttributeNamespace(index),
                getAttributeName(index)
                , defaultValue);
    }

    @Override
    public int getAttributeUnsignedIntValue(int index, int defaultValue) {
        String value = getAttributeValue(index);
        if (value != null) {
            ResourceValue r = getResourceValue(value);

            if (r != null) {
                value = r.getValue();
            }

            return XmlUtils.convertValueToUnsignedInt(value, defaultValue);
        }

        return defaultValue;
    }

    @Override
    public float getAttributeFloatValue(int index, float defaultValue) {
        String s = getAttributeValue(index);
        if (s != null) {
            ResourceValue r = getResourceValue(s);

            if (r != null) {
                s = r.getValue();
            }

            return Float.parseFloat(s);
        }

        return defaultValue;
    }

    // -- private helper methods

    /**
     * Returns a resolved {@link ResourceValue} from a given value.
     */
    private ResourceValue getResourceValue(String value) {
        // now look for this particular value
        RenderResources resources = mContext.getRenderResources();
        return resources.resolveResValue(resources.findResValue(value, mPlatformFile));
    }

    /**
     * Resolves and return a value to its associated integer.
     */
    private int resolveResourceValue(String value, int defaultValue) {
        ResourceValue resource = getResourceValue(value);
        if (resource != null) {
            Integer id;
            if (mPlatformFile || resource.isFramework()) {
                id = Bridge.getResourceId(resource.getResourceType(), resource.getName());
            } else {
                id = mContext.getLayoutlibCallback().getResourceId(
                        resource.getResourceType(), resource.getName());
            }

            if (id != null) {
                return id;
            }
        }

        return defaultValue;
    }
}
