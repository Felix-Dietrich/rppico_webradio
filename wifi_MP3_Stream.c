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


void audio_process_task(__unused void *params)
{
    static buffer_pcm_t buffer_raw;
    
    static buffer_pcm_t buffer_processed;
    static bool isPlaying = false;
    static bool wasPlaying = false;
    while(true)
    {
     /* Freie Plätze der Queue anzeigen
        int raw= uxQueueSpacesAvailable(raw_audio_queue);
        int compressed = uxQueueSpacesAvailable(compressed_audio_queue);
        static int last_compressed;
        static int last_raw;

        if(last_raw != raw)
        {
            last_raw = raw;
            printf("raw: %d\n",raw);
        }
        if(last_compressed != compressed)
        {
            last_compressed=compressed;
            printf("compressed %d\n", compressed);
        }
        */
        if(!isPlaying || xQueueReceive(raw_audio_queue,&buffer_raw,40)==pdFALSE)
        {
            buffer_raw.size=1152;
            buffer_raw.samplerate=44100;
            if (wasPlaying)
            {
                volume=0;
                wasPlaying=false;
            }
            
            for(int i = 0; i < buffer_raw.size; i++)
            {
                buffer_raw.data[i] = (rand()%5000);
            }
            if((uxQueueSpacesAvailable(raw_audio_queue)==0) && (uxQueueSpacesAvailable(compressed_audio_queue)==0)) //warten, bis gesamte quque voll bevor playback beginnt. 
            {
                isPlaying = true;
            }
            else
            {
                isPlaying = false;
            }
        }
        else
        {
            if(!wasPlaying)
            {
                volume=0;
                wasPlaying=true;
            }
        }
       
        equalizer(&buffer_raw,&buffer_processed,volume);
        xQueueSend(processed_audio_queue,&buffer_processed,portMAX_DELAY);
        
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



void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume)
{
    static int16_t filter[FILTERSIZE];
    static int16_t in_last[FILTERSIZE];
    int32_t processedData;
    int32_t volume2 = (1<<15)*volume;
    bool update_filter=false;
    static float last_spectrum[SPECTRUMSIZE] = {-10000};

    for(int i = 0; i<SPECTRUMSIZE; i++)
    {
        if(last_spectrum[i] != flash_content_work.eq[i] )
        {
            last_spectrum[i]=flash_content_work.eq[i];
            update_filter = true;
        }
    }
/*
    for(int i = 0; i< FILTERSIZE; i++)
    {
        filter[i] = (1<<14)/FILTERSIZE;
        //filter[i]=0;
    }
    filter[FILTERSIZE/2] = 1<<8;
*/

    if(update_filter)
    {
        spectrum_to_filter(flash_content_work.eq,filter);
        update_filter = false;
    }

    out->samplerate = in->samplerate;
    out->size = in->size;

    //Faltung
    for(int i = 0; i < in->size; i++)
    {
        processedData = 0;
        for (size_t y = 0; y < FILTERSIZE; y++)
        {
            if(i+y<FILTERSIZE)
            {
                processedData += in_last[i+y]*filter[y];
            }
            else
            {
                processedData += in->data[i+y-FILTERSIZE]*filter[y];
            }
            
        }
        out->data[i] = ((processedData>>16)*volume2)>>14; //ganzzahl multiplikation statt float ist deutlich schneller. eins zu wenig zurückschieben für lautere meaximale lautstärke
    }

    //copy last (Filtersize) samples
    for(int i = 0; i< FILTERSIZE; i++)
    {
        in_last[i] = in->data[in->size-FILTERSIZE+i];
    }

}

void spectrum_to_filter(const float spectrum[SPECTRUMSIZE], int16_t filter[FILTERSIZE])
{
    //clculate factor from dB
    float spectrum_factor[SPECTRUMSIZE];
    float filter_f[FILTERSIZE/2]={0};
    for(int i = 0; i < SPECTRUMSIZE; i++)
    {
        spectrum_factor[i] = powf(10,spectrum[i]/10);
    }

    
    float spectrum_factor_all[FILTERSIZE/2]; //first element lowest frequency
    
    for(int i = 0; i < 6; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[i]; 
    }
    for(int i = 6; i < 8; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[6]; 
    }
    for(int i = 8; i < 16; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[7]; 
    }
    for(int i = 16; i < 32; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[8]; 
    }
    for(int i = 32; i < 64; i++)
    {
        spectrum_factor_all[i] = spectrum_factor[9]; 
    }

    /*
    //interpolate spectrum #todo actual interpolating instead of copying
    int offset = 0;
    for(int x = 0; x<SPECTRUMSIZE; x++)
    {
        for(int i = 0; i<(1<<x); i++)
        {
            spectrum_factor_all[offset + i]=spectrum_factor[x];
        }
        offset+= 1<<x;
    }
     */
    spectrum_factor_all[0] /= 2;
    spectrum_factor_all[(FILTERSIZE/2)-1] /= 2;      //weiss ich auch nicht wieso, aber so stimmt das Resultat mit numpy.fft.irfft überein
  

    //discrete fourrier transform
    for(int i = 0; i<FILTERSIZE/2; i++)
    {
        for(int y = 0; y<FILTERSIZE/2; y++)
        {
            filter_f[i] += 1.0/(FILTERSIZE/2-1)*cosf(M_PI/(FILTERSIZE/2-1)*i*y)*spectrum_factor_all[y];
        }
    }

    //kopieren fftshift und in fixed point wandeln
    for(int i= 0; i < FILTERSIZE/2; i++)
    {
        filter[FILTERSIZE/2+i-1] = (1<<13)*filter_f[i];
        filter[i] = (1<<13)*filter_f[(FILTERSIZE/2)-1-i];
    }

}

