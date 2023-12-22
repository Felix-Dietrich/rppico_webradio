
#include "audio_i2s_api.h"
#include "pico/audio_i2s.h"
#include "hardware/gpio.h"

#define SAMPLES_PER_BUFFER 3000


typedef struct audio_buffer_pool audio_buffer_pool_t;
audio_buffer_pool_t* ap;

struct audio_buffer_pool *init_audio() 
{

    static audio_format_t audio_format = {
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .sample_freq = SAMPLE_FREQ,
            .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);

    return producer_pool;
}

void audio_i2s_api_init(void)
{
    ap = init_audio();
    gpio_init(23);
    gpio_set_dir(23,GPIO_OUT);
    // Set the PS pin to high to force the regulator into PWM mode
    gpio_put(23,1);         
}


void audio_i2s_api_write(int16_t *buffer, int sample_count)
{
    audio_buffer_t *audio_buffer = take_audio_buffer(ap, true);
    int16_t *samples = (int16_t *) audio_buffer->buffer->bytes;
    int pos=0;
    for (uint i = 0; i < sample_count/2; i++) 
    {
        samples[i] = buffer[i*2]>>5;
        pos ++;
        if (pos >= audio_buffer->max_sample_count) 
        {
            pos = 0;
        }
    }
    audio_buffer->sample_count = sample_count/2;
    give_audio_buffer(ap, audio_buffer);
}
