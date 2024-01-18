#include <Arduino.h>
#include <string.h>
#include <strings.h>

#include "constants.h"Ï

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
