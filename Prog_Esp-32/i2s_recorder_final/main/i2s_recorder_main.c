/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

static const char *TAG = "pcm_recorder";

#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO
#define NUM_CHANNELS        1 // For mono recording only!
#define SD_MOUNT_POINT      "/sdcard"
#define SAMPLE_SIZE         1024
#define BYTE_RATE           (CONFIG_SAMPLE_RATE * (CONFIG_BIT_SAMPLE / 8)) * NUM_CHANNELS
#define BUTTON_PIN 25
#define LED_PIN 0

void mount_sdcard(void);
void init_i2s_driver(void);
void record_wav(void*);

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;
i2s_chan_handle_t rx_handle = NULL;

int32_t i2s_readraw_buff[SAMPLE_SIZE];

size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

static volatile bool is_recording = false;
QueueHandle_t interputQueue;

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

void Record_Control_Task(void *params)
{
    int pinNumber, count = 0;
    while(true)
    {
        if (xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY))
        {
            while(gpio_get_level(BUTTON_PIN) == 0)
            {
                vTaskDelay(10);
            }
            while(gpio_get_level(BUTTON_PIN) == 1)
            {
                vTaskDelay(10);
            }
            printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level(BUTTON_PIN));
            if(is_recording == false)
            {
                is_recording = true;

                // Mount the SDCard for recording the audio file
                mount_sdcard();

                // Acquire a I2S PCM channel for the PCM digital microphone
                init_i2s_driver();

                // Enable the I2S channel
                ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

                ESP_LOGI(TAG, "Starting recording!");

                // Start Recording
                xTaskCreatePinnedToCore(record_wav, "Record_Wav", 4096, NULL, 1, NULL, 1);

            }  
            else
            {
                is_recording = false;
            }
        }
        vTaskDelay(10);
    }
}

void init_interrupt(void)
{
    //gpio_pad_select_gpio(LED_PIN);
    //gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON_PIN);
    gpio_pullup_dis(BUTTON_PIN);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_POSEDGE);

    interputQueue = xQueueCreate(10, sizeof(int));
    xTaskCreatePinnedToCore(Record_Control_Task, "Record_Control_Task", 4096, NULL, 2, NULL, 0);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_interrupt_handler, (void *)BUTTON_PIN);
}

void mount_sdcard(void)
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    ESP_LOGI(TAG, "Initializing SD card");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CONFIG_SPI_MOSI,
        .miso_io_num = CONFIG_SPI_MISO,
        .sclk_io_num = CONFIG_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CONFIG_SPI_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}

void generate_wav_header(char *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
    // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
    uint32_t file_size = wav_size + WAVE_HEADER_SIZE - 8;
    uint32_t byte_rate = BYTE_RATE;
    
    //Currently set up as 32 bit mono
    const char set_wav_header[] = {
        'R', 'I', 'F', 'F', // ChunkID
        file_size, file_size >> 8, file_size >> 16, file_size >> 24, // ChunkSize
        'W', 'A', 'V', 'E', // Format
        'f', 'm', 't', ' ', // Subchunk1ID
        0x10, 0x00, 0x00, 0x00, // Subchunk1Size (16 for PCM)
        0x01, 0x00, // AudioFormat (1 for PCM)
        0x01, 0x00, // NumChannels (1 channel)
        0x88, 0x58, 0x01, 0x00, // SampleRate (88200)
        byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24, // ByteRate
        0x04, 0x00, // BlockAlign 4 = 32 bit 1 channel ; 3 = 24 bit 1 channel ; 2 = 16 bit 1 channel
        0x20, 0x00, // BitsPerSample (32 bits)
        'd', 'a', 't', 'a', // Subchunk2ID
        wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24, // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

void record_wav(void*)
{
    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    int rec_time = 5;
    ESP_LOGI(TAG, "Opening file");

    char wav_header_fmt[WAVE_HEADER_SIZE];
    uint32_t flash_rec_time = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_time, CONFIG_SAMPLE_RATE);
    ESP_LOGI(TAG, "Generating WAV header");

    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(SD_MOUNT_POINT"/record.wav", &st) == 0) {
        // Delete it if it exists
        unlink(SD_MOUNT_POINT"/record.wav");
    }

    ESP_LOGI(TAG, "WAV file verification");
    // Create new WAV file
    FILE *f = fopen(SD_MOUNT_POINT"/record.wav", "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Creating WAV file");
    // Write the header to the WAV file
    fwrite(wav_header_fmt, 1, WAVE_HEADER_SIZE, f);

    while (is_recording) { 
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, (void *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
            // Write the samples to the WAV file
            flash_wr_size += bytes_read;
            fwrite(i2s_readraw_buff, 1, bytes_read, f);
        }
        else {
            printf("Read Failed!\n");
        }
    }
    fclose(f);

    // Create new WAV file
    f = fopen(SD_MOUNT_POINT"/record.wav", "r+");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for readig and writing");
        vTaskDelete(NULL);
        return;
    }

    // Place the cursor at the end of the WAV file
    fseek(f, 0, SEEK_END);  

    // Find the size of the WAV file
    uint32_t fileSize = ftell(f);

    // Calculate the size of the audio data (excluding the WAV header)
    uint32_t dataSize = fileSize - 44;

    // Place the cursor at the beginning of the WAV file
    fseek(f, 0, SEEK_SET); 

    // Regenerate the wav header with the correct size
    generate_wav_header(wav_header_fmt, dataSize, CONFIG_SAMPLE_RATE);

    // Rewrite the header to the WAV file with the correct timing
    fwrite(wav_header_fmt, 1, WAVE_HEADER_SIZE, f);

    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");
    // Deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);

    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle)); // Stop the I2S channel
    ESP_ERROR_CHECK(i2s_del_channel(rx_handle)); // Delete the I2S driver

    vTaskDelete(NULL);
}

void init_i2s_driver(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_SLAVE);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    /* Setting the configurations, the slot configuration and clock configuration can be generated by the macros
    * These two helper macros is defined in 'i2s_std.h' which can only be used in STD mode.
    * They can help to specify the slot and clock configurations for initialization or updating */
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = CONFIG_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(CONFIG_BIT_SAMPLE, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_I2S_BCK,
            .ws = CONFIG_I2S_LRCK,
            .dout = I2S_GPIO_UNUSED,
            .din = CONFIG_I2S_DATA,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    /* Initialize the rx channel */
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
}


void app_main(void)
{
    printf("PCM microphone recording start\n--------------------------------------\n");
    //Initialisation of the interrupt
    init_interrupt();
}