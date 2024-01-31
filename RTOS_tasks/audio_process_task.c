/*
* AUDIO_PROCESS_TASK
* Created: 31.01.2023 Manfred Dietrich
* Description: Processing and equalizing PCM data 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <picomp3lib/mp3dec.h>
#include <hardware/gpio.h>
#include <flash_utils/flash_utils.h>
#include <math.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <hardware/watchdog.h>

#include "RTOS_globals.h"

// Prototypes
static void spectrum_to_filter(const float spectrum[SPECTRUMSIZE], int16_t filter[FILTERSIZE]);
static void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume);


void audio_process_task(__unused void *params)
{
    static buffer_pcm_t buffer_raw;
    
    static buffer_pcm_t buffer_processed;
    static bool isPlaying = false;
    static bool wasPlaying = false;
    while(true)
    {
     /* Freie Plätze der Queue anzeigen
        int raw= uxQueueSpacesAvailable(raw_audio_queue);
        int compressed = uxQueueSpacesAvailable(compressed_audio_queue);
        static int last_compressed;
        static int last_raw;

        if(last_raw != raw)
        {
            last_raw = raw;
            printf("raw: %d\n",raw);
        }
        if(last_compressed != compressed)
        {
            last_compressed=compressed;
            printf("compressed %d\n", compressed);
        }
        */
        if(!isPlaying || xQueueReceive(raw_audio_queue,&buffer_raw,40)==pdFALSE)
        {
            buffer_raw.size=1152;
            buffer_raw.samplerate=44100;
            if (wasPlaying)
            {
                volume=0;
                wasPlaying=false;
            }
            
            for(int i = 0; i < buffer_raw.size; i++)
            {
                buffer_raw.data[i] = (rand()%5000);
            }
            if((uxQueueSpacesAvailable(raw_audio_queue)==0) && (uxQueueSpacesAvailable(compressed_audio_queue)==0)) //warten, bis gesamte quque voll bevor playback beginnt. 
            {
                isPlaying = true;
            }
            else
            {
                isPlaying = false;
            }
        }
        else
        {
            if(!wasPlaying)
            {
                volume=0;
                wasPlaying=true;
            }
        }
       
        equalizer(&buffer_raw,&buffer_processed,volume);
        xQueueSend(processed_audio_queue,&buffer_processed,portMAX_DELAY);
        
    }
}


static void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume)
{
    static int16_t filter[FILTERSIZE];
    static int16_t in_last[FILTERSIZE];
    int32_t processedData;
    int32_t volume2 = (1<<15)*volume;
    bool update_filter=false;
    static float last_spectrum[SPECTRUMSIZE] = {-10000};

    for(int i = 0; i<SPECTRUMSIZE; i++)
    {
        if(last_spectrum[i] != flash_content_work.eq[i] )
        {
            last_spectrum[i]=flash_content_work.eq[i];
            update_filter = true;
        }
    }
/*
    for(int i = 0; i< FILTERSIZE; i++)
    {
        filter[i] = (1<<14)/FILTERSIZE;
        //filter[i]=0;
    }
    filter[FILTERSIZE/2] = 1<<8;
*/

    if(update_filter)
    {
        spectrum_to_filter(flash_content_work.eq,filter);
        update_filter = false;
    }

    out->samplerate = in->samplerate;
    out->size = in->size;

    //Faltung
    for(int i = 0; i < in->size; i++)
    {
        processedData = 0;
        for (size_t y = 0; y < FILTERSIZE; y++)
        {
            if(i+y<FILTERSIZE)
            {
                processedData += in_last[i+y]*filter[y];
            }
            else
            {
                processedData += in->data[i+y-FILTERSIZE]*filter[y];
            }
            
        }
        out->data[i] = ((processedData>>16)*volume2)>>14; //ganzzahl multiplikation statt float ist deutlich schneller. eins zu wenig zurückschieben für lautere meaximale lautstärke
    }

    //copy last (Filtersize) samples
    for(int i = 0; i< FILTERSIZE; i++)
    {
        in_last[i] = in->data[in->size-FILTERSIZE+i];
    }

}


static void spectrum_to_filter(const float spectrum[SPECTRUMSIZE], int16_t filter[FILTERSIZE])
{
    //clculate factor from dB
    float spectrum_factor[SPECTRUMSIZE];
    float filter_f[FILTERSIZE/2]={0};
    for(int i = 0; i < SPECTRUMSIZE; i++)
    {
        spectrum_factor[i] = powf(10,spectrum[i]/10);
    }

    
    float spectrum_factor_all[FILTERSIZE/2]; //first element lowest frequency
    
    for(int i = 0; i < 6; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[i]; 
    }
    for(int i = 6; i < 8; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[6]; 
    }
    for(int i = 8; i < 16; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[7]; 
    }
    for(int i = 16; i < 32; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[8]; 
    }
    for(int i = 32; i < 64; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[9]; 
    }

    /*
    //interpolate spectrum #todo actual interpolating instead of copying
    int offset = 0;
    for(int x = 0; x<SPECTRUMSIZE; x++)
    {
        for(int i = 0; i<(1<<x); i++)
        {
            spectrum_factor_all[offset + i]=spectrum_factor[x];
        }
        offset+= 1<<x;
    }
     */
    spectrum_factor_all[0] /= 2;
    spectrum_factor_all[(FILTERSIZE/2)-1] /= 2;      //weiss ich auch nicht wieso, aber so stimmt das Resultat mit numpy.fft.irfft überein
  

    //discrete fourrier transform
    for(int i = 0; i<FILTERSIZE/2; i++)
    {
        for(int y = 0; y<FILTERSIZE/2; y++)
        {
            filter_f[i] += 1.0/(FILTERSIZE/2-1)*cosf(M_PI/(FILTERSIZE/2-1)*i*y)*spectrum_factor_all[y];
        }
    }

    //kopieren fftshift und in fixed point wandeln
    for(int i= 0; i < FILTERSIZE/2; i++)
    {
        filter[FILTERSIZE/2+i-1] = (1<<13)*filter_f[i];
        filter[i] = (1<<13)*filter_f[(FILTERSIZE/2)-1-i];
    }

}

