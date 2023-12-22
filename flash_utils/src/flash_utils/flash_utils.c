#include "flash_utils.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include "stdint.h"


static void writeFlash_safe(flash_content_t* flash_content_w)
{
    flash_range_erase(PICO_FLASH_SIZE_BYTES-FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
    flash_range_program(PICO_FLASH_SIZE_BYTES-FLASH_SECTOR_SIZE,(const uint8_t*) flash_content_w , ((sizeof(flash_content_t)/FLASH_PAGE_SIZE + 1)*FLASH_PAGE_SIZE));
}

void flash_write(flash_content_t* flash_content_w)
{
    flash_safe_execute((void (*)(void*))writeFlash_safe, flash_content_w, 200);
}


