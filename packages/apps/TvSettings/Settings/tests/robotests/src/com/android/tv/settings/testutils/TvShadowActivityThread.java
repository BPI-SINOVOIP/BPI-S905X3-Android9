/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tv.settings.testutils;

import android.app.ActivityThread;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.os.RemoteException;

import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import javax.annotation.Nonnull;

/**
 * A copy of roboletric ShadowActivityThread, added getActivityInfo, getServiceInfo
 * TODO: Remove this class when b/74008159 is fixed.
 */
@Implements(value = ActivityThread.class)
public class TvShadowActivityThread {

    @Implementation
    public static Object getPackageManager() {
        ClassLoader classLoader = TvShadowActivityThread.class.getClassLoader();
        Class<?> iPackageManagerClass;
        try {
            iPackageManagerClass = classLoader.loadClass("android.content.pm.IPackageManager");
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
        return Proxy.newProxyInstance(
                classLoader,
                new Class[]{iPackageManagerClass},
                new InvocationHandler() {
                    @Override
                    public Object invoke(Object proxy, @Nonnull Method method, Object[] args)
                            throws Exception {
                        if (method.getName().equals("getApplicationInfo")) {
                            String packageName = (String) args[0];
                            int flags = (Integer) args[1];

                            try {
                                return RuntimeEnvironment.application
                                        .getPackageManager()
                                        .getApplicationInfo(packageName, flags);
                            } catch (PackageManager.NameNotFoundException e) {
                                throw new RemoteException(e.getMessage());
                            }
                        } else if (method.getName().equals("getActivityInfo")) {
                            ComponentName className = (ComponentName) args[0];
                            int flags = (Integer) args[1];

                            try {
                                return RuntimeEnvironment.application
                                        .getPackageManager()
                                        .getActivityInfo(className, flags);
                            } catch (PackageManager.NameNotFoundException e) {
                                throw new RemoteException(e.getMessage());
                            }
                        } else if (method.getName().equals("getServiceInfo")) {
                            ComponentName className = (ComponentName) args[0];
                            int flags = (Integer) args[1];

                            try {
                                return RuntimeEnvironment.application
                                        .getPackageManager()
                                        .getServiceInfo(className, flags);
                            } catch (PackageManager.NameNotFoundException e) {
                                throw new RemoteException(e.getMessage());
                            }
                        } else if (method.getName().equals("notifyPackageUse")) {
                            return null;
                        } else if (method.getName().equals("getPackageInstaller")) {
                            return null;
                        }
                        throw new UnsupportedOperationException("sorry, not supporting " + method
                                + " yet!");
                    }
                });
    }

    @Implementation
    public static Object currentActivityThread() {
        return RuntimeEnvironment.getActivityThread();
    }

}
