#ifndef SRC_MACROS_H
#define SRC_MACROS_H

#define ARRAYSIZE(a)            \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#endif  // SRC_MACROS_H
