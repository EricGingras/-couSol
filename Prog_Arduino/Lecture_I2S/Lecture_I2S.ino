#include "Arduino.h"
#include "FS.h"
#include "I2S.h"
#include "SD.h"
#include "SPI.h"

const int record_time = 3;  // seconds
const char filename[] = "/sound.raw";

const int buffer_size = 4096; // number of bytes to read from I2S interface
byte buffer[buffer_size];
size_t bytes_read;
File file;

void setup() {
  Serial.begin(115200);
  if (!SD.begin()) {
    Serial.println("SD begin failed");
    return;
  }
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  I2S_Init(I2S_MODE_RX, I2S_BITS_PER_SAMPLE_32BIT);
  Serial.println("Start recording");
}

void loop() {
  static unsigned long start_time = 0;
  unsigned long current_time = millis();

  // Record for record_time seconds
  if (current_time - start_time < record_time * 1000) {
    // Read data from I2S interface
    i2s_read(I2S_NUM_0, (char *)buffer, buffer_size ,&bytes_read, portMAX_DELAY);
    
    // Write data to file
    file.write(buffer, bytes_read);
  } else {
    // Stop recording
    file.close();
    Serial.println("Recording stopped");
    while (1); // Hang indefinitely
  }
}
