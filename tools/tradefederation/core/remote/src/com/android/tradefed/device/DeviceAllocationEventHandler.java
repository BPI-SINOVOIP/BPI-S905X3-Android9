/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * Implements the test device allocation state machine.
 * <p/>
 * Uses the state machine design pattern
 */
interface DeviceAllocationEventHandler {
    DeviceAllocationState handleDeviceEvent(DeviceEvent event);

    /**
     * Handles events in {@link DeviceAllocationState#Unknown} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Unknown -> FORCE_ALLOCATE_REQUEST -> Allocated</li>
     * <li>Unknown -> CONNECTED_ONLINE -> Checking_Availability</li>
     * <li>Unknown -> CONNECTED_OFFLINE -> Unavailable</li>
     * <li>Unknown -> STATE_CHANGE_ONLINE -> Checking_Availability</li>
     * <li>Unknown -> STATE_CHANGE_OFFLINE -> Unavailable</li>
     * <li>Unknown -> FORCE_AVAILABLE -> Available</li>
     * </ul>
     */
    class UnknownHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case FORCE_ALLOCATE_REQUEST:
                    return DeviceAllocationState.Allocated;
                case CONNECTED_ONLINE:
                    return DeviceAllocationState.Checking_Availability;
                case CONNECTED_OFFLINE:
                    return DeviceAllocationState.Unavailable;
                case STATE_CHANGE_ONLINE:
                    return DeviceAllocationState.Checking_Availability;
                case STATE_CHANGE_OFFLINE:
                    return DeviceAllocationState.Unavailable;
                case FORCE_AVAILABLE:
                    return DeviceAllocationState.Available;
                default:
                    return DeviceAllocationState.Unknown;
            }
        }
    }

    /**
     * Handles events in {@link DeviceAllocationState#Checking_Availability} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Checking_Availability -> FORCE_ALLOCATE_REQUEST -> Allocated</li>
     * <li>Checking_Availability -> AVAILABLE_CHECK_PASSED -> Available</li>
     * <li>Checking_Availability -> AVAILABLE_CHECK_FAILED -> Unavailable</li>
     * <li>Checking_Availability -> AVAILABLE_CHECK_IGNORED -> Ignored</li>
     * <li>Checking_Availability -> FORCE_AVAILABLE -> Available</li>
     * <li>Checking_Availability -> STATE_CHANGE_OFFLINE -> Unavailable</li>
     * <li>Checking_Availability -> DISCONNECTED -> Unavailable</li>
     * </ul>
     */
    class CheckingAvailHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case FORCE_ALLOCATE_REQUEST:
                    return DeviceAllocationState.Allocated;
                case AVAILABLE_CHECK_PASSED:
                    return DeviceAllocationState.Available;
                case AVAILABLE_CHECK_FAILED:
                    return DeviceAllocationState.Unavailable;
                case AVAILABLE_CHECK_IGNORED:
                    return DeviceAllocationState.Ignored;
                case FORCE_AVAILABLE:
                    return DeviceAllocationState.Available;
                case STATE_CHANGE_OFFLINE:
                    return DeviceAllocationState.Unavailable;
                case DISCONNECTED:
                    return DeviceAllocationState.Unavailable;
                default:
                    return DeviceAllocationState.Checking_Availability;
            }
        }
    }

    /**
     * Handles events in {@link DeviceAllocationState#Available} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Available -> ALLOCATE_REQUEST -> Allocated</li>
     * <li>Available -> FORCE_ALLOCATE_REQUEST -> Allocated</li>
     * <li>Available -> STATE_CHANGE_OFFLINE -> Unavailable</li>
     * <li>Available -> DISCONNECTED -> Unknown</li>
     * </ul>
     */
    class AvailableHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case ALLOCATE_REQUEST:
                case FORCE_ALLOCATE_REQUEST:
                    return DeviceAllocationState.Allocated;
                case STATE_CHANGE_OFFLINE:
                    return DeviceAllocationState.Unavailable;
                case DISCONNECTED:
                    return DeviceAllocationState.Unknown;
                default:
                    return DeviceAllocationState.Available;

            }
        }
    }

    /**
     * Handles events in {@link DeviceAllocationState#Allocated} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Allocated -> FREE_UNAVAILABLE -> Unavailable</li>
     * <li>Allocated -> FREE_AVAILABLE -> Available</li>
     * <li>Allocated -> FREE_UNRESPONSIVE -> Available</li>
     * <li>Allocated -> FREE_UNKNOWN -> Unknown</li>
     * </ul>
     */
    class AllocatedHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case FREE_UNAVAILABLE:
                    return DeviceAllocationState.Unavailable;
                case FREE_AVAILABLE:
                    return DeviceAllocationState.Available;
                case FREE_UNRESPONSIVE:
                    return DeviceAllocationState.Available;
                case FREE_UNKNOWN:
                    return DeviceAllocationState.Unknown;
                default:
                    return DeviceAllocationState.Allocated;
            }
        }
    }

    /**
     * Handles events in {@link DeviceAllocationState#Unavailable} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Unavailable -> FORCE_ALLOCATE_REQUEST -> Allocated</li>
     * <li>Unavailable -> DISCONNECTED -> Unknown</li>
     * <li>Unavailable -> FORCE_AVAILABLE -> Available</li>
     * </ul>
     */
    class UnavailableHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case FORCE_ALLOCATE_REQUEST:
                    return DeviceAllocationState.Allocated;
                case DISCONNECTED:
                    return DeviceAllocationState.Unknown;
                case FORCE_AVAILABLE:
                    return DeviceAllocationState.Available;
                default:
                    return DeviceAllocationState.Unavailable;
            }
        }
    }

    /**
     * Handles events in {@link DeviceAllocationState#Ignored} state.
     * <p/>
     * Transitions:
     * <ul>
     * <li>Ignored -> DISCONNECTED -> Unknown</li>
     * </ul>
     */
    class IgnoredHandler implements DeviceAllocationEventHandler {
        @Override
        public DeviceAllocationState handleDeviceEvent(DeviceEvent event) {
            switch (event) {
                case DISCONNECTED:
                    return DeviceAllocationState.Unknown;
                default:
                    return DeviceAllocationState.Ignored;
            }
        }
    }
}
