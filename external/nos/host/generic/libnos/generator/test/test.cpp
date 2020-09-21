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

#include <Hello.client.h>
#include <MockHello.client.h>

#include <google/protobuf/util/message_differencer.h>

#include <nos/MockNuggetClient.h>

#include <gtest/gtest.h>

using ::google::protobuf::util::MessageDifferencer;

using ::nos::MockNuggetClient;
using ::nos::generator::test::EmptyRequest;
using ::nos::generator::test::EmptyResponse;
using ::nos::generator::test::GreetRequest;
using ::nos::generator::test::GreetResponse;
using ::nos::generator::test::Hello;
using ::nos::generator::test::IHello;
using ::nos::generator::test::MockHello;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Return;
using ::testing::SetArgPointee;

MATCHER_P(ProtoMessageEq, msg, "") { return MessageDifferencer::Equals(arg, msg); }

// Check the message is the same rather than the encoded bytes as different
// bytes could decode to the same message.
MATCHER_P(DecodesToProtoMessage, msg, "Vector does not decode to correct message") {
    decltype(msg) decoded;
    return decoded.ParseFromArray(arg.data(), arg.size())
           && MessageDifferencer::Equals(decoded, msg);
}

// The method's ID is based on the order of declaration in the service.
TEST(GeneratedServiceClientTest, MethodsHaveCorrectIds) {
    MockNuggetClient client;
    Hello service{client};

    EXPECT_CALL(client, CallApp(APP_ID_TEST, 2, _, _)).WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_TEST, 0, _, _)).WillOnce(Return(APP_SUCCESS));
    EXPECT_CALL(client, CallApp(APP_ID_TEST, 1, _, _)).WillOnce(Return(APP_SUCCESS));

    EmptyRequest request;
    EmptyResponse response;

    service.Third(request, &response);
    service.First(request, &response);
    service.Second(request, &response);
}

// Both request and response messages are exchanged successfully.
TEST(GeneratedServiceClientTest, DataSuccessfullyExchanged) {
    MockNuggetClient client;
    Hello service{client};

    GreetRequest request;
    request.set_who("Tester");
    request.set_age(78);

    GreetResponse response;
    response.set_greeting("Hello, Tester age 78");

    std::vector<uint8_t> responseBytes(response.ByteSize());
    ASSERT_TRUE(response.SerializeToArray(responseBytes.data(), responseBytes.size()));

    EXPECT_CALL(client, CallApp(_, _, DecodesToProtoMessage(request), _))
            .WillOnce(DoAll(SetArgPointee<3>(responseBytes), Return(APP_SUCCESS)));

    GreetResponse real_response;
    EXPECT_THAT(service.Greet(request, &real_response), Eq(APP_SUCCESS));
    EXPECT_THAT(real_response, ProtoMessageEq(response));
}

// The response can be ignored but the request is still passed on.
TEST(GeneratedServiceClientTest, NullptrResponseBufferIgnoresResponseData) {
    MockNuggetClient client;
    Hello service{client};

    GreetRequest request;
    request.set_who("Silence");
    request.set_age(10);

    EXPECT_CALL(client, CallApp(_, _, DecodesToProtoMessage(request), Eq(nullptr)))
            .WillOnce(Return(APP_SUCCESS));

    EXPECT_THAT(service.Greet(request, nullptr), Eq(APP_SUCCESS));
}

// Errors from the chip should be forwarded to the caller without decoding the
// response
TEST(GeneratedServiceClientTest, AppErrorsPropagatedWithoutResponseDecode) {
    MockNuggetClient client;
    Hello service{client};

    GreetResponse response;
    response.set_greeting("Ignore me");

    std::vector<uint8_t> responseBytes(response.ByteSize());
    ASSERT_TRUE(response.SerializeToArray(responseBytes.data(), responseBytes.size()));

    EXPECT_CALL(client, CallApp(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<3>(responseBytes), Return(APP_ERROR_BOGUS_ARGS)));

    GreetRequest request;
    GreetResponse real_response;
    EXPECT_THAT(service.Greet(request, &real_response), Eq(APP_ERROR_BOGUS_ARGS));
    EXPECT_THAT(real_response, ProtoMessageEq(GreetResponse{}));
}

// Invalid encoding of a response message results in an RPC error
TEST(GeneratedServiceClientTest, GarbledResponseIsRpcFailure) {
    MockNuggetClient client;
    Hello service{client};

    std::vector<uint8_t> garbledResponse{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_CALL(client, CallApp(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<3>(garbledResponse), Return(APP_SUCCESS)));

    GreetRequest request;
    GreetResponse response;
    EXPECT_THAT(service.Greet(request, &response), Eq(APP_ERROR_RPC));
}

// Sending too much data will fail before beginning a transaction with the chip.
TEST(GeneratedServiceClientTest, RequestLargerThanBuffer) {
    MockNuggetClient client;
    Hello service{client};

    EXPECT_CALL(client, CallApp(_, _, _, _)).Times(0);

    GreetRequest request;
    request.set_who("This is far too long for the buffer so should fail");

    GreetResponse response;
    EXPECT_THAT(service.Greet(request, &response), Eq(APP_ERROR_TOO_MUCH));
}

// Example using generate service mocks.
TEST(GeneratedServiceClientTest, CanUseGeneratedMocks) {
    MockHello mockService;

    GreetResponse response;
    constexpr auto mockGreeting = "I made this up";
    response.set_greeting(mockGreeting);

    EXPECT_CALL(mockService, Greet(_, _))
            .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    // Use as an instance of the service to mimic a real test
    IHello& service = mockService;
    GreetRequest request;
    GreetResponse real_response;
    EXPECT_THAT(service.Greet(request, &real_response), Eq(APP_SUCCESS));
    EXPECT_THAT(real_response, ProtoMessageEq(response));
}
