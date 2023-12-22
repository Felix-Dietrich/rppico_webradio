#ifndef FLASH_UTILS_H
#define FLASH_UTILS_H
#include <stdint.h>


typedef struct 
{
    char ssid[33];
    char password[33];
    char sender[11][256];
    float eq[10];
}flash_content_t;

extern uint32_t ADDR_PERSISTENT[];
static const flash_content_t* flash_content_r = (const flash_content_t*) ADDR_PERSISTENT;

extern flash_content_t flash_content_work; //work copy of flash

void flash_write(flash_content_t* flash_content_w);

void flash_utils_init();

#endif