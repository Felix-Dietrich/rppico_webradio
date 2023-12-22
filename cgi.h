#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <flash_utils/flash_utils.h>

void depercent(char* string);

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

const char* cgi_credentials_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(int i = 0; i < iNumParams; i++)
    {
        depercent(pcValue[i]);
        if(strcmp(pcParam[i], "SSID") == 0)
        {
            //printf("SSID: %s\n", pcValue[i]);
            strcpy(flash_content_work.ssid,pcValue[i]);
        }
        if(strcmp(pcParam[i], "password") == 0)
        {
            //printf("password: %s\n", pcValue[i]);
            strcpy(flash_content_work.password,pcValue[i]);
        }
        if(strcmp(pcParam[i], "speichern") == 0)
        {
            flash_write(&flash_content_work);
            return "/index.shtml";
        }
    }
    return "/ok.txt";
    
}

const char* cgi_sender_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(int i = 0; i < iNumParams; i++)
    {
        printf("%s=%s\n",pcParam[i],pcValue[i]);
        depercent(pcValue[i]);
        if(strcmp(pcParam[i], "sender1") == 0)
        {
            strcpy(flash_content_work.sender[0],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender2") == 0)
        {
            strcpy(flash_content_work.sender[1],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender3") == 0)
        {
            strcpy(flash_content_work.sender[2],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender4") == 0)
        {
            strcpy(flash_content_work.sender[3],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender5") == 0)
        {
            strcpy(flash_content_work.sender[4],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender6") == 0)
        {
            strcpy(flash_content_work.sender[5],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender7") == 0)
        {
            strcpy(flash_content_work.sender[6],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender8") == 0)
        {
            strcpy(flash_content_work.sender[7],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender9") == 0)
        {
            strcpy(flash_content_work.sender[8],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender10") == 0)
        {
            strcpy(flash_content_work.sender[9],pcValue[i]);
        }
        if(strcmp(pcParam[i], "sender11") == 0)
        {
            strcpy(flash_content_work.sender[10],pcValue[i]);
        }
        if(strcmp(pcParam[i], "speichern") == 0)
        {
            flash_write(&flash_content_work);
            return "/index.shtml";
        }
    }
    return "/ok.txt";
}


const char* cgi_equalizer_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(int i = 0; i < iNumParams; i++)
    {
        //printf("%s=%s\n",pcParam[i],pcValue[i]);
        if(strcmp(pcParam[i], "eq1") == 0)
        {
            flash_content_work.eq[0] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq2") == 0)
        {
            flash_content_work.eq[1] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq3") == 0)
        {
            flash_content_work.eq[2] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq4") == 0)
        {
            flash_content_work.eq[3] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq5") == 0)
        {
            flash_content_work.eq[4] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq6") == 0)
        {
            flash_content_work.eq[5] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq7") == 0)
        {
            flash_content_work.eq[6] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq8") == 0)
        {
            flash_content_work.eq[7] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq9") == 0)
        {
            flash_content_work.eq[8] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "eq10") == 0)
        {
            flash_content_work.eq[9] = (float)atof(pcValue[i]);
        }
        if(strcmp(pcParam[i], "speichern") == 0)
        {
            flash_write(&flash_content_work);
            return "/index.shtml";
        }
    }
    return "/ok.txt";
}

static const tCGI cgi_handlers[] = 
{
     {
        "/led.cgi", cgi_led_handler
     },
     {
        "/credentials", cgi_credentials_handler
     },
     {
        "/sender", cgi_sender_handler
     },
     {
        "/equalizer", cgi_equalizer_handler
     },
};

void cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers,LWIP_ARRAYSIZE(cgi_handlers));

}

/*
* prozentschreibweise der URL-Codierung in ascii umwandeln
* This function processes a string by replacing occurrences of "%xx" substrings with
* the corresponding ASCII characters, where "xx" represents a two-digit hexadecimal
* number.
*/
void depercent(char* string)
{
    char* endpos = string+strlen(string);
    while(string < endpos) 
    {
        if(*string == '+')
        {
            *string = ' ';
        }
        if(*string == '%')
        {
            if(string[1] >= 'a')
            {
                string[1] -= 'a'-10;
            }
            else if(string[1] >= 'A')
            {
                string[1] -= 'A'-10;
            }
            else if(string[1] >= '0')
            {
                string[1] -= '0';
            }
            
            if(string[2] >= 'a')
            {
                string[2] -= 'a'-10;
            }
            else if(string[2] >= 'A')
            {
                string[2] -= 'A'-10;
            }
            else if(string[2] >= '0')
            {
                string[2] -= '0';
            }
            printf("%x,%x\n",string[1],string[2]);
            *string = (0x10 * string[1] + string[2]);
            memmove(string+1, string+3, endpos-(string+2));
        }
        string++;
    }
}
