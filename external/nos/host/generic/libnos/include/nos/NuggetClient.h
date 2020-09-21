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

#ifndef NOS_NUGGET_CLIENT_H
#define NOS_NUGGET_CLIENT_H

#include <cstdint>
#include <string>
#include <vector>

#include <nos/device.h>
#include <nos/NuggetClientInterface.h>

namespace nos {

/**
 * Client to communicate with a Nugget device via the transport API.
 */
class NuggetClient : public NuggetClientInterface {
public:
    /**
     * Create a client for the default Nugget device.
     */
    NuggetClient();

    /**
     * Create a client for the named Nugget device.
     *
     * Passing an empty device name causes the default device to be selected.
     */
    NuggetClient(const std::string& device_name);

    ~NuggetClient() override;

    /**
     * Opens a connection to the default Nugget device.
     *
     * If this fails, isOpen() will return false.
     */
    void Open() override;

    /**
     * Closes the connection to Nugget.
     */
    void Close() override;

    /**
     * Checked whether a connection is open to Nugget.
     */
    bool IsOpen() const override;

    /**
     * Call into and app running on Nugget.
     *
     * @param app_id   The ID of the app to call.
     * @param arg      Argument to pass to the app.
     * @param request  Data to send to the app.
     * @param response Buffer to receive data from the app.
     * @return         Status code from the app.
     */
    uint32_t CallApp(uint32_t appId, uint16_t arg,
                     const std::vector<uint8_t>& request,
                     std::vector<uint8_t>* response) override;

    /**
     * Access the underlying device.
     *
     * NULL is returned if the connection to the device is not open.
     *
     * This can be used by subclasses or to use to use the C API directly.
     */
    nos_device* Device();
    const nos_device* Device() const;

    /**
     * Access the name of the device this is a client for.
     */
    const std::string& DeviceName() const;

private:
    std::string device_name_;
    nos_device device_;
    bool open_;
};

} // namespace nos

#endif // NOS_NUGGET_CLIENT_H
