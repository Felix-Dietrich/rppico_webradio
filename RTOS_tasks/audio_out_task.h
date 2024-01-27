#ifndef AUDIO_OUT_TASK_H
#define AUDIO_OUT_TASK_H
/*
* AUDIO_OUT_TASK
* Created: 27.01.2023 Felix Dietrich
* Description: Fetches the PCM audio data from the queue and pushes them to the I2S audio amplifier 
*/
#include "FreeRTOS.h"
#include "queue.h"

/*Definitions*/
extern QueueHandle_t processed_audio_queue;

/*Variables*/

/*Prototypes*/
void audio_out_task(__unused void *params);














#endif