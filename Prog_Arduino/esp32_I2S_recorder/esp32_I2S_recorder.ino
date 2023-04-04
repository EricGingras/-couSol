#include "Arduino.h"
#include "FS.h"
#include "Wav.h"
#include "I2S.h"
#include "SD.h"
#include "SPI.h"


//comment the first line and uncomment the second if you use MAX9814
#define I2S_MODE I2S_MODE_RX
//#define I2S_MODE I2S_MODE_ADC_BUILT_IN

const int record_time = 3;  // second
const char filename[] = "/sound.wav";

const int headerSize = 44;
const int waveDataSize = record_time * 132000;
const int numCommunicationData = 44;
const int numPartWavData = numCommunicationData/4;
byte header[headerSize];
int32_t communicationData[numPartWavData]; //était char avant
int16_t sd_data[numPartWavData];
char partWavData[numPartWavData];
size_t bytesRead;
File file;

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
  if (!file) return;
  file.write(header, headerSize);
  I2S_Init(I2S_MODE, I2S_BITS_PER_SAMPLE_32BIT);
  Serial.println("start recording");
  for (int j = 0; j < waveDataSize/numPartWavData; ++j) {
    i2s_read(I2S_NUM_0, (char *)communicationData, numCommunicationData ,&bytesRead, portMAX_DELAY);

    
    for (int i = 0; i < numPartWavData; i++)
    {
      communicationData[i] >>= 8;
      sd_data[i] = (int16_t)communicationData[i];
      //sd_data[i] = 0x1234;
    }    

    //printf("[0]%d [1]%d [2]%d [3]%d \n", communicationData[0], communicationData[1], communicationData[2], communicationData[3]);
    file.write((const byte*)sd_data, (numCommunicationData/2));
    //delay(100);
  }
  file.close();
  Serial.println("finish");
}

void loop() {
}
