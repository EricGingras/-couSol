#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_check.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "esp_vfs_fat.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <rom/ets_sys.h>


#define I2S_NUM         I2S_NUM_0
#define I2S_Q_LEN       (8)
#define CHANNEL         (1)
#define SAMPLE_RATE     (44100)
#define BIT_WIDTH       (24)
#define I2S_NUM         (0)
#define I2S_READ_LEN    (1024)
#define PI 3.141592653589793  // value of pi
#define AMPLITUDE ((1<<23) -1)
#define SHIFT_BITS (8)

#define SPI_DMA_CHAN     SPI_DMA_CH_AUTO
#define SD_MOUNT_POINT      "/sdcard"
#define SD_CS           (GPIO_NUM_5)
#define SD_MOSI         (GPIO_NUM_23)
#define SD_CLK          (GPIO_NUM_18)
#define SD_MISO         (GPIO_NUM_19)
#define GPIO_I2S_WS     (GPIO_NUM_14)
#define GPIO_I2S_BCLK   (GPIO_NUM_13)
#define GPIO_I2S_DIN    (GPIO_NUM_12)

#define SAMPLE_RATE 44100
#define BIT_SAMPLE 24
#define NUM_CHANNELS 1
#define REC_TIME 1

#define WAV_HEADER_SIZE 44
#define SAMPLE_SIZE         (CONFIG_BIT_SAMPLE * 1024)   
#define BYTE_RATE           (SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS 
size_t bytes_read;

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;

static QueueHandle_t i2s_event_queue;

static const char *TAG = "i2s_GPT";

void mount_sdcard(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = SD_MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    // Use POSIX and C standard library functions to work with files.
}

