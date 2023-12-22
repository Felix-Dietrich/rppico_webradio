#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <flash_utils/flash_utils.h>

const char* cgi_led_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    if(strcmp(pcParam[0], "led") == 0)
    {
        if(strcmp(pcValue[0], "0") == 0)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
        if(strcmp(pcValue[0], "1") == 0)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        }
    }

    return "/index.shtml";
}

const char* cgi_form_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    flash_content_t flash_content_w;
    for(int i = 0; i < iNumParams; i++)
    {
        if(strcmp(pcParam[i], "SSID") == 0)
        {
            printf("SSID: %s\n", pcValue[i]);
            strcpy(flash_content_w.ssid,pcValue[i]);
        }
        if(strcmp(pcParam[i], "password") == 0)
        {
            printf("password: %s\n", pcValue[i]);
            strcpy(flash_content_w.password,pcValue[i]);
        }
    }
    flash_write(&flash_content_w);
    
   

    return "/index.shtml";
}



static const tCGI cgi_handlers[] = 
{
     {
        "/led.cgi", cgi_led_handler
     },
     {
        "/submit-form", cgi_form_handler
     },
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers,2);

}