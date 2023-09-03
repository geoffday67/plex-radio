#include "Scroll.h"
#include "Output.h"
#include "utils.h"

Scroll::Scroll(char *ptext, int line) {
  this->ptext = strdup(ptext);
  this->line = line;
}

Scroll::~Scroll() {
  if (taskHandle) {
    // Trigger task to stop.
    xTaskNotify(taskHandle, 0, eIncrement);

    // Wait till task says it's safe to delete it.
    xSemaphoreTake(semaphoreQuit, pdMS_TO_TICKS(2000));
    vSemaphoreDelete(semaphoreQuit);
    vTaskDelete(taskHandle);
  }
  free(ptext);
}

void Scroll::begin() {
  semaphoreQuit = xSemaphoreCreateBinary();
  taskHandle = startTask(scrollCode, "Scroll", this);
}

void Scroll::scrollCode(void *pdata) {
  Scroll *pscroll = (Scroll *)pdata;
  char *ptext = pscroll->ptext;
  int start, length = strlen(ptext), line = pscroll->line;

  for (start = 0; length - start >= OUTPUT_WIDTH; start++) {
    Output.setLine(line, ptext + start);
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200)) == 1) {
      // We got a notification to quit, indicate that it's safe to delete us.
      xSemaphoreGive(pscroll->semaphoreQuit);
      vTaskSuspend(NULL);
      return;
    }
  }

  Output.setLine(line, ptext);
  xSemaphoreGive(pscroll->semaphoreQuit);
  vTaskSuspend(NULL);
}

