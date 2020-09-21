#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <google/protobuf/message.h>

#ifdef ANDROID
#define CONFIG_NO_UART 1
#endif  // ANDROID

#ifndef CONFIG_NO_UART
#include <termios.h>

#include "src/lib/inc/frame_layer.h"
#include "src/lib/inc/frame_layer_types.h"
#endif  // CONFIG_NO_UART
#include "nugget_tools.h"

using std::string;
using std::vector;
using std::unique_ptr;

namespace test_harness {

/** The approximate time it takes to transmit one bit over UART at 115200
 * baud. */
const auto BIT_TIME = std::chrono::microseconds(10000 / 1152);
/** The approximate time it takes to transmit one byte over UART at 115200
 * baud. */
const auto BYTE_TIME = std::chrono::microseconds(80000 / 1152);

const size_t PROTO_BUFFER_MAX_LEN = 512;

enum error_codes : int {
  NO_ERROR = 0,
  GENERIC_ERROR = 1,
  TIMEOUT = 2,
  TRANSPORT_ERROR = 3,
  OVERFLOW_ERROR = 4,
  SERIALIZE_ERROR = 5,
};

const char* error_codes_name(int code);

struct raw_message {
  uint16_t type;  // The "magic number" used to identify the contents of data[].
  uint16_t data_len;  // How much data is in the buffer data[].
  uint8_t data[PROTO_BUFFER_MAX_LEN - 2];  // The payload of the message.
};

class TestHarness {
 public:
  enum VerbosityLevels : int {
    SILENT = 0,
    CRITICAL = 10,
    ERROR = 20,
    WARNING = 30,
    INFO = 40,
  };

  static std::unique_ptr<TestHarness> MakeUnique();

  TestHarness();
  /**
   * @param path The device path to the tty (e.g. "/dev/tty1"). */
  TestHarness(const char* path);

  ~TestHarness();

  /**
   * @return true if data can be sent and received over the tty. */
  bool ttyState() const;

  int getVerbosity() const;
  int setVerbosity(int v);

  /** Reads from tty until it would block. */
  void flushConsole();
  /** Reads from tty until the specified duration has passed. */
  string ReadUntil(std::chrono::microseconds end);
  void PrintUntilClosed();

  bool RebootNugget();

  int SendData(const raw_message& msg);
  int SendOneofProto(uint16_t type, uint16_t subtype,
                     const google::protobuf::Message& message);
  int SendProto(uint16_t type, const google::protobuf::Message& message);

  int GetData(raw_message* msg, std::chrono::microseconds timeout);

  bool UsingSpi() const;

#ifndef CONFIG_NO_UART
  bool SwitchFromConsoleToProtoApi();

  bool SwitchFromProtoApiToConsole(raw_message* out_msg);
#endif  // CONFIG_NO_UART

 protected:
  int verbosity;
  vector<uint8_t> output_buffer;
  vector<uint8_t> input_buffer;

  void Init(const char* path);

  /** Writes @len bytes from @data until complete. */
  void BlockingWrite(const char* data, size_t len);

  /** Reads raw bytes from the tty until either an end of line is reached or
   * the input would block.
   *
   * @return a single line with the '\n' character unless the last read() would
   * have blocked.*/
  string ReadLineUntilBlock();

  // Needed for AHDLC / UART.
#ifndef CONFIG_NO_UART
  struct termios tty_state;
  ahdlc_frame_encoder_t encoder;
  ahdlc_frame_decoder_t decoder;
  int SendAhdlc(const raw_message& msg);
  int GetAhdlc(raw_message* msg, std::chrono::microseconds timeout);
#endif  // CONFIG_NO_UART
  int tty_fd;

  // Needed for libnos / SPI.
  unique_ptr<nos::NuggetClientInterface> client;
  int SendSpi(const raw_message& msg);
  int GetSpi(raw_message* msg, std::chrono::microseconds timeout);

  std::unique_ptr<std::thread> print_uart_worker;
};

void FatalError(const string& msg);

}  // namespace test_harness

#endif  // SRC_UTIL_H
