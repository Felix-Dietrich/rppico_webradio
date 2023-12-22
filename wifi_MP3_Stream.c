#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <audioI2SAPI/audio_i2s_api.h>
#include <picomp3lib/mp3dec.h>
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"


#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif


#define MAX_MP3_FRAME_SIZE 1441
#define BUF_LEN (TCP_MSS + MAX_MP3_FRAME_SIZE)



//char ssid[] = "***REMOVED***";
//char pass[] = "***REMOVED***";

char ssid[] = "***REMOVED***";
char pass[] = "***REMOVED***";

const char stream0[] = "vintageradio.ice.infomaniak.ch/vintageradio-high.mp3";    //vintagereadio
const char stream1[] = "chmedia.streamabc.net/79-pilatus-mp3-192-4664468";        //radio Pilatus
const char stream2[] = "stream.srg-ssr.ch/m/rsp/mp3_128";                         //radio swiss pop
const char stream3[] = "stream.srg-ssr.ch/m/rsc_de/mp3_128";                      //swiss classic
const char stream4[] = "chmedia.streamabc.net/79-rbern1-mp3-192-7860422";         //bern1
const char stream5[] = "energybern.ice.infomaniak.ch/energybern-high.mp3";        //energy bern
const char stream6[] = "stream.srg-ssr.ch/m/drs1/mp3_128";                        //srf1
const char stream7[] = "stream.srg-ssr.ch/m/rsj/mp3_128";                         //swiss jazz
const char stream8[] = "streaming.swisstxt.ch/m/drs3/mp3_128";                    //srf3
const char stream9[] = "chmedia.streamabc.net/79-ffm-mp3-192-2470075";            //flashback
const char stream10[] ="chmedia.streamabc.net/79-virginrockch-mp3-192-2872456";   //virgin radio

const char* streams[] = {stream0,stream1,stream2,stream3,stream4,stream5,stream6,stream7,stream8,stream9,stream10};

int current_stream = 0;

typedef struct 
{
    int size;
    char data[TCP_MSS];
}buffer_t;

typedef struct 
{
    int size;
    int samplerate;
    int16_t data[1250];
}buffer_pcm_t;


float volume = 0;

bool stop_stream = false;
bool is_streaming = false;
bool is_connected = false;

buffer_t http_buffer;
QueueHandle_t compressed_audio_queue;
QueueHandle_t raw_audio_queue;
QueueHandle_t processed_audio_queue;


void vLaunch( void);

void http_request(void);
void start_stream_mp3(const char* stream_link);
void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);


int main()
{
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    set_sys_clock_khz(200000,true);
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
    watchdog_enable(1000,1);
    gpio_init(16);
    gpio_set_dir(16,GPIO_OUT);
    gpio_put(16,1);
    static buffer_pcm_t audio_buffer;
    static int senderwechsel = 0;
    static int lastStream = -1;
     //printf("main task on core%d\n",get_core_num());
    while(true)
    {
        vTaskDelay(100);
        if(is_connected && !is_streaming)
        {
            //while (xQueueReceive(compressed_audio_queue, &http_buffer, 300));            
            //while (xQueueReceive(raw_audio_queue, &audio_buffer, 10));
            xQueueReset(compressed_audio_queue);
            xQueueReset(raw_audio_queue);
            //volume = 0;
            start_stream_mp3(streams[current_stream]);
            is_streaming = true;
            lastStream = current_stream;
        }
        if(is_streaming && !stop_stream)
        {   
            if(lastStream != current_stream)
            {
                senderwechsel++;
                printf("senderwechsel: %d\n",senderwechsel);
                stop_stream = true;  
            }
             
        }
    }
}

void wifi_task(__unused void *params)
{
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWITZERLAND))
    {
        while (true)
        {
            puts("init failed");
            sleep_ms(1000);
        }
    }
    puts("initialised");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,true);
    cyw43_arch_enable_sta_mode();

    cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);

    
    while (true)
    {   
        static int last_status = INT32_MIN;
        int new_status = cyw43_tcpip_link_status(&cyw43_state,CYW43_ITF_STA);
        if(last_status != new_status)
        {
            last_status=new_status;
            is_connected = false;
            switch(new_status)
            {
                case CYW43_LINK_BADAUTH:
                printf("badauth\n");
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_DOWN:
                printf("link down\n");
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
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
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_NONET:
                printf("link no net\n");
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                break;
                case CYW43_LINK_UP:
                printf("link up\n");
                is_connected = true;
                break;

            }
        }
        vTaskDelay(100);
    }
}

