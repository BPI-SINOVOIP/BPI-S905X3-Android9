#include "poc_test.h"

#include <asm/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SIZE 32

int main(int argc, char* argv[]) {
  VtsHostInput host_input = ParseVtsHostFlags(argc, argv);
  const char* path = host_input.params["path"].c_str();
  if (strlen(path) == 0) {
    fprintf(stderr, "path parameter is empty.\n");
    return POC_TEST_FAIL;
  }

  int ret;
  int fd;
  char buf[SIZE] = {0};

  fd = open(path, O_RDWR);
  if (fd < 0) {
    perror("open fail");
    return POC_TEST_FAIL;
  }
  printf("open %s succ\n", path);

  sprintf(buf, "%x %x", 0x1111111, 0x2222222);
  ret = write(fd, buf, SIZE);
  if (ret < 0) {
    perror("write fail");
    return POC_TEST_FAIL;
  } else {
    printf("succ write %d byte\n", ret);
  }
  close(fd);

  return POC_TEST_PASS;
}
