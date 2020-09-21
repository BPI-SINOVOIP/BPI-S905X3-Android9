//
// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

namespace tpm_manager {

// Handles protobuf conversions for a binder proxy. Implements ITpmManagerClient
// and acts as a response observer for async requests it sends out. The
// |RequestProtobufType| and |ResponseProtobufType| types can be any protobufs.
template<typename RequestProtobufType, typename ResponseProtobufType>
class BinderProxyHelper {
 public:
  using CallbackType = base::Callback<void(const ResponseProtobufType&)>;
  using GetErrorResponseCallbackType =
      base::Callback<void(ResponseProtobufType*)>;
  using BinderMethodType = base::Callback<android::binder::Status(
      const std::vector<uint8_t>& command_proto,
      const android::sp<android::tpm_manager::ITpmManagerClient>& client)>;

  BinderProxyHelper(const BinderMethodType& method,
                    const CallbackType& callback,
                    const GetErrorResponseCallbackType& get_error_response)
      : method_(method),
        callback_(callback),
        get_error_response_(get_error_response) {}

  void SendErrorResponse() {
    ResponseProtobufType response_proto;
    get_error_response_.Run(&response_proto);
    callback_.Run(response_proto);
  }

  void SendRequest(const RequestProtobufType& request) {
    VLOG(2) << __func__;
    android::sp<ResponseObserver> observer(
        new ResponseObserver(callback_, get_error_response_));
    std::vector<uint8_t> request_bytes;
    request_bytes.resize(request.ByteSize());
    if (!request.SerializeToArray(request_bytes.data(), request_bytes.size())) {
      LOG(ERROR) << "BinderProxyHelper: Failed to serialize protobuf.";
      SendErrorResponse();
      return;
    }
    android::binder::Status status = method_.Run(request_bytes, observer);
    if (!status.isOk()) {
      LOG(ERROR) << "BinderProxyHelper: Binder error: " << status.toString8();
      SendErrorResponse();
      return;
    }
  }

 private:
  class ResponseObserver : public android::tpm_manager::BnTpmManagerClient {
   public:
    ResponseObserver(const CallbackType& callback,
                     const GetErrorResponseCallbackType& get_error_response)
        : callback_(callback), get_error_response_(get_error_response) {}

    // ITpmManagerClient interface.
    android::binder::Status OnCommandResponse(
        const std::vector<uint8_t>& response_proto_data) override {
      VLOG(2) << __func__;
      ResponseProtobufType response_proto;
      if (!response_proto.ParseFromArray(response_proto_data.data(),
                                         response_proto_data.size())) {
        LOG(ERROR) << "BinderProxyHelper: Bad response data.";
        get_error_response_.Run(&response_proto);
      }
      callback_.Run(response_proto);
      return android::binder::Status::ok();
    }

   private:
    CallbackType callback_;
    GetErrorResponseCallbackType get_error_response_;
  };

  BinderMethodType method_;
  CallbackType callback_;
  GetErrorResponseCallbackType get_error_response_;
};

}  // tpm_manager