void audio_decode_task(__unused void *params)
{
    static unsigned char buffer_mp3[BUF_LEN];
    static buffer_pcm_t buffer_pcm;
    static int buffer_mp3_pos = 0;
    //printf("hallo von core%d\n",get_core_num());
    HMP3Decoder decoder = MP3InitDecoder();
    
    while (true)
    {
        //puts("mp3_task");
        static buffer_t receive_buffer;
        static int16_t wav_buffer[2500];
        xQueueReceive(compressed_audio_queue,&receive_buffer,portMAX_DELAY);
        //printf("bytes empfangen audio out task: %d\n", receive_buffer.size);
        memcpy(buffer_mp3+buffer_mp3_pos,receive_buffer.data,receive_buffer.size);
        buffer_mp3_pos+=receive_buffer.size;
        //printf("total data: %d\n", buffer_mp3_pos);
        //puts("decode\n");
        
        static int err;
        err = -1;
        // Find the start of the mp3 frame.
        static int syncoffset; 
        syncoffset = MP3FindSyncWord(buffer_mp3,buffer_mp3_pos);
        if(syncoffset < 0)
        {
            // No sync word found, discard the buffer.
            buffer_mp3_pos = 0;
            puts("no sync found\n");
            break;
        }
        if(syncoffset >0)
        {
            //printf("syncoffset:%d\n",syncoffset);
        }
        while(buffer_mp3_pos>0)
        {
            //puts("mp3_decode");
          //  printf("syncoffset: %d\n", syncoffset);
            static unsigned char* inbuf;
            inbuf = buffer_mp3 + syncoffset;
            static int bytesleft; 
            bytesleft = buffer_mp3_pos;
            //gpio_put(16,1);
            err= MP3Decode(decoder,&inbuf,&bytesleft, wav_buffer,0);
            //gpio_put(16,0);
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
                buffer_pcm.samplerate = info.samprate;
                static int Samprate = 0;
                
                //puts("audio out\n");
                for(int i = 0; i < (info.outputSamps-1); i+=2)
                {
                    buffer_pcm.data[i/2] = ((wav_buffer[i]+wav_buffer[i+1])>>1);
                }
                buffer_pcm.size = info.outputSamps/2;
                xQueueSend(raw_audio_queue,&buffer_pcm,portMAX_DELAY);
            }
            else if(err == -1) //data underflow
            {
               // puts("underflow");
                break;
            }
            else //other error
            {
                buffer_mp3_pos = 0;
               // puts("other error");
                break;
            }
        }
    }  
}

