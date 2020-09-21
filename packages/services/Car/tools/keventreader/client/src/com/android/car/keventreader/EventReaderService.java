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
package com.android.car.keventreader;

import android.annotation.Nullable;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import java.util.Objects;

public final class EventReaderService {
  private static final String TAG = "car.keventreader";
  private static final String SERVICE_NAME = "com.android.car.keventreader";
  private final IEventProvider mService;

  private EventReaderService(IEventProvider service) {
      mService = Objects.requireNonNull(service);
  }

  @Nullable
  private static IEventProvider getService() {
      return IEventProvider.Stub.asInterface(ServiceManager.getService(SERVICE_NAME));
  }

  @Nullable
  public static EventReaderService tryGet() {
      IEventProvider provider = getService();
      if (provider == null) return null;
      return new EventReaderService(provider);
  }

  public boolean registerCallback(IEventCallback callback) {
      try {
          mService.registerCallback(callback);
          return true;
      } catch (RemoteException e) {
          Log.e(TAG, "unable to register new callback", e);
          return false;
      }
  }

  public boolean unregisterCallback(IEventCallback callback) {
      try {
          mService.unregisterCallback(callback);
          return true;
      } catch (RemoteException e) {
          Log.e(TAG, "unable to remove callback registration", e);
          return false;
      }
  }
}
