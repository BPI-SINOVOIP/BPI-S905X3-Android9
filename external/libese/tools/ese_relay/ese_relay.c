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
 *
 * Hack-y server to forward communication with an eSE during development.
 * See README.md for more information.
 */

#include <arpa/inet.h>
#include <linux/un.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LOG_TAG "ese-relay"
#include <ese/ese.h>
#include <ese/log.h>

extern const uint8_t *kAtr;
extern size_t kAtrLength;
extern void *kEseOpenData;
void ese_relay_init(struct EseInterface *ese);

/*
 * Aligned with vpcd.h in
 * https://frankmorgner.github.io/vsmartcard/virtualsmartcard
 */
#define CMD_POWER_OFF 0
#define CMD_POWER_ON 1
#define CMD_RESET 2
#define CMD_ATR 4

int setup_socket(const char *name) {
  int fd;
  struct sockaddr_un addr;

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  if (strlen(name) > UNIX_PATH_MAX - 1) {
    ALOGE("Abstract listener name too long.");
    return -1;
  }
  strncpy(&addr.sun_path[1], name, strlen(name));
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    ALOGE("Could not open socket.");
    return fd;
  }
  if (bind(fd, (struct sockaddr *)&addr,
           sizeof(sa_family_t) + strlen(name) + 1) == -1) {
    ALOGE("Failed to bind to abstract socket name");
    close(fd);
    return -1;
  }
  return fd;
}

int main() {
  int server_fd = setup_socket(LOG_TAG);
  struct EseInterface ese;
  ese_relay_init(&ese);

  if (listen(server_fd, 4)) {
    ALOGE("Failed to listen on socket.");
    close(server_fd);
    return -1;
  }

  while (server_fd) {
    struct sockaddr client_info;
    socklen_t client_info_len = (socklen_t)sizeof(&client_info);
    int client_fd;
    uint32_t tx_len, data_read;
    uint32_t rx_len;
    uint16_t network_tx_len;
    uint16_t network_rx_len;
    uint8_t tx_buf[4096];
    uint8_t rx_buf[4096];
    int connected = 0;

    if ((client_fd = accept(server_fd, &client_info, &client_info_len)) == -1) {
      ALOGE("Fatal error accept()ing a client connection.");
      return -1;
    }
    printf("Client connected.\n");
    connected = 1;
    if (ese_open(&ese, kEseOpenData)) {
      ALOGE("Cannot open hw");
      if (ese_error(&ese))
        ALOGE("eSE error (%d): %s", ese_error_code(&ese),
              ese_error_message(&ese));
      return 1;
    }
    printf("eSE is open\n");

    while (connected) {
      printf("Listening for data from client\n");
      if (read(client_fd, &network_tx_len, sizeof(network_tx_len)) !=
          sizeof(network_tx_len)) {
        ALOGE("Client disconnected.");
        break;
      }
      tx_len = (uint32_t)ntohs(network_tx_len);
      printf("tx_len: %u\n", tx_len);
      if (tx_len == 0) {
        ALOGE("Client had nothing to say. Goodbye.");
        break;
      }
      if (tx_len > sizeof(tx_buf)) {
        ALOGE("Client payload too large: %u", tx_len);
        break;
      }
      for (data_read = 0; data_read < tx_len;) {
        printf("Reading payload: %u of %u remaining\n", data_read, tx_len);
        ssize_t bytes = read(client_fd, tx_buf + data_read, tx_len - data_read);
        if (bytes < 0) {
          ALOGE("Client abandoned hope during transmission.");
          connected = 0;
          break;
        }
        data_read += bytes;
      }
      /* Finally, we can transcieve. */
      if (tx_len) {
        uint32_t i;
        printf("Sending %u bytes to card\n", tx_len);
        printf("TX: ");
        for (i = 0; i < tx_len; ++i)
          printf("%.2X ", tx_buf[i]);
        printf("\n");
      }

      if (tx_len == 1) { /* Control request */
        printf("Received a control request: %x\n", tx_buf[0]);
        rx_len = 0;
        switch (tx_buf[0]) {
        case CMD_POWER_OFF:
          ese.ops->hw_reset(&ese);
          break;
        case CMD_POWER_ON:
          break;
        case CMD_RESET:
          ese.ops->hw_reset(&ese);
          break;
        case CMD_ATR:
          /* Send a dummy ATR for another JCOP card */
          rx_len = kAtrLength;
          printf("Filling card RX buf with fake ATR (%u)\n", rx_len);
          memcpy(rx_buf, kAtr, rx_len);
          printf("Sending back ATR of length %u\n", rx_len);
          break;
        default:
          ALOGE("Unknown control byte seen: %x", tx_buf[0]);
        }
        if (!rx_len)
          continue;
      } else {
        rx_len = ese_transceive(&ese, tx_buf, tx_len, rx_buf, sizeof(rx_buf));
        if (ese_error(&ese)) {
          ALOGE("An error (%d) occurred: %s", ese_error_code(&ese),
                ese_error_message(&ese));
          return -1;
        }
      }
      if (rx_len > 0) {
        uint32_t i;
        printf("Read %d bytes from card\n", rx_len);
        printf("RX: ");
        for (i = 0; i < rx_len; ++i)
          printf("%.2X ", rx_buf[i]);
        printf("\n");
      }

      /* Send to client */
      network_rx_len = htons((uint16_t)rx_len);
      if (write(client_fd, &network_rx_len, sizeof(network_rx_len)) !=
          sizeof(network_rx_len)) {
        ALOGE("Client abandoned hope during response size.");
        break;
      }

      for (data_read = 0; data_read < rx_len;) {
        ssize_t bytes =
            write(client_fd, rx_buf + data_read, rx_len - data_read);
        if (bytes < 0) {
          ALOGE("Client abandoned hope during response.");
          connected = 0;
          break;
        }
        data_read += bytes;
      }
      usleep(1000);
    }
    close(client_fd);
    printf("Session ended\n\n");
    ese_close(&ese);
  }
  return 0;
}
