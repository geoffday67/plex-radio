#include <Arduino.h>
#include <string.h>
#include <strings.h>

#include "constants.h"
#include "screens/Output.h"

bool begins(char *pdest, char *pfind) {
  return !strncasecmp(pdest, pfind, strlen(pfind));
}

char *strnthchr(char *pdest, char find, int count) {
  int n;
  char *pc = pdest;

  for (n = 0; n < count; n++) {
    pc = strchr(pc, find);
    if (!pc) {
      return 0;
    }
    pc++;
  }
  return --pc;
}

void strlcpy(char *pdest, const char *psrc, int max) {
  while (--max != 0) {
    if ((*pdest++ = *psrc++) == '\0')
      break;
  }

  *pdest = '\0';
}

void decodeUTF8(char *pdest, const char *psrc) {
  char c;
  uint16_t replacement;

  do {
    c = *psrc++;
    replacement = 0;
    if ((c & 0xE0) == 0xC0) {
      // It's a 2 byte char, get the next byte to decode it. We only handle 2 byte characters.
      replacement = ((c & 0x03) << 6) | (*psrc++ & 0x3F);
    }
    if (replacement) {
      switch (replacement) {
        case 0xF6:
          c = O_UMLAUT;
          break;
        case 0xE9:
          c = E_ACUTE;
          break;
        default:
          c = replacement & 0x00FF;
          break;
      }
    }
    *pdest++ = c;

  } while (c);
}

TaskHandle_t startTask(TaskFunction_t pcode, const char *const pname, void *const pparameters = 0) {
  TaskHandle_t handle;
  xTaskCreatePinnedToCore(
      pcode,
      pname,
      ARDUINO_STACK,
      pparameters,
      ARDUINO_PRIORITY,
      &handle,
      ARDUINO_CORE);
  return handle;
}
