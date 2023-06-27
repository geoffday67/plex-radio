#include <string.h>
#include <strings.h>

bool begins(char *pdest, char *pfind) {
  return !strncasecmp(pdest, pfind, strlen(pfind));
}

char* strnthchr(char* pdest, char find, int count) {
  int n;
  char *pc = pdest;

  for (n = 0; n < count; n++) {
    pc = strchr (pc, find);
    if (!pc) {
      return 0;
    }
    pc++;
  }
  return --pc;
}
