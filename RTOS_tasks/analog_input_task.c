/*
* AUDIO_OUT_TASK
* Created: 27.01.2023 Manfred Dietrich
* Description: ADC for potentiometers (volume, station) and battery 
*/


//#include "pico/stdlib.h"
#include <stdlib.h>

#include "analog_input_task.h"
#include "RTOS_globals.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"

void analog_in_task(__unused void *params)
{
    float battery_v = -1;
    int i = 0;
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_select_input(0);
    
    while(true)
    {
        gpio_put(11,1);
        adc_select_input(0);
        uint16_t ADCresult0 = adc_read();
        adc_select_input(1);
        uint16_t ADCresult1 = adc_read();
        
        if(i>=10)   //akkustand muss nicht so oft aktualisiert werden
        {
            adc_select_input(2);
            uint16_t ADCresult2 = adc_read();

            if(battery_v <0) //beim ersten Mal
            {
                battery_v = (ADCresult2*3.3*2.0/4096.0);
            }
            battery_v = battery_v*0.95 + (ADCresult2*3.3*2.0/4096.0)*0.05;
            battery_percent = (battery_v-ENTLADESCHLUSS_V)*100.0/(LADESCHLUSS_V-ENTLADESCHLUSS_V);
            if(battery_percent<0)
            {
                battery_percent = 0;
            }
            if(battery_percent>100)
            {
                battery_percent = 100;
            }
            i=0;
        }
        i++;


        volume = volume*0.95+(float)ADCresult0/4096*0.05;

        const int steps= 11;
        const int ADCResolution = 4096;
        const float overlap = 0.25;
        const int divider = ADCResolution/(steps-1);

        if(abs((current_stream*divider)-ADCresult1)>(divider/2+divider*overlap))
        {
            //printf("last stream: %d\n",current_stream);
            //printf("adc-Result: %d\n",ADCresult1);
            current_stream = (ADCresult1+divider/2)/divider;
            //printf("ADC selection: %d\n\n", current_stream);
        }
        gpio_put(11,0);
        vTaskDelay(20);
    }

}

