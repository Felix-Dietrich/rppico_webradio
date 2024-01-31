#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <picomp3lib/mp3dec.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "lwip/apps/httpd.h"
#include "lwipopts.h"
#include <flash_utils/flash_utils.h>
#include "lwip/apps/mdns.h"
#include <dhcpserver/dhcpserver.h>
#include <dnsserver/dnsserver.h>
#include "math.h"

#include "RTOS_tasks/RTOS_globals.h"
#include "RTOS_tasks/main_task.h"
#include "RTOS_tasks/httpd_server_task.h"
#include "RTOS_tasks/audio_out_task.h"
#include "RTOS_tasks/audio_decode_task.h"
#include "RTOS_tasks/audio_process_task.h"
#include "RTOS_tasks/analog_input_task.h"



#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

void vLaunch( void);

//void http_request(void);

void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume);
void spectrum_to_filter(const float spectrum[6], int16_t filter[256]);

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



void wifi_task(__unused void *params)
{
    const char* ssid = flash_content_r->ssid;
    const char* pass = flash_content_r->password;
     
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWITZERLAND))
    {
        while (true)
        {
            //puts("init failed");
            sleep_ms(1000);
        }
    }
    //puts("initialised");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,true);
    cyw43_arch_enable_sta_mode();

    cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
    int last_status = INT32_MIN;
    while (true)
    {   
        int new_status = cyw43_tcpip_link_status(&cyw43_state,CYW43_ITF_STA);
        if(last_status != new_status)
        {
            last_status=new_status;
            is_connected = false;
            switch(new_status)
            {
                case CYW43_LINK_BADAUTH:
                printf("badauth\n");
                is_connected = false;
                break;
                case CYW43_LINK_DOWN:
                printf("link down\n");
                is_connected = false;
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_FAIL:
                printf("link fail\n");
                is_connected = false;
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_JOIN:
                printf("link join\n");
                is_connected = false;
                break;
                case CYW43_LINK_NOIP:
                printf("link no ip\n");
                is_connected = false;
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_NONET:
                printf("link no net\n");
                is_connected = false;
                break;
                case CYW43_LINK_UP:
                printf("link up: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
                is_connected = true;
                
                break;

            }

            if((new_status == CYW43_LINK_NONET) || (new_status == CYW43_LINK_BADAUTH))
            {
                    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
                    cyw43_arch_enable_ap_mode("deDietrich",NULL,CYW43_AUTH_WPA2_MIXED_PSK);

                    ip4_addr_t mask;
                    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
                    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

                    // Start the dhcp server
                    dhcp_server_t dhcp_server;
                    dhcp_server_init(&dhcp_server, &state->gw, &mask);

                    // Start the dns server
                    dns_server_t dns_server;
                    dns_server_init(&dns_server, &state->gw);
                    is_connected_ap = true;
                    while(true)
                    {
                        vTaskDelay(100);
                    }

                    /*if (!tcp_server_open(state)) 
                    {
                        DEBUG_printf("failed to open server\n");
                        return 1;
                    }*/
            }


        }
        vTaskDelay(100);
    }
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



