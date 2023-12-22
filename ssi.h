#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <flash_utils/flash_utils.h>


const char* ssi_tags[] = {"batt","temp", "led", "ssid", "sender1", "sender2", "sender3", "sender4", "sender5", "sender6", "sender7", "sender8", "sender9", "sender10", "sender11", "eq1", "eq2", "eq3", "eq4", "eq5", "eq6", "eq7", "eq8", "eq9", "eq10"};

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
        case 3: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->ssid);
        }
        break;
        default:
            printed = 0;
        break;
    }

    if(index >=4 && index <=14)
    {
        printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[index-4]);
    }
    if(index >= 15 && index <=25)
    {
        printed = snprintf(pcInsert,iInsertLen, "%.1f", flash_content_r->eq[index-15]);
    }
    return printed;
}

void ssi_init()
{
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}