void generate_wav_header(char *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
    // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
    uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
    uint32_t byte_rate = BYTE_RATE;

    const char set_wav_header[] = {
        'R', 'I', 'F', 'F', // ChunkID
        //0x02, 0x04, 0xF0, 0x00, //ChunkSize
        file_size, file_size >> 8, file_size >> 16, file_size >> 24, // ChunkSize
        'W', 'A', 'V', 'E', // Format
        'f', 'm', 't', ' ', // Subchunk1ID
        0x10, 0x00, 0x00, 0x00, // Subchunk1Size (16 for PCM)
        0x01, 0x00, // AudioFormat (1 for PCM)
        0x01, 0x00, // NumChannels (1 channel)
        0x44, 0xAC, 0x00, 0x00, // SampleRate (44100)
        0xCC, 0x04, 0x02, 0x00, // ByteRate
        //byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24, // ByteRate
        0x03, 0x00, // BlockAlign 3 = 24 bit 1 channel ; 2 = 16 bit 1 channel
        0x18, 0x00, // BitsPerSample (0x18 = 24 bits) (0x10 = 16 bits)
        'd', 'a', 't', 'a', // Subchunk2ID
        //0x02, 0x04, 0xCC, 0x00, //Subchunk2Size
        wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24, // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

void i2s_setup() {
    ESP_LOGI(TAG, "Initializing I2S");
    //int i = 0;
    /*unsigned int i2s_read_len = I2S_READ_LEN;
    size_t bytes_read = 0;
    int i2s_read_buff[i2s_read_len];*/
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_SLAVE | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BIT_WIDTH,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 3,
        .dma_buf_len = 1024,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = GPIO_I2S_BCLK,
        .ws_io_num = GPIO_I2S_WS,
        .data_in_num = GPIO_I2S_DIN,
        .data_out_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM, &i2s_config, I2S_Q_LEN, &i2s_event_queue);
    i2s_set_pin(I2S_NUM, &pin_config);
    /*
    while (1) {
        i = 0;
        if((i2s_read(I2S_NUM, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY)) ==ESP_OK)
        {
            ESP_LOGI(TAG, "[echo] %d byte.", bytes_read);
            while(i < (1024/4))
            {
                ESP_LOGI(TAG, "Data du buffer position %d: %d", i, (int)i2s_read_buff[i]);
                i++;
            }
        }
            
        // process received data
        else
            ESP_LOGI(TAG,"ERROR");
        
    }
    */
}

void app_main() {
    //xTaskCreate(i2s_task, "i2s_task", 8198, NULL, 5, NULL);
    mount_sdcard();
    i2s_setup();

    // Use POSIX and C standard library functions to work with files.
    uint32_t rec_time = REC_TIME;
    int flash_wr_size = 0;
    ESP_LOGI(TAG, "Opening file");

    char wav_header_fmt[WAV_HEADER_SIZE];
    uint32_t flash_rec_time = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_time, SAMPLE_RATE);

    ESP_LOGI(TAG, "Generating WAV header");
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(SD_MOUNT_POINT"/record.wav", &st) == 0) {
        // Delete it if it exists
        unlink(SD_MOUNT_POINT"/record.wav");
    }

    ESP_LOGI(TAG, "WAV file verification");
    // Create new WAV file
    FILE *f = fopen(SD_MOUNT_POINT"/record.wav", "wb"); //a (append) before now wb (new file and treat it as binary)
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    ESP_LOGI(TAG, "Creating WAV file in SD card");
    // Write the header to the WAV file
    fwrite(wav_header_fmt, 1, WAV_HEADER_SIZE, f);
    ESP_LOGI(TAG, "Finished writing WAV file header");




	// Allocate DMA buffers
	size_t buf_size = 1024 * sizeof(int32_t);
	int32_t *dma_buf = (int32_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    int32_t *buf_ptr = (int32_t*) dma_buf;
    int32_t shiftValue = 0x00FF << SHIFT_BITS; // create the shift value

    
	if (!dma_buf) {
		ESP_LOGE(TAG, "Failed to allocate DMA buffer");
		fclose(f);
		return;
	}
    ESP_LOGI(TAG, "Allocating DMA buffer");
    //Code tester carte SD et format WAV ainsi que décalage 8bit vers la gauche
    size_t bytes_written = 0;

    
    //génération d'onde carré
    double t = 0;
    double sin_value = 0;
    int32_t sample = 0;
    int i = 0;
    while (flash_wr_size < flash_rec_time) {
        t = (double)i / SAMPLE_RATE;
        sin_value = sin(2.0 * PI * 1000.0 * t); // 1 kHz sine wave
        if(i% 500 == 0)
            sample = 0;//(uint32_t)(AMPLITUDE * sin_value) & 0xFFFFFF; // convert to 24-bit unsigned integer
        if(i% 1500 == 0)
            sample = 0x000FFFFF;//(uint32_t)(AMPLITUDE * sin_value) & 0xFFFFFF; // convert to 24-bit unsigned integer 
        //ESP_LOGI(TAG, "Generation d'onde sinus : %d", (int)sample);
        dma_buf[i%1024] = sample;
        //bytes_written += fwrite(sample, 4, 1, f);
        flash_wr_size += 4;
        i++;
        if(i%1024 == 0)
        {
            printf("dma_buf 0: %lu\r\n", dma_buf[0]);
            printf("dma_buf 1: %lu\r\n", dma_buf[1]);
            
            for (int j = 0; j < 1024; j++) {
               dma_buf[j] <<= SHIFT_BITS;
            }
            printf("dma_buf 0: %lu\r\n", dma_buf[0]);
            printf("dma_buf 1: %lu\r\n", dma_buf[1]);
            
            //bytes_written += fwrite(dma_buf, 1024, 4, f);
        }
            
    }
    bytes_written += fwrite(dma_buf[0], 4, 1, f);
    ets_delay_us(1000);			//Stalls execution for #uS
    bytes_written += fwrite(dma_buf[1], 4, 1, f);
    ets_delay_us(1000);			//Stalls execution for #uS
    bytes_written += fwrite(dma_buf[2], 4, 1, f);
    ets_delay_us(1000);			//Stalls execution for #uS
    bytes_written += fwrite(dma_buf[3], 4, 1, f);
    ets_delay_us(1000);			//Stalls execution for #uS
    bytes_written += fwrite(dma_buf[4], 4, 1, f);
    ets_delay_us(1000);			//Stalls execution for #uS


    //Code enregistrement audio du PCM
     /*
	// Receive and write audio data to file
	size_t bytes_written = 0;
    //QueueHandle_t i2s_event_queue = i2s_get_event_queue(I2S_NUM_0);
	while (flash_wr_size < flash_rec_time) {
		// Receive audio data
		i2s_event_t evt;
		if (xQueueReceive(i2s_event_queue, &evt, portMAX_DELAY)) {
			if (evt.type == I2S_EVENT_RX_DONE) {
				i2s_read(I2S_NUM, (char *)dma_buf, buf_size, &bytes_read, portMAX_DELAY);
				flash_wr_size += bytes_read;

				// Write audio data to file
				if (bytes_read > 0) {
					size_t bytes = fwrite(dma_buf, 1, bytes_read, f);
					bytes_written += bytes;
				}
			}
		}
        
        flash_wr_size += bytes_read;
	}
    */
    ESP_LOGI(TAG, "Finished writing AUDIO into file");

	// Update WAV header with total data size
    ESP_LOGI(TAG, "total bytes to write = %d", flash_wr_size);
    ESP_LOGI(TAG, "Nomber of bytes written = %d", bytes_written);

    ESP_LOGI(TAG, "Closing file");
	fclose(f);
    //esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    return;
}
