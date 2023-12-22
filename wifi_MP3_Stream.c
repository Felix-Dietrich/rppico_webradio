#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "FreeRTOS.h"
#include "task.h"
#include <audioI2SAPI/audio_i2s_api.h>
#include <picomp3lib/mp3dec.h>


#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )

#define MAX_MP3_FRAME_SIZE 1441
#define BUF_LEN (TCP_MSS + MAX_MP3_FRAME_SIZE)


char ssid[] = "***REMOVED***";
char pass[] = "***REMOVED***";

unsigned char buffer_mp3[BUF_LEN];
short buffer_wav[4000];
unsigned long tot_len=0;
int buffer_mp3_pos = 0;

TickType_t lastTick;
TickType_t startTime;

void main_task(__unused void *params);
void vLaunch( void);

void http_request(void);
void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);

HMP3Decoder decoder;

int main()
{
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


void main_task(__unused void *params)
{
    audio_i2s_api_init();
    decoder = MP3InitDecoder();
    
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWITZERLAND))
    {
        while (true)
        {
            puts("init failed");
            sleep_ms(1000);
        }
    }
    puts("initialised");

    cyw43_arch_enable_sta_mode();

    /*while (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 5000))
    {
        puts("failed to connect");
    }*/

    cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);

    
    while (true)
    {   
        static int last_status = INT32_MIN;
        int new_status = cyw43_tcpip_link_status(&cyw43_state,CYW43_ITF_STA);
        if(last_status != new_status)
        {
            last_status=new_status;
            switch(new_status)
            {
                case CYW43_LINK_BADAUTH:
                printf("badauth\n");
                break;
                case CYW43_LINK_DOWN:
                printf("link down\n");
                break;
                case CYW43_LINK_FAIL:
                printf("link fail\n");
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_JOIN:
                printf("link join\n");
                break;
                case CYW43_LINK_NOIP:
                printf("link no ip\n");
                break;
                case CYW43_LINK_NONET:
                printf("link no net\n");
                break;
                case CYW43_LINK_UP:
                printf("link up\n");
                http_request();
                break;

            }
        }
        if(new_status == CYW43_LINK_UP)
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        }    
        vTaskDelay(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(500);
    }
}

void vLaunch( void) 
{
    TaskHandle_t task;
    xTaskCreate(main_task, "TestMainThread", configMINIMAL_STACK_SIZE*2, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}


void http_request(void)
{
    puts("http_request");
    uint16_t port = 80;
    static httpc_connection_t settings;
    settings.result_fn = http_result;
    settings.headers_done_fn = http_header;
    startTime = xTaskGetTickCount();
    lastTick = startTime;
    //err_t err = httpc_get_file_dns("chmedia.streamabc.net", port, "/79-pilatus-mp3-192-4664468", &settings, http_body, NULL, NULL);
    err_t err = httpc_get_file_dns("stream.srg-ssr.ch", port, "/m/rsp/mp3_128", &settings, http_body, NULL, NULL);
    //err_t err = httpc_get_file_dns("stream.srg-ssr.ch", port, "/m/rsp/mp3_128", &settings, http_body, NULL, NULL);
    //err_t err = httpc_get_file_dns("example.com", port, "/index.html", &settings, http_body, NULL, NULL);
    //err_t err = httpc_get_file_dns("ipv4.download.thinkbroadband.com", port, "/10MB.zip", &settings, http_body, NULL, NULL);
    //err_t err = httpc_get_file_dns("samples-files.com", port, "/samples/Audio/wav/sample-file-1.wav", &settings, http_body, NULL, NULL);
    printf("http_request status %d \n", err);
}

void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    puts("\n\n\n\nhttp_result");
    puts("----------------------------------------");
    printf("local result=%d\n", httpc_result);
    printf("http result=%d\n", srv_res);
    static int request;
    request++;
    printf("request: %d\n",request);
    TickType_t elapsed = xTaskGetTickCount() - startTime;
    printf("total rec: %d KBytes in %d ms = %d KBytes/s",tot_len/1024,elapsed, tot_len/elapsed);

}

err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    puts("\n\n\n\nhttp_header");
    puts("----------------------------------------");
    printf("content_length=%d\n", content_len);
    printf("header length=%d\n", hdr_len);
    printf("buffer length:%d\n",hdr->tot_len);
    pbuf_copy_partial(hdr, buffer_mp3, hdr_len, 0);
    pbuf_free_header(hdr,hdr_len);
    printf("headers:\n");
    buffer_mp3[hdr_len] = 0;
    printf("%s", buffer_mp3);
    return ERR_OK;
}

err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    //puts("\n\n\n\nhttp_body");
    //puts("-----------------------------------------");
    if(p != NULL)
    {    
        if(p->tot_len > TCP_MSS)
        {
            puts("Err: too big to handle\n");
            tcp_recved(conn,p->tot_len);
            return ERR_OK;
        }
        
        //puts("buffer copy\n");
        buffer_mp3_pos += pbuf_copy_partial(p, buffer_mp3+buffer_mp3_pos, p->tot_len, 0);
        tot_len += p->tot_len;

        //printf("received Data: %d\n",p->tot_len);
        tcp_recved(conn,p->tot_len);
        //puts("buffer free\n");
        pbuf_free(p);
        

        //printf("total data: %d\n", buffer_mp3_pos);
        
        //puts("decode\n");
        int err = -1;
        // Find the start of the mp3 frame.
        int syncoffset = MP3FindSyncWord(buffer_mp3,buffer_mp3_pos);
        if(syncoffset < 0)
        {
            // No sync word found, discard the buffer.
            buffer_mp3_pos = 0;
        }
        while(buffer_mp3_pos>0)
        {
            
          //  printf("syncoffset: %d\n", syncoffset);
            unsigned char* inbuf = buffer_mp3 + syncoffset;
            int bytesleft = buffer_mp3_pos;
            err= MP3Decode(decoder,&inbuf,&bytesleft, buffer_wav,0);
            //printf("mp3dec_err: %d\n", err);
            
            if(err == 0)
            {
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
                buffer_mp3_pos -= (syncoffset+info.size);
                memmove(buffer_mp3,buffer_mp3+syncoffset+info.size,buffer_mp3_pos);
                syncoffset = 0;
               // printf("decoded bitrate: %d\n", info.bitrate);
                //printf("decoded bits per Sample: %d\n", info.bitsPerSample);
                //printf("decoded layer: %d\n", info.layer);
                //printf("decoded channels: %d\n", info.nChans);
                //printf("decoded samples: %d\n", info.outputSamps);
                //printf("decoded samplerate: %d\n", info.samprate);
                //printf("decoded size: %d\n", info.size);
                //printf("decoded version: %d\n", info.version);
                //printf("remaining data: %d\n", buffer_mp3_pos);

            
            

                if(info.outputSamps == 0) //no samples decoded
                {
                    buffer_mp3_pos = 0;
                    puts("no decode");
                    break;
                }
                //puts("audio out\n");
                audio_i2s_api_write((int16_t*)buffer_wav,info.outputSamps);
            }
            else if(err == -1) //data underflow
            {
                break;
            }
            else //other error
            {
                buffer_mp3_pos = 0;
                puts("other error");
                break;
            }

          
        }

     
        
        TickType_t elapsed = xTaskGetTickCount() - lastTick; 
        //printf("rec: %d Bytes in %d ms = %d KBytes/s\n",p->tot_len, elapsed, p->tot_len/elapsed);
        //printf("err %d\n",err);
        //printf("%s", buffer);
        lastTick = xTaskGetTickCount();
    }
    return ERR_OK;
}
