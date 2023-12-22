#include "flash_utils.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"

flash_content_t flash_content_work;

static void writeFlash_safe(flash_content_t* flash_content_w)
{
    flash_range_erase(PICO_FLASH_SIZE_BYTES-FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
    flash_range_program(PICO_FLASH_SIZE_BYTES-FLASH_SECTOR_SIZE,(const uint8_t*) flash_content_w , ((sizeof(flash_content_t)/FLASH_PAGE_SIZE + 1)*FLASH_PAGE_SIZE));
}

void flash_write(flash_content_t* flash_content_w)
{
    flash_safe_execute((void (*)(void*))writeFlash_safe, flash_content_w, 200);
}


/* Flash auf ungültigen inhalt überprüfen und gegebenenfalls auf initialwerte zurücksetzen
* flash content_r in flash_contetnt_work kopieren
*/
void flash_utils_init()
{
    flash_content_work= *flash_content_r;
    bool update = false;
    if(flash_content_work.password[0] == 0xff)
    {
        strcpy(flash_content_work.password,"dedietrich");
        update = true;
    }
    if(flash_content_work.ssid[0] == 0xff)
    {
        strcpy(flash_content_work.ssid,"dedietrich");
        update = true;
    }
    if(flash_content_work.sender[0][0] == 0xff)
    {
        strcpy(flash_content_work.sender[0],"stream.srg-ssr.ch/m/rsp/mp3_128");
        update = true;
    }
    if(flash_content_work.sender[1][0] == 0xff)
    {
        strcpy(flash_content_work.sender[1], "stream.srg-ssr.ch/m/rsc_de/mp3_128");
        update = true;
    }
    if(flash_content_work.sender[2][0] == 0xff)
    {
        strcpy(flash_content_work.sender[2], "strm112.1.fm/country_mobile_mp3");
        update = true;
    }
    if(flash_content_work.sender[3][0] == 0xff)
    {
        strcpy(flash_content_work.sender[3], "energybern.ice.infomaniak.ch/energybern-high.mp3");
        update = true;
    }
    if(flash_content_work.sender[4][0] == 0xff)
    {
        strcpy(flash_content_work.sender[4], "stream.srg-ssr.ch/m/drs1/mp3_128");
        update = true;
    }
    if(flash_content_work.sender[5][0] == 0xff)
    {
        strcpy(flash_content_work.sender[5], "stream.srg-ssr.ch/m/rsj/mp3_128");
        update = true;
    }
    if(flash_content_work.sender[6][0] == 0xff)
    {
        strcpy(flash_content_work.sender[6], "streaming.swisstxt.ch/m/drs3/mp3_128");
        update = true;
    }
    if(flash_content_work.sender[7][0] == 0xff)
    {
        strcpy(flash_content_work.sender[7], "chmedia.streamabc.net/79-ffm-mp3-192-2470075");
        update = true;
    }
    if(flash_content_work.sender[8][0] == 0xff)
    {
        strcpy(flash_content_work.sender[8], "vintageradio.ice.infomaniak.ch/vintageradio-high.mp3");
        update = true;
    }
    if(flash_content_work.sender[9][0] == 0xff)
    {
        strcpy(flash_content_work.sender[9], "chmedia.streamabc.net/79-virginrockch-mp3-192-2872456");
        update = true;
    }
    if(flash_content_work.sender[10][0] == 0xff)
    {
        strcpy(flash_content_work.sender[10], "chmedia.streamabc.net/79-pilatus-mp3-192-4664468");
        update = true;
    }

    for(int i = 0; i<10; i++)
    {
        if(flash_content_work.eq[i]>40 || flash_content_work.eq[i]<-40)
        {
            flash_content_work.eq[i] = 0;
            update = true;
        }
    }

    printf("ssid: %s", flash_content_work.ssid);
    printf("sender1 %s", flash_content_work.ssid);
    if(update)
    {
        flash_write(&flash_content_work);
    }
}


