#include <Wire.h>
#include <driver/i2s.h>

#define I2S_BCK_IO 10
#define I2S_LRCK_IO 11
#define I2S_DATA_OUT_IO 12
#define I2S_DATA_IN_IO 13

const int I2S_NUM = 0;
const int I2S_DMA_CHANNEL = 1;
const int I2S_SAMPLE_RATE = 44100;
const int I2S_BITS_PER_SAMPLE = 24;
const int I2S_CHANNEL_NUM = 1;
const int I2S_MAX_BUFFER = 32;

void i2sReadTask(void *pvParameters)
{
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 2,
    .dma_buf_len = I2S_MAX_BUFFER
  };
  
  i2s_pin_config_t i2s_pin_config = {
    .bck_io_num = I2S_BCK_IO,
    .ws_io_num = I2S_LRCK_IO,
    .data_out_num = I2S_DATA_OUT_IO,
    .data_in_num = I2S_DATA_IN_IO
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

  size_t bytes_read;
  uint32_t sample_data[I2S_MAX_BUFFER];

  while (1) {
    i2s_read(I2S_NUM_0, (void*)sample_data, I2S_MAX_BUFFER * 4, &bytes_read, portMAX_DELAY);
    
    // Do something with the sample data
    for (int i = 0; i < bytes_read / 4; i++) {
      // ...
    }
  }

  i2s_driver_uninstall(I2S_NUM_0);
}

void setup() {
  xTaskCreate(i2sReadTask, "i2sReadTask", 2048, NULL, 5, NULL);
}

void loop() {
  // ...
}
