#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <hardware/clocks.h>
//#include <semphr.h>
#include <hardware/gpio.h>
#include <hardware/vreg.h>
#include <hardware/watchdog.h>
#include <flash_utils/flash_utils.h>

#include "RTOS_tasks/RTOS_globals.h"
#include "RTOS_tasks/main_task.h"
#include "RTOS_tasks/httpd_server_task.h"
#include "RTOS_tasks/audio_out_task.h"
#include "RTOS_tasks/audio_decode_task.h"
#include "RTOS_tasks/audio_process_task.h"
#include "RTOS_tasks/analog_input_task.h"
#include "RTOS_tasks/wifi_task.h"



#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

// Prototypes
void vLaunch(void);


int main()
{
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    set_sys_clock_khz(250000,true);
    stdio_init_all();
     const char *rtos_name;
#if ( portSUPPORT_SMP == 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( portSUPPORT_SMP == 1 ) && ( configNUM_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREERTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}

/*
void core1_task(__unused void *params)
{

}

void core2_task(__unused void *params)
{

}
*/

void vLaunch( void) 
{ 
    //puts("task handle");
    TaskHandle_t main_task_handle, audio_out_task_handle, audio_decode_task_handle, audio_process_task_handle, analog_in_task_handle, wifi_task_handle, http_server_task_handle, core1_task_handle, core2_task_handle;
    //puts("queues");
    compressed_audio_queue = xQueueCreate(16,sizeof(http_buffer));
    raw_audio_queue = xQueueCreate(4,sizeof(buffer_pcm_t));
    processed_audio_queue = xQueueCreate(4,sizeof(buffer_pcm_t));
    //puts("Tasks");
    xTaskCreate(main_task, "Main Task", configMINIMAL_STACK_SIZE, NULL, 2, &main_task_handle);
    xTaskCreate(audio_out_task,"Audio out Task",configMINIMAL_STACK_SIZE,NULL,2,&audio_out_task_handle);
    xTaskCreate(audio_decode_task,"Audio decode Task",configMINIMAL_STACK_SIZE,NULL,2,&audio_decode_task_handle);
    xTaskCreate(audio_process_task,"Audio process Task",configMINIMAL_STACK_SIZE,NULL,2,&audio_process_task_handle);
    xTaskCreate(analog_in_task,"Analog in Task",configMINIMAL_STACK_SIZE,NULL,2,&analog_in_task_handle);
    xTaskCreate(wifi_task,"Wifi Task",configMINIMAL_STACK_SIZE,NULL,2,&wifi_task_handle);
    xTaskCreate(http_server_task,"HTTP Server Task",configMINIMAL_STACK_SIZE,NULL,2,&http_server_task_handle);
 //   xTaskCreate(core1_task,"Core1 Task",configMINIMAL_STACK_SIZE,NULL,2,&core1_task_handle);
 //   xTaskCreate(core2_task,"Core2 Task",configMINIMAL_STACK_SIZE,NULL,2,&core2_task_handle);
    //puts("start");

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(main_task_handle, 1);
#endif
//    vTaskCoreAffinitySet(core1_task_handle, 1<<0);
//    vTaskCoreAffinitySet(core2_task_handle, 1<<1);


    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}



