#include "poc_test.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  VtsHostInput host_input = ParseVtsHostFlags(argc, argv);
  struct sockaddr sa;
  socklen_t len, i;
  int fd;

  fd = socket(AF_BLUETOOTH, SOCK_STREAM, 3);
  if (fd == -1) {
    printf("[-] can't create socket: %s\n", strerror(errno));
    return POC_TEST_SKIP;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sa_family = AF_BLUETOOTH;

  if (bind(fd, &sa, 2)) {
    printf("[-] can't bind socket: %s\n", strerror(errno));
    close(fd);
    return POC_TEST_SKIP;
  }

  len = sizeof(sa);
  if (getsockname(fd, &sa, &len)) {
    printf("[-] can't getsockname for socket: %s\n", strerror(errno));
    close(fd);
    return POC_TEST_SKIP;
  } else {
    printf("[+] getsockname return len = %d\n", len);
  }

  for (i = 0; i < len; i++) {
    printf("%02x ", ((unsigned char*)&sa)[i]);
  }
  printf("\n");

  for (i = 1; i < len; i++) {
    if (((unsigned char*)&sa)[i] != 0) {
      return POC_TEST_FAIL;
    }
  }

  close(fd);
  return POC_TEST_PASS;
}
