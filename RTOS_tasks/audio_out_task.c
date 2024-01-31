/*
* AUDIO_OUT_TASK
* Created: 27.01.2023 Felix Dietrich
* Description: Fetches the PCM audio data from the queue and pushes them to the I2S audio amplifier 
*/

#include <audioI2SAPI/audio_i2s_api.h>
#include <hardware/watchdog.h>
#include <FreeRTOS.h>
#include <queue.h>

#include "audio_out_task.h"
#include "RTOS_globals.h"


void audio_out_task(__unused void *params)
{
    //printf("hallo von core%d\n",get_core_num());
    audio_i2s_api_init();
    uint8_t status = 0;
    static buffer_pcm_t buffer_wav;
    buffer_wav.size = 0;
    buffer_wav.samplerate = 0;

    while (true)
    {
        if(status == 0)
        {
            xQueueReceive(processed_audio_queue,&buffer_wav,portMAX_DELAY);
        }
        
        else
        {
            vTaskDelay(1);
            //taskYIELD();
        }
        if((buffer_wav.samplerate>10000) && (buffer_wav.samplerate<100000))
        {
            status = audio_i2s_api_write((int16_t*)buffer_wav.data,buffer_wav.size, buffer_wav.samplerate);
            watchdog_update();
        }
        else
        {
            status = 0;
        }
        
    }    
}

