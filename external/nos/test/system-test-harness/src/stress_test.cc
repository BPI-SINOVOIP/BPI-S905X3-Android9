
#include <unistd.h>

#include <memory>
#include <iostream>
#include <vector>

#include "google/protobuf/empty.pb.h"
#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"
#include "nugget/app/protoapi/testing_api.pb.h"
#include "src/util.h"

using google::protobuf::Empty;
using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::Notice;
using nugget::app::protoapi::OneofTestParametersCase;
using nugget::app::protoapi::OneofTestResultsCase;
using std::unique_ptr;
using std::vector;
using test_harness::BYTE_TIME;
using test_harness::TestHarness;

unique_ptr<TestHarness> harness = unique_ptr<TestHarness>(new TestHarness());

void cleanup() {
  std::cout << "Performing Reboot!\n";
  harness->RebootNugget();
  std::cout << "Done!\n";
}

void signal_handler(int signal) {
  if (signal) {}
  exit(0);
}
void signal_ignore(int signal) { if (signal) {} }

int main(void) {
  vector<uint8_t> input_buffer;
  vector<uint8_t> output_buffer;
  input_buffer.reserve(0x4000);
  output_buffer.reserve(0x4000);

  std::atexit(cleanup);
  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGHUP, signal_ignore);

  std::cout << "SendOneofProto()\n";
  int result = harness->SendOneofProto(
      APImessageID::TESTING_API_CALL,
      OneofTestParametersCase::kFullStressTest,
      Empty());
  if (result != test_harness::error_codes::NO_ERROR) {
    std::cerr << "SendOneofProto() ERROR: "
              << test_harness::error_codes_name(result) << "\n";
    return 1;
  }

  std::cout << "GetData()\n";
  test_harness::raw_message msg;
  if(harness->GetData(&msg, 4096 * BYTE_TIME) !=
      test_harness::error_codes::NO_ERROR) {
    std::cerr << "GetData() ERROR: "
              << test_harness::error_codes_name(result) << "\n";
    return 1;
  }

  if (msg.type != APImessageID::TESTING_API_RESPONSE) {
    std::cerr << "Unexpected received message type: "
              << nugget::app::protoapi::APImessageID_Name(
                  (APImessageID)msg.type) << " (" << msg.type << ")\n";
    if (msg.type == APImessageID::NOTICE) {
      Notice notice;
      if (notice.ParseFromArray(
          reinterpret_cast<char *>(msg.data), msg.data_len)) {
        std::cerr << notice.DebugString();
      }
    }
    return 1;
  }

  if (msg.data_len != 2) {
    std::cerr << "Unexpected received message length: "
              << msg.data_len << "\n";
    return 1;
  }

  uint16_t subtype = (msg.data[0] << 8) | msg.data[1];
  if (subtype != OneofTestResultsCase::kFullStressResult) {
    std::cerr << "Unexpected received message sub type: "
              << nugget::app::protoapi::OneofTestResultsCase_Name(
                  (OneofTestResultsCase)subtype) << " (" << subtype << ")\n";
    return 1;
  }

  std::cout << "Waiting!\n";
  while (true) {
    sleep(15);
  }
  return 0;
}
