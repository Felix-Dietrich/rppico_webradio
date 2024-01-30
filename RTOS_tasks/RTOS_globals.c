/*
* RTOS_globals.h
* Created: 27.01.2023 Felix Dietrich
* Description: Contains global definitions and variables
*/

#include "RTOS_globals.h"

QueueHandle_t processed_audio_queue;

buffer_t http_buffer;
QueueHandle_t compressed_audio_queue;
QueueHandle_t raw_audio_queue;


float volume = 0;
int current_stream = 0;
bool stop_stream = false;
bool is_streaming = false;
bool is_connected = false;
bool is_connected_ap = false;


