#ifndef RTOS_GLOBALS_H
#define RTOS_GLOBALS_H
/*
* RTOS_globals.h
* Created: 27.01.2023 Felix Dietrich
* Description: Contains global definitions and variables
*/
#define TCP_MSS 1460

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

#endif