#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

#if !NO_SYS
#define TCPIP_THREAD_STACKSIZE 2048
#define DEFAULT_THREAD_STACKSIZE 1024
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define TCPIP_MBOX_SIZE 8
#define LWIP_TIMEVAL_PRIVATE 0


//activate server
#define HTTPD_TCP_PRIO 1
#define LWIP_HTTPD 1
#define LWIP_HTTPD_SSI 1
#define LWIP_HTTPD_CGI 1
#define LWIP_HTTPD_SSI_INCLUDE_TAG 0
#define LWIP_HTTPD_SUPPORT_POST 0
#define HTTPD_FSDATA_FILE "htmldata.c" 

//activate MDNS
#define LWIP_MDNS_RESPONDER 1
#define LWIP_IGMP 1
//#define LWIP_AUTOIP 1
#define LWIP_NUM_NETIF_CLIENT_DATA 1
#define MDNS_RESP_USENETIF_EXTCALLBACK  1
#define MEMP_NUM_SYS_TIMEOUT 20
// not necessary, can be done either way
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1
#endif

#endif
