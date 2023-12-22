#ifndef AUDIO_I2S_API
#define AUDIO_I2S_API

#include <stdio.h>
#include "pico/stdlib.h"


#define PICO_AUDIO_I2S_DATA_PIN 2
#define PICO_AUDIO_I2S_CLOCK_PIN_BASE 0
#define SAMPLE_FREQ 48000
//#define SAMPLE_FREQ 48000/2

void audio_i2s_api_init(void);
uint8_t audio_i2s_api_write(int16_t *buffer, int sample_count, int sample_freq);

void update_pio_frequency(uint32_t sample_freq);
#endif