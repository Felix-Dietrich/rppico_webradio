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
    flash_content_w = *flash_content_r;
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

const char* cgi_sender_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    flash_content_t flash_content_w;
    flash_content_w = *flash_content_r;
    for(int i = 0; i < iNumParams; i++)
    {
        printf("%s=%s\n",pcParam[i],pcValue[i]);
        if(strcmp(pcParam[i], "sender1") == 0)
        {
            strcpy(flash_content_w.sender[0],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender2") == 0)
        {
            strcpy(flash_content_w.sender[1],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender3") == 0)
        {
            strcpy(flash_content_w.sender[2],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender4") == 0)
        {
            strcpy(flash_content_w.sender[3],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender5") == 0)
        {
            strcpy(flash_content_w.sender[4],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender6") == 0)
        {
            strcpy(flash_content_w.sender[5],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender7") == 0)
        {
            strcpy(flash_content_w.sender[6],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender8") == 0)
        {
            strcpy(flash_content_w.sender[7],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender9") == 0)
        {
            strcpy(flash_content_w.sender[8],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender10") == 0)
        {
            strcpy(flash_content_w.sender[9],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender11") == 0)
        {
            strcpy(flash_content_w.sender[10],pcValue[i]);
        }
       
    }
    /*for(int i = 0; i < LWIP_ARRAYSIZE(flash_content_w.sender); i++)  //alle %2F durch / ersetzen
    {
        int offset = 0;
        for(int zeichen=0; (zeichen+offset) < 255; zeichen++)
        {
            if(flash_content_w.sender[i][zeichen] == '%')
            {
                offset+=2;
                flash_content_w.sender[i][zeichen] = '/';
                zeichen++;
            }
            flash_content_w.sender[i][zeichen] = flash_content_w.sender[i][zeichen+offset];
            if(flash_content_w.sender[i][zeichen] == 0 || flash_content_w.sender[i][zeichen] == 0xff)
            {
                break;
            }
        }
    }
    */
   for(int i = 0; i < LWIP_ARRAYSIZE(flash_content_w.sender); i++)  //alle %2F durch / ersetzen
    {
        char *pos;
        while ((pos = strstr(flash_content_w.sender[i], "%2F")) != NULL) {
            memmove(pos + 1, pos + 3, strlen(pos + 2));
            *pos = '/';
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
     {
        "/sender", cgi_sender_handler
     },
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers,LWIP_ARRAYSIZE(cgi_handlers));

}