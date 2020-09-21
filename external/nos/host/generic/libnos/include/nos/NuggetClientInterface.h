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
 * limitations under the License.
 */

#ifndef NOS_NUGGET_CLIENT_INTERFACE_H
#define NOS_NUGGET_CLIENT_INTERFACE_H

#include <cstdint>
#include <vector>

namespace nos {

/**
 * Interface for communication with a Nugget device.
 */
class NuggetClientInterface {
public:
    virtual ~NuggetClientInterface() = default;

    /**
     * Opens a connection to the default Nugget device.
     *
     * If this fails, isOpen() will return false.
     */
    virtual void Open() = 0;

    /**
     * Closes the connection to Nugget.
     */
    virtual void Close() = 0;

    /**
     * Checked whether a connection is open to Nugget.
     */
    virtual bool IsOpen() const = 0;

    /**
     * Call into and app running on Nugget.
     *
     * @param app_id   The ID of the app to call.
     * @param arg      Argument to pass to the app.
     * @param request  Data to send to the app.
     * @param response Buffer to receive data from the app.
     * @return         Status code from the app.
     */
    virtual uint32_t CallApp(uint32_t appId, uint16_t arg,
                             const std::vector<uint8_t>& request,
                             std::vector<uint8_t>* response) = 0;
};

} // namespace nos

#endif // NOS_NUGGET_CLIENT_INTERFACE_H
