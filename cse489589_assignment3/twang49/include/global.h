#ifndef _GLOBAL_H_
#define _GLOBAL_H_ 

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/time.h>

// typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

#define CNTRL_CONTROL_CODE_OFFSET 0x04
#define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#define CNTRL_RESP_RESPONSE_CODE_OFFSET 0x05
#define CNTRL_RESP_PAYLOAD_LEN_OFFSET 0x06

 // https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ 
// #define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

/* Store as network byte order. */
struct Router
{
    int index;   
    int socket;             
    bool connect;             
    uint16_t dest_id;              
    uint16_t dest_router_port;     
    uint16_t dest_data_port;       
    uint32_t dest_ip;              
    uint16_t cost;            
    uint16_t next_hop_id;        
};

/* time manager*/
struct Time
{
    bool is_connect;
    int index;
    int socket;
    uint32_t router_ip_addr;
    struct timeval begin_expire_time;
    struct timeval begin_send_time;
    struct timeval expire_time;
    struct timeval send_time;
};

#endif