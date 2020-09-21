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

package com.android.server.cts;

import android.providers.settings.SettingProto;
import android.providers.settings.SettingsOperationProto;
import android.providers.settings.SettingsServiceDumpProto;
import android.providers.settings.UserSettingsProto;

import com.android.ddmlib.Log.LogLevel;
import com.android.incident.Destination;
import com.android.incident.Privacy;
import com.android.tradefed.log.LogUtil.CLog;
import com.google.protobuf.GeneratedMessage;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Test to check that the settings service properly outputs its dump state.
 */
public class SettingsIncidentTest extends ProtoDumpTestCase {

    /**
     * Test that there are some secure/system settings for the user and there are some global
     * settings.
     *
     * @throws Exception
     */
    public void testBasicStructure() throws Exception {
        SettingsServiceDumpProto dump = getDump(SettingsServiceDumpProto.parser(),
                "dumpsys settings --proto");

        verifySettingsServiceDumpProto(dump, PRIVACY_NONE);
    }

    static void verifySettingsServiceDumpProto(SettingsServiceDumpProto dump, final int filterLevel) throws Exception {
        if (dump.getUserSettingsCount() > 0) {
            UserSettingsProto userSettings = dump.getUserSettings(0);
            assertEquals(0, userSettings.getUserId());

            CLog.logAndDisplay(LogLevel.INFO, "#*#*#*#*#*#*#*#*#*#*# SECURE #*#*#*#*#*#*#*#*#*#*#");
            verifySettings(userSettings.getSecureSettings(), filterLevel);
            CLog.logAndDisplay(LogLevel.INFO, "#*#*#*#*#*#*#*#*#*#*# SYSTEM #*#*#*#*#*#*#*#*#*#*#");
            verifySettings(userSettings.getSystemSettings(), filterLevel);
        }

        CLog.logAndDisplay(LogLevel.INFO, "#*#*#*#*#*#*#*#*#*#*# GLOBAL #*#*#*#*#*#*#*#*#*#*#");
        verifySettings(dump.getGlobalSettings(), filterLevel);
    }

    private static void verifySettings(GeneratedMessage settings, final int filterLevel) throws Exception {
        verifySettings(getSettingProtos(settings), filterLevel);

        final List<SettingsOperationProto> ops = invoke(settings, "getHistoricalOperationsList");
        for (SettingsOperationProto op : ops) {
            assertTrue(op.getTimestamp() >= 0);
            assertNotNull(op.getOperation());
            // setting is optional
            if (filterLevel == PRIVACY_AUTO) {
                // SettingOperationProto is EXPLICIT by default.
                assertTrue(op.getOperation().isEmpty());
                assertTrue(op.getSetting().isEmpty());
            }
        }
    }

    private static Map<SettingProto, Destination> getSettingProtos(GeneratedMessage settingsProto) {
        CLog.d("Checking out class: " + settingsProto.getClass());

        Map<SettingProto, Destination> settings = new HashMap<>();
        for (FieldDescriptor fd : settingsProto.getDescriptorForType().getFields()) {
            if (fd.getType() != FieldDescriptor.Type.MESSAGE) {
                // Only looking for SettingProtos and messages that contain them. Skip any primitive
                // fields.
                continue;
            }
            List<Object> tmp;
            if (fd.isRepeated()) {
                tmp = (List) settingsProto.getField(fd);
            } else {
                tmp = new ArrayList<>();
                tmp.add(settingsProto.getField(fd));
            }
            Destination dest = fd.getOptions().getExtension(Privacy.privacy).getDest();
            for (Object o : tmp) {
                if ("android.providers.settings.SettingProto".equals(fd.getMessageType().getFullName())) {
                    // The container's default privacy doesn't affect message types. However,
                    // anotations on the field override the message's default annotation. If a
                    // message field doesn't have an annotation, it is treated as EXPLICIT by
                    // default.
                    settings.put((SettingProto) o, dest == Destination.DEST_UNSET ? Destination.DEST_EXPLICIT : dest);
                } else {
                    // Sub messages don't inherit the container's default privacy. If the field had
                    // an annotation, it would override the sub message's default privacy.
                    settings.putAll(getSettingProtos((GeneratedMessage) o));
                }
            }
        }

        return settings;
    }

    private static <T> T invoke(Method method, Object instance, Object... args) {
        method.setAccessible(true);
        try {
            return (T) method.invoke(instance, args);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static <T> T invoke(GeneratedMessage instance, String methodName, Object... args)
            throws Exception {
        final Class<?>[] inputParamTypes = Arrays.stream(args)
                .map((arg) -> toPrimitive(arg.getClass()))
                .toArray(Class[]::new);
        return invoke(
                instance.getClass().getDeclaredMethod(methodName, inputParamTypes),
                instance, args);
    }

    private static Class<?> toPrimitive(Class<?> c) {
        return c == Integer.class ? int.class : c;
    }

    private static void verifySettings(Map<SettingProto, Destination> settings, final int filterLevel) throws Exception {
        assertFalse(settings.isEmpty());

        CLog.d("Field count: " + settings.size());
        for (Map.Entry<SettingProto, Destination> sDPair : settings.entrySet()) {
            SettingProto setting = sDPair.getKey();
            Destination dest = sDPair.getValue();
            try {
                final String id = setting.getId();
                if (!id.isEmpty()) {
                    // _ID has to be a long converted to a String
                    Long.parseLong(id);
                }
                assertNotNull(setting.getName());
                if (filterLevel < PRIVACY_LOCAL) {
                    if  (dest == Destination.DEST_LOCAL) {
                        // Any filter that is not LOCAL should make sure local isn't printed at all.
                        String err = "Setting '" + setting.getName() + "' with LOCAL privacy didn't strip data for filter level '" + privacyToString(filterLevel) + "'";
                        assertTrue(err, setting.getId().isEmpty());
                        assertTrue(err, setting.getName().isEmpty());
                        assertTrue(err, setting.getPkg().isEmpty());
                        assertTrue(err, setting.getValue().isEmpty());
                        assertTrue(err, setting.getDefaultValue().isEmpty());
                    }
                    if (filterLevel < PRIVACY_EXPLICIT) {
                        if (dest == Destination.DEST_EXPLICIT) {
                            String err = "Setting '" + setting.getName() + "' with EXPLICIT privacy didn't strip data for filter level '" + privacyToString(filterLevel) + "'";
                            assertTrue(err, setting.getId().isEmpty());
                            assertTrue(err, setting.getName().isEmpty());
                            assertTrue(err, setting.getPkg().isEmpty());
                            assertTrue(err, setting.getValue().isEmpty());
                            assertTrue(err, setting.getDefaultValue().isEmpty());
                        }
                    }
                }
                // pkg is optional
                // value can be anything
                // default can be anything
                // default from system reported only if optional default present
            } catch (Throwable e) {
                throw new AssertionError("Failed for setting " + setting, e);
            }
        }
    }
}

