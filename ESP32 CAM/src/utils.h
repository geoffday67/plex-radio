#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

enum SortOrder {
  None,
  Artist,
  Title
};

extern bool begins(char *, char *);
extern char *strnthchr(char *, char, int);
extern void strlcpy(char *pdest, const char *psrc, int max);
extern void decodeUTF8(char *pdest, const char *psrc);
extern TaskHandle_t startTask(TaskFunction_t pcode, const char *const pname, void *const pparameters = 0);