/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.device;

/**
 * Represents the allocation state of the device from the IDeviceManager perspective
 */
public enum DeviceAllocationState implements DeviceAllocationEventHandler {
    /** the initial state of a device - should not reside here for long */
    Unknown(new UnknownHandler()),
    /** Device does not match global device filter, and will be ignored by this TF */
    Ignored(new IgnoredHandler()),
    /** device is available to be allocated to a test */
    Available(new AvailableHandler()),
    /** device is visible via adb but is in an error state that prevents it from running tests */
    Unavailable(new UnavailableHandler()),
    /** device is currently allocated to a test */
    Allocated(new AllocatedHandler()),
    /** device is currently being checked for responsiveness */
    Checking_Availability(new CheckingAvailHandler());

    private final DeviceAllocationEventHandler mEventHandler;

    DeviceAllocationState(DeviceAllocationEventHandler eventHandler) {
        mEventHandler = eventHandler;
    }

    @Override
    public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
        return mEventHandler.handleDeviceEvent(event);
    }
}
