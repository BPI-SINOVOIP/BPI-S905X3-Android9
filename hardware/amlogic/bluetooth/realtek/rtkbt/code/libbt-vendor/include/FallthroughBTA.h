#ifndef __FALLTHROUGHBAD_H__
#define __FALLTHROUGHBAD_H__
#include <fcntl.h>
#include <cstdint>
#include <string>
#include <vector>

namespace android {

// Check for a legacy address stored as a property.
static constexpr char PERSIST_BDADDR_PROPERTY[] =
    "persist.service.bdroid.bdaddr";

 //Encapsulate handling for Bluetooth Addresses:
class FallthroughBTA {
 public:
    // Conversion constants
    static constexpr size_t kStringLength = sizeof("XX:XX:XX:XX:XX:XX") - 1;
    static constexpr size_t kBytes = (kStringLength + 1) / 3;

    static void bytes_to_string(const uint8_t* addr, char* addr_str);

    static bool string_to_bytes(const char* addr_str, uint8_t* addr);

    FallthroughBTA();
};

}
#endif
