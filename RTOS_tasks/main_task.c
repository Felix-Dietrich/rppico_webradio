/*
* MAIN_TASK
* Created: 30.01.2023 Manfred Dietrich
* Description: Stream supervisor 
*/

#include "main_task.h"
#include "RTOS_globals.h"

#include <string.h>
#include <audioI2SAPI/audio_i2s_api.h>
#include <flash_utils/flash_utils.h>
#include <hardware/watchdog.h>
#include <hardware/gpio.h>
#include <lwip/apps/http_client.h>


void start_stream_mp3(const char* stream_link);
void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
err_t http_header(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
err_t http_body(void *arg, struct tcp_pcb *conn, struct pbuf *p, err_t err);


void main_task(__unused void *params)
{
    flash_utils_init();
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
                //printf("senderwechsel: %d\n",senderwechsel);
                stop_stream = true;  
            }
             
        }
    }
}


void start_stream_mp3(const char* stream_link)
{
    //printf("start Stream: %s\n", stream_link);
    char* uri = strchr(stream_link,'/');
    if(uri == NULL)
    {
        //printf("ungültige URL");
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

// Callback function
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


// Callback function
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

