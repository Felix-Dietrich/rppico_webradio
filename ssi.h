#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"

const char* ssi_tags[] = {"batt","temp", "led"};

int battery_percent;

u16_t ssi_handler(int index, char *pcInsert, int iInsertLen)
{
    u16_t printed;
    switch(index)
    {
        case 0:
        {
            printed = snprintf(pcInsert,iInsertLen, "%d", battery_percent);
        }
        break;
        case 1:
        {
            const float temp = 2;
            printed = snprintf(pcInsert,iInsertLen, "%f", temp);
        }
        break;
        case 2:
        {
            const float LED = 3;
            printed = snprintf(pcInsert,iInsertLen, "%f", LED);
        }
        break;
        default:
            printed = 0;
        break;
    }
    return printed;
}

void ssi_init()
{
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}