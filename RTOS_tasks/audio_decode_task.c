/*
* AUDIO_DECODE_TASK
* Created: 31.01.2023 Manfred Dietrich
* Description: MP3 to PCM decoding
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <picomp3lib/mp3dec.h>
#include <hardware/gpio.h>
#include <hardware/watchdog.h>
#include <FreeRTOS.h>
#include <queue.h>

#include "RTOS_globals.h"


void audio_decode_task(__unused void *params)
{
    static unsigned char buffer_mp3[BUF_LEN];
    static buffer_pcm_t buffer_pcm;
    static int buffer_mp3_pos = 0;
    //printf("hallo von core%d\n",get_core_num());
    HMP3Decoder decoder = MP3InitDecoder();
    
    while (true)
    {
        //puts("mp3_task");
        static buffer_t receive_buffer;
        static int16_t wav_buffer[2500];
        xQueueReceive(compressed_audio_queue,&receive_buffer,portMAX_DELAY);
        //printf("bytes empfangen audio out task: %d\n", receive_buffer.size);
        memcpy(buffer_mp3+buffer_mp3_pos,receive_buffer.data,receive_buffer.size);
        buffer_mp3_pos+=receive_buffer.size;
        //printf("total data: %d\n", buffer_mp3_pos);
        //puts("decode\n");
        
        static int err;
        err = -1;
        // Find the start of the mp3 frame.
        static int syncoffset; 
        syncoffset = MP3FindSyncWord(buffer_mp3,buffer_mp3_pos);
        if(syncoffset < 0)
        {
            // No sync word found, discard the buffer.
            buffer_mp3_pos = 0;
            puts("no sync found\n");
        }
        if(syncoffset >0)
        {
            //printf("syncoffset:%d\n",syncoffset);
        }
        while(buffer_mp3_pos>0)
        {
            //puts("mp3_decode");
          //  printf("syncoffset: %d\n", syncoffset);
            static unsigned char* inbuf;
            inbuf = buffer_mp3 + syncoffset;
            static int bytesleft; 
            bytesleft = buffer_mp3_pos;
            gpio_put(16,1);
            err= MP3Decode(decoder,&inbuf,&bytesleft, wav_buffer,0);
            gpio_put(16,0);
            if(err == 0)
            {
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
                buffer_mp3_pos -= (syncoffset+info.size);
                memmove(buffer_mp3,buffer_mp3+syncoffset+info.size,buffer_mp3_pos);
                syncoffset = 0;
               // printf("decoded bitrate: %d\n", info.bitrate);
                //printf("decoded bits per Sample: %d\n", info.bitsPerSample);
                //printf("decoded layer: %d\n", info.layer);
                //printf("decoded channels: %d\n", info.nChans);
                //printf("decoded samples: %d\n", info.outputSamps);
                //printf("decoded samplerate: %d\n", info.samprate);
                //printf("decoded size: %d\n", info.size);
                //printf("decoded version: %d\n", info.version);
                //printf("remaining data: %d\n", buffer_mp3_pos);
                
                if(info.outputSamps == 0) //no samples decoded
                {
                    buffer_mp3_pos = 0;
                    puts("no decode");
                    break;
                }
                buffer_pcm.samplerate = info.samprate;
                static int Samprate = 0;
                
                //puts("audio out\n");
                for(int i = 0; i < (info.outputSamps-1); i+=2)
                {
                    buffer_pcm.data[i/2] = ((wav_buffer[i]+wav_buffer[i+1])>>1);
                }
                buffer_pcm.size = info.outputSamps/2;
                xQueueSend(raw_audio_queue,&buffer_pcm,portMAX_DELAY);
            }
            else if(err == -1) //data underflow
            {
               // puts("underflow");
                break;
            }
            else //other error
            {
                buffer_mp3_pos = 0;
               // puts("other error");
                break;
            }
        }
    }  
}

