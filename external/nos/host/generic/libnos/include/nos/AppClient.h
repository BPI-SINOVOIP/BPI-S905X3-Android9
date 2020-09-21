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

#ifndef NOS_APP_CLIENT_H
#define NOS_APP_CLIENT_H

#include <cstdint>
#include <vector>

#include <nos/NuggetClientInterface.h>

namespace nos {

/**
 * Client to communicate with an app running on Nugget.
 */
class AppClient {
public:
    /**
     * Create a client for an app with the given ID that communicates with
     * Nugget through the given NuggetClient.
     *
     * @param client Client for Nugget.
     * @param appId  ID of the target app.
     */
    AppClient(NuggetClientInterface& client, uint32_t appId)
            : _client(client), _appId(appId) {}

    /**
     * Call the app.
     *
     * @param arg      Argument to pass to the app.
     * @param request  Data to send to the app.
     * @param response Buffer to receive data from the app.
     */
    uint32_t Call(uint16_t arg, const std::vector<uint8_t>& request,
                  std::vector<uint8_t>* response) {
        return _client.CallApp(_appId, arg, request, response);
    }


private:
    NuggetClientInterface& _client;
    uint32_t _appId;
};

} // namespace nos

#endif // NOS_APP_CLIENT_H
