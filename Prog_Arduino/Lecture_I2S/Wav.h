#include <Arduino.h>

// 24bit, monoral, 44100Hz,  linear PCM
void CreateWavHeader(byte* header, int waveDataSize);  // size of header is 44
