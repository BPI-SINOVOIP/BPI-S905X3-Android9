/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package android.server.am;

import android.app.WindowConfiguration;
import android.app.nano.WindowConfigurationProto;
import android.content.nano.ConfigurationProto;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.graphics.nano.RectProto;

/**
 * Utility class for extracting some common framework object from nano proto objects.
 * Normally the extractors will be in the framework object class, but we don't want the framework to
 * depend on nano proto due to size cost.
 * TODO: This class should probably be in frameworks/base/lib project so it can be used
 * outside of CTS.
 */
public class ProtoExtractors {
    public static Configuration extract(ConfigurationProto proto) {
        final Configuration config = new Configuration();
        if (proto == null) {
            return config;
        }
        config.windowConfiguration.setTo(extract(proto.windowConfiguration));
        config.densityDpi = proto.densityDpi;
        config.orientation = proto.orientation;
        config.screenHeightDp = proto.screenHeightDp;
        config.screenWidthDp = proto.screenWidthDp;
        config.smallestScreenWidthDp = proto.smallestScreenWidthDp;
        config.screenLayout = proto.screenLayout;
        config.uiMode = proto.uiMode;
        return config;
    }

    public static WindowConfiguration extract(WindowConfigurationProto proto) {
        final WindowConfiguration config = new WindowConfiguration();
        if (proto == null) {
            return config;
        }
        config.setAppBounds(extract(proto.appBounds));
        config.setWindowingMode(proto.windowingMode);
        config.setActivityType(proto.activityType);
        return config;
    }

    public static Rect extract(RectProto proto) {
        if (proto == null) {
            return null;
        }
        return new Rect(proto.left, proto.top, proto.right, proto.bottom);
    }
}
