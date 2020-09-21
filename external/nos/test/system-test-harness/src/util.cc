#include "src/util.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>

#include <application.h>

#include "nugget_tools.h"
#include "nugget/app/protoapi/control.pb.h"
#include "nugget/app/protoapi/header.pb.h"

#ifndef CONFIG_NO_UART
#include "src/lib/inc/crc_16.h"
#endif  // CONFIG_NO_UART

#ifdef ANDROID
#define FLAGS_util_use_ahdlc false
#define FLAGS_util_print_uart false
#else
#include "gflags/gflags.h"

DEFINE_bool(util_use_ahdlc, false, "Use aHDLC over UART instead of SPI.");
DEFINE_bool(util_print_uart, false, "Print the output of citadel UART.");
DEFINE_string(util_verbosity, "ERROR", "One of SILENT, CRITICAL, ERROR, WARNING, or INFO.");
#endif  // ANDROID

using nugget::app::protoapi::APImessageID;
using nugget::app::protoapi::ControlRequest;
using nugget::app::protoapi::ControlRequestType;
using nugget::app::protoapi::Notice;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

namespace test_harness {
namespace {

int GetVerbosityFromFlag() {
#ifdef ANDROID
  return TestHarness::ERROR;
#else
  std::string upper_case_flag;
  upper_case_flag.reserve(FLAGS_util_verbosity.size());
  std::transform(FLAGS_util_verbosity.begin(), FLAGS_util_verbosity.end(),
                 std::back_inserter(upper_case_flag), toupper);

  if (upper_case_flag == "SILENT")
    return TestHarness::SILENT;
  if (upper_case_flag == "CRITICAL")
    return TestHarness::CRITICAL;
  if (upper_case_flag == "WARNING")
    return TestHarness::WARNING;
  if (upper_case_flag == "INFO")
    return TestHarness::INFO;

  // Default to ERROR.
  return TestHarness::ERROR;
#endif  // ANDROID
}

#ifndef ANDROID
string find_uart(int verbosity) {
  constexpr char dir_path[] = "/dev/";
  auto dir = opendir(dir_path);
  if (!dir) {
    return "";
  }

  string manual_serial_no = nugget_tools::GetCitadelUSBSerialNo();
  const char prefix[] = "ttyUltraTarget_";

  string return_value = "";
  if (manual_serial_no.empty()) {
    const size_t prefix_length = sizeof(prefix) / sizeof(prefix[0]) - 1;
    while (auto listing = readdir(dir)) {
      // The following is always true so it is not checked:
      // sizeof(listing->d_name) >= sizeof(prefix)
      if (std::equal(prefix, prefix + prefix_length, listing->d_name)) {
        return_value = string(dir_path) + listing->d_name;
        break;
      }
    }
  } else {
    return_value = string(dir_path) + prefix + manual_serial_no;
  }

  if (verbosity >= TestHarness::VerbosityLevels::INFO) {
    if (return_value.empty()) {
      std::cout << "UltraDebug UART not found" << std::endl;
    } else {
      std::cout << "USING: " << return_value << std::endl;
    }
  }

  closedir(dir);
  return return_value;
}
#endif  // ANDROID

}  // namespace

std::unique_ptr<TestHarness> TestHarness::MakeUnique() {
  return std::unique_ptr<TestHarness>(new TestHarness());
}

TestHarness::TestHarness() : verbosity(GetVerbosityFromFlag()),
                             output_buffer(PROTO_BUFFER_MAX_LEN, 0),
                             input_buffer(PROTO_BUFFER_MAX_LEN, 0), tty_fd(-1) {
#ifdef CONFIG_NO_UART
  Init(nullptr);
#else
  string path = find_uart(verbosity);
  Init(path.c_str());
#endif  // CONFIG_NO_UART
}

TestHarness::TestHarness(const char* path) :
    verbosity(ERROR), output_buffer(PROTO_BUFFER_MAX_LEN, 0),
    input_buffer(PROTO_BUFFER_MAX_LEN, 0), tty_fd(-1) {
  Init(path);
}

TestHarness::~TestHarness() {
#ifndef CONFIG_NO_UART
  if (verbosity >= INFO) {
    std::cout << "CLOSING TEST HARNESS" << std::endl;
  }
  if (ttyState()) {
    auto temp = tty_fd;
    tty_fd = -1;
    close(temp);
  }
  if (print_uart_worker) {
    print_uart_worker->join();
    print_uart_worker = nullptr;
  }
#endif  // CONFIG_NO_UART

  if (client) {
    client->Close();
    client = unique_ptr<nos::NuggetClientInterface >();
  }
}

bool TestHarness::ttyState() const {
  return tty_fd != -1;
}

int TestHarness::getVerbosity() const {
  return verbosity;
}

int TestHarness::setVerbosity(int v) {
  int temp = verbosity;
  verbosity = v;
  return temp;
}

void TestHarness::flushConsole() {
#ifndef CONFIG_NO_UART
  while (ReadLineUntilBlock().size() > 0) {}
#endif  // CONFIG_NO_UART
}

bool TestHarness::RebootNugget() {
  return nugget_tools::RebootNugget(client.get());
}

void print_bin(std::ostream &out, uint8_t c) {
  if (c == '\\') {
    out << "\\\\";
  } else if (isprint(c)) {
    out << c;
  } else if (c < 16) {
    out << "\\x0" << std::hex << (uint32_t) c;
  } else {
    out << "\\x" << std::hex << (uint32_t) c;
  }
}

int TestHarness::SendData(const raw_message& msg) {
#ifdef CONFIG_NO_UART
  return SendSpi(msg);
#else
  return FLAGS_util_use_ahdlc ? SendAhdlc(msg) : SendSpi(msg);
#endif  // ANDROID
}

#ifndef CONFIG_NO_UART
int TestHarness::SendAhdlc(const raw_message& msg) {
  if (EncodeNewFrame(&encoder) != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }

  if (EncodeAddByteToFrameBuffer(&encoder, (uint8_t) (msg.type >> 8))
      != AHDLC_OK || EncodeAddByteToFrameBuffer(&encoder, (uint8_t) msg.type)
      != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }
  if (EncodeBuffer(&encoder, msg.data, msg.data_len) != AHDLC_OK) {
    return TRANSPORT_ERROR;
  }

  BlockingWrite((const char*) encoder.frame_buffer,
                encoder.frame_info.buffer_index);
  return NO_ERROR;
}
#endif  // CONFIG_NO_UART

int TestHarness::SendSpi(const raw_message& msg) {
  if (!client) {
    client = nugget_tools::MakeNuggetClient();
    client->Open();
    if (!client->IsOpen()) {
      FatalError("Unable to connect");
    }
  }

  input_buffer.resize(msg.data_len + sizeof(msg.type));
  input_buffer[0] = msg.type >> 8;
  input_buffer[1] = (uint8_t) msg.type;
  std::copy(msg.data, msg.data + msg.data_len, input_buffer.begin() + 2);

  if (verbosity >= INFO) {
    std::cout << "SPI_TX: ";
    for (char c : input_buffer) {
      if (c == '\n') {
        std::cout << "\nSPI_TX: ";
      } else {
        print_bin(std::cout, c);
      }
    }
    std::cout << "\n";
    std::cout.flush();
  }

  output_buffer.resize(output_buffer.capacity());
  return client->CallApp(APP_ID_PROTOBUF, msg.type, input_buffer,
                                &output_buffer);
}

int TestHarness::SendOneofProto(uint16_t type, uint16_t subtype,
                                const google::protobuf::Message& message) {
  test_harness::raw_message msg;
  msg.type = type;
  int msg_size = message.ByteSize();
  if (msg_size + 2 > (int) PROTO_BUFFER_MAX_LEN) {
    return OVERFLOW_ERROR;
  }
  msg.data[0] = subtype >> 8;
  msg.data[1] = (uint8_t) subtype;

  msg.data_len = (uint16_t) (msg_size + 2);
  if (!message.SerializeToArray(msg.data + 2, msg_size)) {
    return SERIALIZE_ERROR;
  }

  auto return_value = SendData(msg);
  return return_value;
}

int TestHarness::SendProto(uint16_t type,
                           const google::protobuf::Message& message) {
  test_harness::raw_message msg;
  msg.type = type;
  int msg_size = message.ByteSize();
  if (msg_size > (int) (PROTO_BUFFER_MAX_LEN - 2)) {
    return OVERFLOW_ERROR;
  }
  msg.data_len = (uint16_t) msg_size;
  if (!message.SerializeToArray(msg.data, msg.data_len)) {
    return SERIALIZE_ERROR;
  }

  auto return_value = SendData(msg);
  return return_value;
}

#ifndef CONFIG_NO_UART
int TestHarness::GetAhdlc(raw_message* msg, microseconds timeout) {
  if (verbosity >= INFO) {
    std::cout << "RX: ";
  }
  size_t read_count = 0;
  while (true) {
    uint8_t read_value;
    auto start = high_resolution_clock::now();
    while (read(tty_fd, &read_value, 1) <= 0) {
      if (timeout >= microseconds(0) &&
         duration_cast<microseconds>(high_resolution_clock::now() - start) >
         microseconds(timeout)) {
        if (verbosity >= INFO) {
          std::cout << "\n";
          std::cout.flush();
        }
        return TIMEOUT;
      }
    }
    ++read_count;

    ahdlc_op_return return_value =
        DecodeFrameByte(&decoder, read_value);

    if (verbosity >= INFO) {
      if (read_value == '\n') {
        std::cout << "\nRX: ";
      } else {
        print_bin(std::cout, read_value);
      }
      std::cout.flush();
    }

    if (read_count > 7) {
      if (return_value == AHDLC_COMPLETE ||
          decoder.decoder_state == DECODE_COMPLETE_BAD_CRC) {
        if (decoder.frame_info.buffer_index < 2) {
          if (verbosity >= ERROR) {
            std::cout << "\n";
            std::cout << "UNDERFLOW ERROR\n";
            std::cout.flush();
          }
          return TRANSPORT_ERROR;
        }

        msg->type = (decoder.pdu_buffer[0] << 8) | decoder.pdu_buffer[1];
        msg->data_len = decoder.frame_info.buffer_index - 2;
        std::copy(decoder.pdu_buffer + 2,
                  decoder.pdu_buffer + decoder.frame_info.buffer_index,
                  msg->data);

        if (verbosity >= INFO) {
          std::cout << "\n";
          if (return_value == AHDLC_COMPLETE) {
            std::cout << "GOOD CRC\n";
          } else {
            std::cout << "BAD CRC\n";
          }
          std::cout.flush();
        }
        return NO_ERROR;
      } else if (decoder.decoder_state == DECODE_COMPLETE_BAD_CRC) {
        if (verbosity >= ERROR) {
          std::cout << "\n";
          std::cout << "AHDLC BAD CRC\n";
          std::cout.flush();
        }
        return TRANSPORT_ERROR;
      } else if (decoder.frame_info.buffer_index >= PROTO_BUFFER_MAX_LEN) {
        if (AhdlcDecoderInit(&decoder, CRC16, NULL) != AHDLC_OK) {
          FatalError("AhdlcDecoderInit()");
        }
        if (verbosity >= ERROR) {
          std::cout << "\n";
          std::cout.flush();
          std::cout << "OVERFLOW ERROR\n";
        }
        return OVERFLOW_ERROR;
      }
    }
  }
}
#endif  // CONFIG_NO_UART

int TestHarness::GetSpi(raw_message* msg, microseconds timeout) {
  if (timeout > microseconds(0)) {}  // Prevent unused parameter warning.
  if (output_buffer.size() < 2) {
    return GENERIC_ERROR;
  }

  if (verbosity >= INFO) {
    std::cout << "SPI_RX: ";
    for (char c : output_buffer) {
      if (c == '\n') {
        std::cout << "\nSPI_RX: ";
      } else {
        print_bin(std::cout, c);
      }
    }
    std::cout << "\n";
    std::cout.flush();
  }

  msg->type = (output_buffer[0] << 8) | output_buffer[1];
  msg->data_len = output_buffer.size() - sizeof(msg->type);
  std::copy(output_buffer.begin() + 2, output_buffer.end(), msg->data);
  output_buffer.resize(0);
  return NO_ERROR;
}

int TestHarness::GetData(raw_message* msg, microseconds timeout) {
#ifdef CONFIG_NO_UART
  return GetSpi(msg, timeout);
#else
  return FLAGS_util_use_ahdlc ? GetAhdlc(msg, timeout) : GetSpi(msg, timeout);
#endif  // CONFIG_NO_UART
}

void TestHarness::Init(const char* path) {
  if (verbosity >= INFO) {
    std::cout << "init() start\n";
    std::cout.flush();
  }

#ifndef CONFIG_NO_UART
  if (FLAGS_util_use_ahdlc) {  // AHDLC UART transport.
    encoder.buffer_len = output_buffer.size();
    encoder.frame_buffer = output_buffer.data();
    if (ahdlcEncoderInit(&encoder, CRC16) != AHDLC_OK) {
      FatalError("ahdlcEncoderInit()");
    }

    decoder.buffer_len = input_buffer.size();
    decoder.pdu_buffer = input_buffer.data();
    if (AhdlcDecoderInit(&decoder, CRC16, NULL) != AHDLC_OK) {
      FatalError("AhdlcDecoderInit()");
    }
  }

  // Setup UART
  errno = 0;
  tty_fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (errno != 0) {
    perror("ERROR open()");
    FatalError("Cannot open debug UART to Citadel chip. Is UltraDebug board connected?");
  }
  errno = 0;

  if (!isatty(tty_fd)) {
    FatalError("Path is not a tty");
  }

  if (tcgetattr(tty_fd, &tty_state)) {
    perror("ERROR tcgetattr()");
    FatalError("");
  }

  if (cfsetospeed(&tty_state, B115200) ||
      cfsetispeed(&tty_state, B115200)) {
    perror("ERROR cfsetospeed()");
    FatalError("");
  }

  tty_state.c_cc[VMIN] = 0;
  tty_state.c_cc[VTIME] = 0;

  tty_state.c_iflag = tty_state.c_iflag & ~(IXON | ISTRIP | INPCK | PARMRK |
      INLCR | ICRNL | BRKINT | IGNBRK);
  tty_state.c_iflag = 0;
  tty_state.c_oflag = 0;
  tty_state.c_lflag = tty_state.c_lflag & ~(ECHO | ECHONL | ICANON | IEXTEN |
      ISIG);
  tty_state.c_cflag = (tty_state.c_cflag & ~(CSIZE | PARENB)) | CS8;

  if (tcsetattr(tty_fd, TCSAFLUSH, &tty_state)) {
    perror("ERROR tcsetattr()");
    FatalError("");
  }
#else
  if (path) {}  // Prevent the unused variable warning for path.
#endif  // CONFIG_NO_UART

  // libnos SPI transport is initialized on first use for interoperability.

  if (verbosity >= INFO) {
    std::cout << "init() finish\n";
    std::cout.flush();
  }

  if (FLAGS_util_print_uart) {
    print_uart_worker = std::unique_ptr<std::thread>(new std::thread(
        [](TestHarness* harness){
          if (harness->getVerbosity() >= INFO) {
            std::cout << "Citadel UART printing enabled!\n";
            std::cout.flush();
          }
          while(harness->ttyState()) {
            harness->PrintUntilClosed();
          }
          if (harness->getVerbosity() >= INFO) {
            std::cout << "Citadel UART printing disabled!\n";
            std::cout.flush();
          }
        }, this));
  }
}

bool TestHarness::UsingSpi() const {
  return !FLAGS_util_use_ahdlc;
}

#ifndef CONFIG_NO_UART
bool TestHarness::SwitchFromConsoleToProtoApi() {
  if (verbosity >= INFO) {
    std::cout << "SwitchFromConsoleToProtoApi() start\n";
    std::cout.flush();
  }

  if (!ttyState()) { return false; }

  ReadUntil(BYTE_TIME * 1024);

  BlockingWrite("version\n", 1);

  ReadUntil(BYTE_TIME * 1024);

  BlockingWrite("\n", 1);

  while (ReadLineUntilBlock() != "> ") {}

  const char command[] = "protoapi uart on 1\n";
  BlockingWrite(command, sizeof(command) - 1);

  ReadUntil(BYTE_TIME * 1024);

  if (verbosity >= INFO) {
    std::cout << "SwitchFromConsoleToProtoApi() finish\n";
    std::cout.flush();
  }

  return true;
}

bool TestHarness::SwitchFromProtoApiToConsole(raw_message* out_msg) {
  if (verbosity >= INFO) {
    std::cout << "SwitchFromProtoApiToConsole() start\n";
    std::cout.flush();
  }

  ControlRequest controlRequest;
  controlRequest.set_type(ControlRequestType::REVERT_TO_CONSOLE);
  string line;
  controlRequest.SerializeToString(&line);

  raw_message msg;
  msg.type = APImessageID::CONTROL_REQUEST;

  std::copy(line.begin(), line.end(), msg.data);
  msg.data_len = line.size();

  if (SendAhdlc(msg) != error_codes::NO_ERROR) {
    return false;
  }


  if (GetAhdlc(&msg, 4096 * BYTE_TIME) == NO_ERROR &&
      msg.type == APImessageID::NOTICE) {
    Notice message;
    message.ParseFromArray((char *) msg.data, msg.data_len);
    if (verbosity >= INFO) {
      std::cout << message.DebugString() << std::endl;
    }
  } else {
    if (verbosity >= ERROR) {
      std::cout << "Receive Error" << std::endl;
      std::cout.flush();
    }
    return false;
  }

  ReadUntil(BYTE_TIME * 4096);

  if (verbosity >= INFO) {
    std::cout << "SwitchFromProtoApiToConsole() finish\n";
    std::cout.flush();
  }
  if (out_msg) {
    *out_msg = std::move(msg);
  }
  return true;
}
#endif  // CONFIG_NO_UART

void TestHarness::BlockingWrite(const char* data, size_t len) {
  if (verbosity >= INFO) {
    std::cout << "TX: ";
    for (size_t i = 0; i < len; ++i) {
      uint8_t value = data[i];
      if (value == '\n') {
        std::cout << "\nTX: ";
      } else {
        print_bin(std::cout, value);
      }
    }
    std::cout << "\n";
    std::cout.flush();
  }

  size_t loc = 0;
  while (loc < len) {
    errno = 0;
    int return_value = write(tty_fd, data + loc, len - loc);
    if (verbosity >= CRITICAL && errno != 0){
      perror("ERROR write()");
    }
    if (return_value < 0) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        FatalError("write(tty_fd,...)");
      } else {
        std::this_thread::sleep_for(BYTE_TIME);
      }
    } else {
      loc += return_value;
    }
  }
}

