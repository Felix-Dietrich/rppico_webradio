#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <flash_utils/flash_utils.h>


const char* ssi_tags[] = {"batt","temp", "led", "ssid", "sender1", "sender2", "sender3", "sender4", "sender5", "sender6", "sender7", "sender8", "sender9", "sender10", "sender11"};

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
        case 4: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[0]);
        }
        break;
        case 5: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[1]);
        }
        break;
        case 6: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[2]);
        }
        break;
        case 7: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[3]);
        }
        break;
        case 8: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[4]);
        }
        break;
        case 9: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[5]);
        }
        break;
        case 10: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[6]);
        }
        break;
        case 11: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[7]);
        }
        break;
        case 12: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[8]);
        }
        break;
        case 13: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[9]);
        }
        break;
        case 14: 
        {
            printed = snprintf(pcInsert,iInsertLen, "%s", flash_content_r->sender[10]);
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