void audio_process_task(__unused void *params)
{
    static buffer_pcm_t buffer_raw;
    static buffer_pcm_t buffer_processed;
    static bool wasPlaying = false;
    while(true)
    {
        if(xQueueReceive(raw_audio_queue,&buffer_raw,20)==pdFALSE)
        {
            buffer_raw.size=1250;
            buffer_raw.samplerate=44100;
            if (wasPlaying)
            {
                volume=0;
                wasPlaying=false;
            }
            
            for(int i = 0; i < buffer_raw.size; i++)
            {
                buffer_raw.data[i] = rand()%10000;
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
        buffer_processed.samplerate= buffer_raw.samplerate;
        buffer_processed.size = buffer_raw.size; 
        for(int i = 0; i < buffer_raw.size; i++)
        {
            buffer_processed.data[i] = buffer_raw.data[i] * volume;
        }
        xQueueSend(processed_audio_queue,&buffer_processed,portMAX_DELAY);
    }
}

void audio_out_task(__unused void *params)
{
    //printf("hallo von core%d\n",get_core_num());
    audio_i2s_api_init();
    uint8_t status = 0;
    static buffer_pcm_t buffer_wav;
    buffer_wav.size = 100;
    buffer_wav.samplerate = 48000;

    while (true)
    {
        
        if(status == 0)
        {
            xQueueReceive(processed_audio_queue,&buffer_wav,portMAX_DELAY);
        }
        else
        {
            vTaskDelay(1);
        }
        if((buffer_wav.samplerate>10000) && (buffer_wav.samplerate<100000))
        {
            status = audio_i2s_api_write((int16_t*)buffer_wav.data,buffer_wav.size, buffer_wav.samplerate);
            watchdog_update();
        }
        else
        {
            status = 0;
        }
        
    }
    
}


void analog_in_task(__unused void *params)
{
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_select_input(0);
    while(true)
    {
        adc_select_input(0);
        uint16_t ADCresult0 = adc_read();
        adc_select_input(1);
        uint16_t ADCresult1 = adc_read();
        volume = volume*0.95+(float)ADCresult0/4096*0.05;

        const int steps= 11;
        const int ADCResolution = 4096;
        const float overlap = 0.5;
        const int divider = ADCResolution/steps+1;

        if(abs(((current_stream*divider)+divider/2)-ADCresult1)>(2*divider*overlap))
        {
            current_stream = ADCresult1/divider;
            //printf("ADC selection: %d\n", ADCselection);
        }

        vTaskDelay(20);
    }

}

void vLaunch( void) 
{ 
    puts("task handle");
    TaskHandle_t main_task_handle, audio_out_task_handle, audio_decode_task_handle, audio_process_task_handle, analog_in_task_handle, wifi_task_handle;
    puts("queues");
    compressed_audio_queue = xQueueCreate(8,sizeof(http_buffer));
    raw_audio_queue = xQueueCreate(2,sizeof(buffer_pcm_t));
    processed_audio_queue = xQueueCreate(2,sizeof(buffer_pcm_t));
    puts("Tasks");
    xTaskCreate(main_task, "Main Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &main_task_handle);
    xTaskCreate(audio_out_task,"Audio out Task",configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY+1,&audio_out_task_handle);
    xTaskCreate(audio_decode_task,"Audio decode Task",configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY+1,&audio_decode_task_handle);
    xTaskCreate(audio_process_task,"Audio process Task",configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY+1,&audio_process_task_handle);
    xTaskCreate(analog_in_task,"Analog in Task",configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY+1,&analog_in_task_handle);
    xTaskCreate(wifi_task,"Wifi Task",configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY+1,&wifi_task_handle);

    puts("start");

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}


void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    is_streaming = false;
    stop_stream = 0;
    puts("\n\n\n\nhttp_result");
    puts("----------------------------------------");
    printf("local result=");
    switch (httpc_result)
    {
    case 0:
        puts("File successfully received");
    break;
    case 1:
        puts("Unknown error");
    break;
    case 2:
        puts("Connection to server failed");
    break;
    case 3:
        puts("Failed to resolve server hostname");
    break;
    case 4:
        puts("Connection unexpectedly closed by remote server");
    break;
    case 5:
        puts("Connection timed out (server didn't respond in time");
    break;
    case 6:
        puts("Server responded with an error code");
    break;
    case 7:
        puts("Local memory error");
    break;
    case 8:
        puts("Local abort");
    break;
    case 9:
        puts("Content length mismatch");
    break;
    default:
        break;
    }
    printf("http result=%d\n", srv_res);
    puts("retry");    
}

err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    is_streaming = true;
    //puts("\n\n\n\nhttp_header");
    //puts("----------------------------------------");
    if(hdr != NULL)
    {
        //printf("content_length=%d\n", content_len);
        //printf("header length=%d\n", hdr_len);
        //printf("buffer length:%d\n",hdr->tot_len);
        //pbuf_copy_partial(hdr, http_buffer.data, hdr_len, 0);
        //pbuf_free_header(hdr,hdr_len); //das führt zu einem Fehler, wenn hdr_len=hdr->tot_len
        //printf("headers:\n");
        //http_buffer.data[hdr_len] = 0;
        //printf("%s", http_buffer.data);
        return ERR_OK;
    }
    else
    {
        puts("HTTP header empty");
        is_streaming = false;
        return ERR_OK;
    }
}

err_t http_body(void *arg, struct tcp_pcb *conn, struct pbuf *p, err_t err)
{
    //puts("\n\n\n\nhttp_body");
    //puts("-----------------------------------------");
    //printf("http task core%d\n",get_core_num());
    if(stop_stream)
    {
        
        pbuf_free(p);
        //tcp_abort(conn);
        //return ERR_ABRT;
        //tcp_shutdown(conn,0,1); 
        tcp_close(conn);
        is_streaming = false;
        stop_stream = 0;
        return ERR_OK;
        
        //return ERR_OK;
    }
    if(p != NULL)
    {    
        if(p->tot_len > TCP_MSS)
        {
            puts("Err: too big to handle\n");
            tcp_recved(conn,p->tot_len);
            return ERR_OK;
        }
        
        //puts("buffer copy\n");
        pbuf_copy_partial(p, http_buffer.data, p->tot_len, 0);
        http_buffer.size = p->tot_len;
        //puts("queue_send");
        xQueueSend(compressed_audio_queue,&http_buffer,portMAX_DELAY);
        //printf("received Data: %d\n",p->tot_len);
        tcp_recved(conn,p->tot_len);
        //puts("buffer free\n");
        pbuf_free(p);
    }
    else
    {
        puts("http body empty");
    }
    return ERR_OK;
}

void start_stream_mp3(const char* stream_link)
{
    printf("start Stream: %s\n", stream_link);
    char* uri = strchr(stream_link,'/');
    char server_name[100];
    strncpy(server_name, stream_link, uri-stream_link);
    server_name[uri-stream_link]=0;
//    printf("server name: %s\n", server_name);
//    printf("uri: %s\n", uri);
    uint16_t port = 80;
    static httpc_connection_t settings;
    settings.result_fn = http_result;
    settings.headers_done_fn = http_header;
    err_t err = httpc_get_file_dns(server_name, port, uri, &settings, http_body, NULL, NULL);
//    printf("http_request status %d \n", err);
}