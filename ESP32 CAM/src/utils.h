#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

extern bool begins(char *, char *);
extern char *strnthchr(char *, char, int);
extern void strlcpy(char *pdest, const char *psrc, int max);
extern TaskHandle_t startTask(TaskFunction_t pcode, const char *const pname, void *const pparameters = 0);