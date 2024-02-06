#include <Arduino.h>
#include <freertos/stream_buffer.h>

TaskHandle_t task1Handle;
TaskHandle_t task2Handle;
TaskHandle_t task3Handle;
StreamBufferHandle_t streamHandle;

void task1Code(void *pparam) {
  uint8_t b;

  while (1) {
    xStreamBufferReceive(streamHandle, &b, 1, portMAX_DELAY);
    Serial.printf("Received = 0x%02X\n", b);
  }
}

void task2Code(void *pparam) {
  uint8_t n;

  for (n = 0; n < 5; n++) {
    xStreamBufferSend(streamHandle, &n, 1, portMAX_DELAY);
    Serial.printf("0x%02X written to stream\n", n);
    vTaskDelay(500);
  }

  vTaskDelete(NULL);
}

void task3Code(void *pparam) {
  vTaskDelay(5000);

  // vTaskDelete(task2Handle);
  // Serial.println("Task 2 deleted by task 3");

  // vTaskDelete(task1Handle);
  // Serial.println("Task 1 deleted by task 3");

  xStreamBufferReset(streamHandle);
  Serial.println("Stream reset");

  vTaskDelete(NULL);
}

void setup() {
  uint8_t n;

  Serial.begin(230400);
  Serial.println();
  Serial.println("Starting");

  streamHandle = xStreamBufferCreate(1024, 30);
  //xStreamBufferSetTriggerLevel(streamHandle, 1025);

  //xTaskCreatePinnedToCore(task1Code, "", 8192, NULL, 10, &task1Handle, 1);
  xTaskCreatePinnedToCore(task2Code, "", 8192, NULL, 10, &task2Handle, 1);
  return;




  xTaskCreatePinnedToCore(task1Code, "", 8192, NULL, 10, &task1Handle, 1);
  for (n = 0; n < 5; n++) {
    xStreamBufferSend(streamHandle, &n, 1, portMAX_DELAY);
    Serial.printf("0x%02X written to first stream\n", n);
  }
  vTaskDelete(task1Handle);  // We can delete this task but we can't re-create it cos the wait on the stream isn't released so RTOS complains when it gets another wait.
  vStreamBufferDelete(streamHandle); // So we need to delete the stream after deleting the waiting task and re-create it.
  streamHandle = xStreamBufferCreate(1024, 5);
  xTaskCreatePinnedToCore(task1Code, "", 8192, NULL, 10, &task1Handle, 1);
  for (n = 5; n < 10; n++) {
    xStreamBufferSend(streamHandle, &n, 1, portMAX_DELAY);
    Serial.printf("0x%02X written to second stream\n", n);
  }
  Serial.println("Stream filled");

  /*
  Writing to a stream where there is a deleted task pending causes a crash. Deleting the sending task first makes it OK.
  */
}

void loop() {
}