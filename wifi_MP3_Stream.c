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
#include "lwip/apps/httpd.h"
#include "lwipopts.h"
#include <flash_utils/flash_utils.h>
#include "ssi.h"
#include "cgi.h"
#include "lwip/apps/mdns.h"
#include <dhcpserver/dhcpserver.h>
#include <dnsserver/dnsserver.h>


#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif


#define MAX_MP3_FRAME_SIZE 1441
#define BUF_LEN (TCP_MSS + MAX_MP3_FRAME_SIZE)

#define LADESCHLUSS_V 4.2
#define ENTLADESCHLUSS_V 3.2

#define FILTERSIZE 200


const char ssid0[] = "dedietrich";
const char pass0[] = "dedietrich";

/*
const char stream0[] = "vintageradio.ice.infomaniak.ch/vintageradio-high.mp3";    //vintagereadio
const char stream1[] = "chmedia.streamabc.net/79-pilatus-mp3-192-4664468";        //radio Pilatus
const char stream2[] = "stream.srg-ssr.ch/m/rsp/mp3_128";                         //radio swiss pop
const char stream3[] = "stream.srg-ssr.ch/m/rsc_de/mp3_128";                      //swiss classic
const char stream4[] = "chmedia.streamabc.net/79-rbern1-mp3-192-7860422";         //bern1
//const char stream5[] = "www.energybern.ice.infomaniak.ch/energybern-high.mp3";    //energy bern
//const char stream5[] = "www.energybern.ice.infomaniak.ch/energybern-low.mp3";
const char stream5[] = "185.74.70.51/energybern-high.mp3";
const char stream6[] = "stream.srg-ssr.ch/m/drs1/mp3_128";                        //srf1
const char stream7[] = "stream.srg-ssr.ch/m/rsj/mp3_128";                         //swiss jazz
const char stream8[] = "streaming.swisstxt.ch/m/drs3/mp3_128";                    //srf3
const char stream9[] = "chmedia.streamabc.net/79-ffm-mp3-192-2470075";            //flashback
const char stream10[] ="chmedia.streamabc.net/79-virginrockch-mp3-192-2872456";   //virgin radio

const char* streams[] = {stream0,stream1,stream2,stream3,stream4,stream5,stream6,stream7,stream8,stream9,stream10};
*/
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
bool is_connected_ap = false;

buffer_t http_buffer;
QueueHandle_t compressed_audio_queue;
QueueHandle_t raw_audio_queue;
QueueHandle_t processed_audio_queue;


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

void vLaunch( void);

void http_request(void);
void start_stream_mp3(const char* stream_link);
void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
err_t http_body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);

static void srv_txt(struct mdns_service *service, void *txt_userdata);
void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume);

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


void main_task(__unused void *params)
{
    watchdog_enable(1000,1);
    gpio_init(16);
    gpio_set_dir(16,GPIO_OUT);
    gpio_put(16,1);

    gpio_init(11);
    gpio_set_dir(11,GPIO_OUT);
    gpio_put(11,1);
    static buffer_pcm_t audio_buffer;
    static int senderwechsel = 0;
    static int lastStream = -1;
     //printf("main task on core%d\n",get_core_num());
    while(true)
    {
        vTaskDelay(100);
        if(is_connected && !is_streaming)
        {
            //start_stream_mp3(streams[current_stream]);
            if ((flash_content_r->sender[current_stream][0] >= 0x30) && (flash_content_r->sender[current_stream][0] <= 0x7E))
            {
                xQueueReset(compressed_audio_queue);
                xQueueReset(raw_audio_queue);
                start_stream_mp3(flash_content_r->sender[current_stream]);
                is_streaming = true;
                volume = 0;
            }
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
                is_connected = false;
                cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
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
                if(ssid==flash_content_r->ssid && pass==flash_content_r->password)
                {
                    ssid=ssid0;
                    pass=pass0;
                    cyw43_arch_wifi_connect_async(ssid, pass, CYW43_AUTH_WPA2_AES_PSK);
                }
                else
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

                    /*if (!tcp_server_open(state)) 
                    {
                        DEBUG_printf("failed to open server\n");
                        return 1;
                    }*/
                }
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

void audio_out_task(__unused void *params)
{
    //printf("hallo von core%d\n",get_core_num());
    audio_i2s_api_init();
    uint8_t status = 0;
    static buffer_pcm_t buffer_wav;
    buffer_wav.size = 0;
    buffer_wav.samplerate = 0;

    while (true)
    {
        
        if(status == 0)
        {
            xQueueReceive(processed_audio_queue,&buffer_wav,portMAX_DELAY);
        }
        else
        {
            vTaskDelay(1);
            //taskYIELD();
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
        gpio_put(11,1);
        adc_select_input(0);
        uint16_t ADCresult0 = adc_read();
        adc_select_input(1);
        uint16_t ADCresult1 = adc_read();
        adc_select_input(2);
        uint16_t ADCresult2 = adc_read();

        float battery_v = ADCresult2*3.3*2/4096;
        battery_percent = (battery_v-ENTLADESCHLUSS_V)*100.0/(LADESCHLUSS_V-ENTLADESCHLUSS_V);
        if(battery_percent<0)
        {
            battery_percent = 0;
        }
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

void http_server_task(__unused void *params)
{
    while(!is_connected && !is_connected_ap)
    {
        vTaskDelay(100);
    }
    httpd_init();
    ssi_init();
    cgi_init();
    mdns_resp_init();
    mdns_resp_add_netif(&cyw43_state.netif[CYW43_ITF_STA], "deDietrich");
    mdns_resp_add_service(&cyw43_state.netif[CYW43_ITF_STA], "picow_freertos_httpd", "_http", DNSSD_PROTO_TCP, 80, srv_txt, NULL);
    while (true)
    {
        static bool wasConnected = true;
        if(is_connected && wasConnected == false)
        {
            mdns_resp_restart(&cyw43_state.netif[CYW43_ITF_STA]);
        }
        
        vTaskDelay(5000);
        //mdns_resp_restart(&cyw43_state.netif[CYW43_ITF_STA]);
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


void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    
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
    vTaskDelay(500);
    puts("retry");
    is_streaming = false;    
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
            //puts("Err: too big to handle\n");
            tcp_recved(conn,p->tot_len);
            pbuf_free(p);
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
    if(uri == NULL)
    {
        printf("ungültige URL");
        return;
    }
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

static void srv_txt(struct mdns_service *service, void *txt_userdata)
{
  err_t res;
  LWIP_UNUSED_ARG(txt_userdata);

  res = mdns_resp_add_service_txtitem(service, "path=/", 6);
  LWIP_ERROR("mdns add service txt failed\n", (res == ERR_OK), return);
}


void equalizer(buffer_pcm_t* in, buffer_pcm_t* out, float volume)
{
    static int16_t filter[FILTERSIZE];
    static int16_t in_last[FILTERSIZE];
    int32_t processedData;
    int32_t volume2 = (1<<15)*volume;


    for(int i = 0; i< FILTERSIZE; i++)
    {
        filter[i] = (1<<14)/FILTERSIZE;
        //filter[i]=0;
    }
    filter[FILTERSIZE/2] = 1<<8;

    out->samplerate = in->samplerate;
    out->size = in->size;

    
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