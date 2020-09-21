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

#include <general_test/test.h>

#include <shared/abort.h>
#include <shared/send_message.h>

#include <chre.h>

using nanoapp_testing::sendFatalFailureToHost;

namespace general_test {

Test::Test(uint32_t minSupportedVersion)
    : mApiVersion(chreGetApiVersion())
      , mIsSupported(mApiVersion >= minSupportedVersion) {
}

void Test::testSetUp(uint32_t messageSize, const void *message) {
  if (mIsSupported) {
    setUp(messageSize, message);
  } else {
    sendMessageToHost(nanoapp_testing::MessageType::kSkipped);
  }
}

void Test::testHandleEvent(uint32_t senderInstanceId, uint16_t eventType,
                           const void *eventData) {
  if (mIsSupported) {
    handleEvent(senderInstanceId, eventType, eventData);
  }
}

void Test::unexpectedEvent(uint16_t eventType) {
  uint32_t localEvent = eventType;
  sendFatalFailureToHost("Test received unexpected event:", &localEvent);
}

const void *Test::getMessageDataFromHostEvent(uint32_t senderInstanceId,
                                              uint16_t eventType, const void* eventData,
                                              nanoapp_testing::MessageType expectedMessageType,
                                              uint32_t expectedMessageSize) {
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    sendFatalFailureToHost("Unexpected sender ID:", &senderInstanceId);
  }
  if (eventType != CHRE_EVENT_MESSAGE_FROM_HOST) {
    unexpectedEvent(eventType);
  }
  if (eventData == nullptr) {
    sendFatalFailureToHost("NULL eventData given");
  }
  auto data = static_cast<const chreMessageFromHostData*>(eventData);
  if (data->reservedMessageType != uint32_t(expectedMessageType)) {
    sendFatalFailureToHost("Unexpected reservedMessageType:",
                           &(data->reservedMessageType));
  }
  if (data->messageSize != expectedMessageSize) {
    sendFatalFailureToHost("Unexpected messageSize:", &(data->messageSize));
  }
  return data->message;
}


}  // namespace general_test
