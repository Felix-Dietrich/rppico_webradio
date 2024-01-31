/*
* WIFI_TASK
* Created: 31.01.2023 Manfred Dietrich
* Description: WiFi connection management 
*/

//#include "pico/stdlib.h"
#include <stdlib.h>
#include <dhcpserver/dhcpserver.h>
#include <dnsserver/dnsserver.h>
#include <flash_utils/flash_utils.h>
#include <FreeRTOS.h>
#include <hardware/watchdog.h>
#include <pico/cyw43_arch.h>

#include "analog_input_task.h"
#include "RTOS_globals.h"


// Defintions
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;


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


