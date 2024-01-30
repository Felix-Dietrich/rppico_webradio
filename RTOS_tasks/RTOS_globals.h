#ifndef RTOS_GLOBALS_H
#define RTOS_GLOBALS_H
/*
* RTOS_globals.h
* Created: 27.01.2023 Felix Dietrich
* Description: Contains global definitions and variables
*/
#include "FreeRTOS.h"
#include "queue.h"

// Definitions
#define TCP_MSS 1460

#define MAX_MP3_FRAME_SIZE 1441
#define BUF_LEN (TCP_MSS + MAX_MP3_FRAME_SIZE)

#define LADESCHLUSS_V 4.0
#define ENTLADESCHLUSS_V 3.2

#define FILTERSIZE 128
#define SPECTRUMSIZE 10


typedef struct 
{
    int size;
    char data[TCP_MSS];
}buffer_t;

typedef struct 
{
    int size;
    int samplerate;
    int16_t data[1250];
}buffer_pcm_t;

// Variables
extern QueueHandle_t processed_audio_queue;
extern buffer_t http_buffer;
extern QueueHandle_t compressed_audio_queue;
extern QueueHandle_t raw_audio_queue;

extern float volume;
extern int current_stream;
extern bool stop_stream;
extern bool is_streaming;
extern bool is_connected;
extern bool is_connected_ap;
extern int battery_percent;

// Prototypes

#endif