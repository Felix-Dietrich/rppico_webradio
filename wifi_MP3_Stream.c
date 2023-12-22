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
#include "hardware/vreg.h"


#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )

#define MAX_MP3_FRAME_SIZE 1441
#define BUF_LEN (TCP_MSS + MAX_MP3_FRAME_SIZE)


//char ssid[] = "***REMOVED***";
//char pass[] = "***REMOVED***";

char ssid[] = "***REMOVED***";
char pass[] = "***REMOVED***";



typedef struct 
{
    int size;
    char data[TCP_MSS];
}buffer_t;

typedef struct 
{
    int size;
    int16_t data[1250];
}buffer_pcm_t;

typedef struct
{
    int16_t *data;
    int size;
    int readIndex;
    int writeIndex;
    SemaphoreHandle_t mutex;
}ring_buffer_t;

float volume = 0.01;


buffer_t http_buffer;
QueueHandle_t compressed_audio_queue;
QueueHandle_t pcm_audio_queue;
ring_buffer_t *ringbuffer;



void main_task(__unused void *params);
void vLaunch( void);

void http_request(void);
void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);
ring_buffer_t* createRingBuffer(int size);
void destroyRingBuffer(ring_buffer_t* rb);
void writeRingBuffer(ring_buffer_t* rb, uint16_t *data, int size);
void readRingBuffer(ring_buffer_t *rb, uint16_t *data, int size);

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
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    set_sys_clock_khz(250000,true);
    gpio_init(16);
    gpio_set_dir(16,GPIO_OUT);
    gpio_put(16,1);
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


void audio_out_task(__unused void *params)
{
    //printf("hallo von core%d\n",get_core_num());
    audio_i2s_api_init();
    uint8_t status = 0;
    static buffer_pcm_t buffer_wav;
    buffer_wav.size = 100;
    while (true)
    {
        if(status == 0)
        {
            xQueueReceive(pcm_audio_queue,&buffer_wav,portMAX_DELAY);
            //readRingBuffer(ringbuffer,buffer_wav.data,buffer_wav.size);
            //xQueueReceiveFromISR(pcm_audio_queue,&buffer_wav,portMAX_DELAY);
        }
        else
        {
            //puts("else");
            taskYIELD();
        }
        status = audio_i2s_api_write((int16_t*)buffer_wav.data,buffer_wav.size);
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
            printf("syncoffset:%d\n",syncoffset);
        }
        while(buffer_mp3_pos>0)
        {
            
          //  printf("syncoffset: %d\n", syncoffset);
            static unsigned char* inbuf;
            inbuf = buffer_mp3 + syncoffset;
            static int bytesleft; 
            bytesleft = buffer_mp3_pos;
            gpio_put(16,1);
            err= MP3Decode(decoder,&inbuf,&bytesleft, wav_buffer,0);
            gpio_put(16,0);
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
                for(int i = 0; i < (info.outputSamps-1); i+=2)
                {
                    buffer_pcm.data[i/2] = (wav_buffer[i]+wav_buffer[i+1])>>3;
                }
                buffer_pcm.size = info.outputSamps/2;
                //writeRingBuffer(ringbuffer,buffer_pcm.data,buffer_pcm.size);
                xQueueSend(pcm_audio_queue,&buffer_pcm,portMAX_DELAY);
            }
            else if(err == -1) //data underflow
            {
               // puts("underflow");
                break;
            }
            else //other error
            {
                buffer_mp3_pos = 0;
                puts("other error");
                break;
            }
        }
    }  
}

void vLaunch( void) 
{
    puts("task handle");
    TaskHandle_t task,task2, task3;
    puts("queues");
    compressed_audio_queue = xQueueCreate(2,sizeof(http_buffer));
    pcm_audio_queue = xQueueCreate(6,sizeof(buffer_pcm_t));
    ringbuffer = createRingBuffer(4000);
    puts("Tasks");
    xTaskCreate(main_task, "TestMainThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &task);
//    vTaskCoreAffinitySet(task2,1);
    xTaskCreate(audio_out_task,"Audio out Task",configMINIMAL_STACK_SIZE,NULL,TEST_TASK_PRIORITY,&task2);
    xTaskCreate(audio_decode_task,"Audio decode Task",configMINIMAL_STACK_SIZE,NULL,TEST_TASK_PRIORITY,&task3);
//    vTaskCoreAffinitySet(task3,2);
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


void http_request(void)
{
    puts("http_request");
    uint16_t port = 80;
    static httpc_connection_t settings;
    settings.result_fn = http_result;
    settings.headers_done_fn = http_header;
    //err_t err = httpc_get_file_dns("chmedia.streamabc.net", port, "/79-pilatus-mp3-192-4664468", &settings, http_body, NULL, NULL);
    err_t err = httpc_get_file_dns("stream.srg-ssr.ch", port, "/m/rsp/mp3_128", &settings, http_body, NULL, NULL);
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
}

err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    puts("\n\n\n\nhttp_header");
    puts("----------------------------------------");
    printf("content_length=%d\n", content_len);
    printf("header length=%d\n", hdr_len);
    printf("buffer length:%d\n",hdr->tot_len);
    pbuf_copy_partial(hdr, http_buffer.data, hdr_len, 0);
    pbuf_free_header(hdr,hdr_len);
    printf("headers:\n");
    http_buffer.data[hdr_len] = 0;
    printf("%s", http_buffer);
    return ERR_OK;
}

err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    //puts("\n\n\n\nhttp_body");
    //puts("-----------------------------------------");
    //printf("http task core%d\n",get_core_num());
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
    return ERR_OK;
}

ring_buffer_t* createRingBuffer(int size)
{
    ring_buffer_t *rb = (ring_buffer_t*)malloc(sizeof(ring_buffer_t));
    rb->data = (uint16_t*)malloc(size* sizeof(uint16_t));
    rb->size = size;
    rb->readIndex = 0;
    rb->writeIndex = 0;
    rb->mutex = xSemaphoreCreateMutex();
    puts("ringbuffer created\n");
    return rb;
}

void destroyRingBuffer(ring_buffer_t* rb)
{
    vSemaphoreDelete(rb->mutex);
    free(rb->data);
    free(rb);
}

void writeRingBuffer(ring_buffer_t* rb, uint16_t *data, int size)
{
    for(int i = 0; i < size; i++)
    {
        //xSemaphoreTake(rb->mutex,portMAX_DELAY);
        rb->data[rb->writeIndex] = data[i];
        rb->writeIndex++;
        if(rb->writeIndex >= rb->size)
        {
            rb->writeIndex = 0;
        }
        //xSemaphoreGive(rb->mutex);
        while(rb->writeIndex == rb->readIndex)
        {
            taskYIELD();
        }
    }
    
}

void readRingBuffer(ring_buffer_t *rb, uint16_t *data, int size)
{
    for(int i = 0; i < size; i++)
    {
        //xSemaphoreTake(rb->mutex,portMAX_DELAY);
        data[i] = rb->data[rb->readIndex];
        rb->readIndex++;
        if(rb->readIndex >= rb->size)
        {
            rb->readIndex = 0;
        }
        //xSemaphoreGive(rb->mutex);
        while((rb->readIndex+1)%rb->size == rb->writeIndex)
        {
            taskYIELD();
        }
    }

}