string TestHarness::ReadLineUntilBlock() {
  if (!ttyState()) {
    return "";
  }

  string line = "";
  line.reserve(128);
  char read_value = ' ';
  std::stringstream ss;

  auto last_success = high_resolution_clock::now();
  while (true) {
    errno = 0;
    while (read_value != '\n' && read(tty_fd, &read_value, 1) > 0) {
      last_success = high_resolution_clock::now();
      print_bin(ss, read_value);
      line.append(1, read_value);
    }
    if (verbosity >= CRITICAL && errno != 0) {
      perror("ERROR read()");
    }

    /* If there wasn't anything to read yet, or the end of line is reached
     * there is no need to continue. */
    if (read_value == '\n' || line.size() == 0 ||
        duration_cast<microseconds>(high_resolution_clock::now() -
                                    last_success) > 4 * BYTE_TIME) {
      break;
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }

  if (verbosity >= INFO && line.size() > 0) {
    std::cout << "RX: " << ss.str() <<"\n";
    std::cout.flush();
  }
  return line;
}

string TestHarness::ReadUntil(microseconds end) {
#ifdef CONFIG_NO_UART
  std::this_thread::sleep_for(end);
  return "";
#else
  if (!ttyState()) {
    return "";
  }

  char read_value = ' ';
  bool first = true;
  std::stringstream ss;

  auto start = high_resolution_clock::now();
  while (duration_cast<microseconds>(high_resolution_clock::now() -
      start) < end) {
    errno = 0;
    while (read(tty_fd, &read_value, 1) > 0) {
      ss << read_value;
      if (verbosity >= INFO) {
        if (first) {
          first = false;
          std::cout << "RX: ";
          print_bin(std::cout, read_value);
        } else if (read_value == '\n') {
          std::cout << "\n";
          std::cout.flush();
          std::cout << "RX: ";
        } else {
          print_bin(std::cout, read_value);
        }
      }
    }
    if (verbosity >= CRITICAL && errno != 0) {
      perror("ERROR read()");
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }
  if (verbosity >= INFO && !first) {
    std::cout << "\n";
    std::cout.flush();
  }

  return ss.str();
#endif  // CONFIG_NO_UART
}

void TestHarness::PrintUntilClosed() {
#ifdef CONFIG_NO_UART
#else
  if (!ttyState()) {
    return;
  }

  char read_value = ' ';
  bool first = true;
  std::stringstream ss("UART: ");

  while (ttyState()) {
    errno = 0;
    while (read(tty_fd, &read_value, 1) > 0) {
      first = false;
      if (read_value == '\r')
        continue;
      if (read_value == '\n') {
        ss << "\n";
        std::cout.flush();
        std::cout << ss.str();
        std::cout.flush();
        ss.str("");
        ss << "UART: ";
      } else {
        print_bin(ss, read_value);
      }
    }
    if (verbosity >= CRITICAL && errno != 0 && errno != EAGAIN) {
      if (errno != EBADF) {
        perror("ERROR read()");
      }
      break;
    }

    /* Wait for at least one bit time before checking read() again. */
    std::this_thread::sleep_for(BIT_TIME);
  }
  if (!first) {
    ss << "\n";
    std::cout.flush();
    std::cout << ss.str();
    std::cout.flush();
  }
#endif  // CONFIG_NO_UART
}


void FatalError(const string& msg) {
  std::cerr << "FATAL ERROR: " << msg << std::endl;
  exit(1);
}

const char* error_codes_name(int code) {
  switch (code) {
    case error_codes::NO_ERROR:
      return "NO_ERROR";
    case error_codes::GENERIC_ERROR:
      return "GENERIC_ERROR";
    case error_codes::TIMEOUT:
      return "TIMEOUT";
    case error_codes::TRANSPORT_ERROR:
      return "TRANSPORT_ERROR";
    case error_codes::OVERFLOW_ERROR:
      return "OVERFLOW_ERROR";
    case error_codes::SERIALIZE_ERROR:
      return "SERIALIZE_ERROR";
    default:
      return "unknown";
  }
}

}  // namespace test_harness
