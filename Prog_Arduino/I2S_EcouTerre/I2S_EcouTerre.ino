#include "Arduino.h"
#include <cstdint>
#include <cstring>
#include "FS.h"
#include "Wav.h"
#include "I2S.h"
#include "SD.h"
#include "SPI.h"

const int record_time = 10;  // seconds
const int buffer_size = 48; // number of bytes to read from I2S interface 
const int headerSize = 44;
const int waveDataSize = record_time * 176400;
const char filename[] = "/sound.wav";


byte buffer[buffer_size];
byte newBuffer[buffer_size/2];
int32_t intArray[12];
int32_t newIntArray[12];
byte header[headerSize];
size_t bytes_read;
File file;

void byteArrayToInt32(const uint8_t* byteArray, const int byteArrayLength, int32_t* intArray) 
{
    std::memcpy(intArray, byteArray, byteArrayLength / 4 * sizeof(int32_t));
}

void int32ToByteArray(const int32_t* intArray, const int intArrayLength, uint8_t* byteArray) 
{
    std::memcpy(byteArray, intArray, intArrayLength * sizeof(int32_t));
}

void setup() {
  Serial.begin(115200);
  if (!SD.begin()) Serial.println("SD begin failed");
  while(!SD.begin()){
    Serial.print(".");
    delay(500);
  }

  CreateWavHeader(header, waveDataSize);
  SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.write(header, headerSize);
  I2S_Init(I2S_MODE_RX, I2S_BITS_PER_SAMPLE_32BIT);
  Serial.println("start recording");
}

void loop() {
  static unsigned long start_time = 0;
  unsigned long current_time = millis();

  // Record for record_time seconds
  if (current_time - start_time < record_time * 1000) {
    // Read data from I2S interface
    i2s_read(I2S_NUM_0, (char *)buffer, buffer_size, &bytes_read, portMAX_DELAY);

    byteArrayToInt32(buffer, 48, intArray);

    /*
    for (int i = 0; i < 12; i++) {
        if (i % 2 == 0) {
            // if the index is even, copy the corresponding value from intArray to newIntArray
            newIntArray[i] = intArray[i];
        } else {
            // otherwise, calculate the average of two adjacent elements in intArray
            newIntArray[i] = (intArray[i-1] + intArray[i+1]) / 2;
        }
        newIntArray[i] = newIntArray[i] * 4;
    }
    */
    int j = 0;
    for (int i = 0; i <= 12; i +=2)
    {
      newIntArray[j] = intArray[i];
      //newIntArray[j] = newIntArray[j] * 15;
      j++;
    }
    

    int32ToByteArray(newIntArray, 12, newBuffer);
    
    
    /*
    int j = 0;
    for (int i = 0; i < buffer_size; i+=8)
    {
      newBuffer[j] = buffer[i];
      newBuffer[j + 1] = buffer[i + 1];
      newBuffer[j + 2] = buffer[i + 2];
      newBuffer[j + 3] = buffer[i + 3];
      j += 4;
    }

    
    int j = 0;
    for (int i = 1; i < buffer_size; i+=4)
    {
      newBuffer[j++] = buffer[i];
      newBuffer[j++] = buffer[i + 1];
      newBuffer[j++] = buffer[i + 2];
      
    }
    */

  

    // Write data to file
    file.write(newBuffer, buffer_size/2);
  } else {
    // Stop recording
    file.close();
    Serial.println("Recording stopped");
    while (1); // Hang indefinitely
  }
}
