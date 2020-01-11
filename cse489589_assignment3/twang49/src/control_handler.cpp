/**
 * @control_handler
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <vector>
#include <stdint.h>

#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/connection_manager.h"

using namespace std;

#define UINT16_INF 65535
#define AUTHOR_STATEMENT "I, twang49, have read and understood the course academic integrity policy."


// /* for init response */
int control_socket, router_socket, data_socket;
uint16_t number_of_routers;
uint16_t updates_periodic_interval; 
uint16_t my_router_id;
uint16_t this_router_i;                       
uint16_t my_data_port;                      
uint16_t my_router_port;                    
uint32_t my_router_ip; 
// struct timeval timeout;
uint16_t **distance_vector;

vector<int> control_conn_list;
vector<Router>router_list;
vector<Time>time_list;

extern vector<int> data_sock_list;



void author_response(int sock_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
    cntrl_response_payload = (char *) malloc(payload_len);
    memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

    cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);
    /* Copy Payload */
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(sock_index, cntrl_response, response_len);

    free(cntrl_response);
}


std::string getIPfromUint16_t(uint32_t ip)
{
    struct in_addr ipAddr;
    ipAddr.s_addr = ip;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    std::string ip_add = str;
    return ip_add;
}


//---------------------------------------------------------------------------------------------
/* Init_response */
int create_router_sock(uint16_t router_port)
{
    int router_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(router_sock < 0)
    {
        ERROR("router, socket() failed");
        return -1;
    }

    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);
    bzero(&router_addr, sizeof(router_addr));
    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = htons(router_port);

    int opt = 1;
    int flag = setsockopt(router_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) ;
    if(flag < 0)
    {
        ERROR("router, setsockopt() failed");
        return -1;
    }

    flag = bind(router_sock, (struct sockaddr *)&router_addr, sizeof(router_addr)) ;
    if(flag < 0)
    {
        ERROR("router, bind() failed");
        return -1;
    }

    return router_sock;
}

int create_data_sock(uint16_t data_port)
{
    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(data_sock < 0)
    {
        ERROR("data, socket() failed");
        return -1;
    }

    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    int opt = 1;
    int flag = setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) ;
    if(flag < 0)
    {
        ERROR("data, setsockopt() failed");
        return -1;
    }

    bzero(&data_addr, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    data_addr.sin_port = htons(data_port);

    flag = bind(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));
    if(flag < 0)
    {
        ERROR("data, bind() failed");
        return -1;
    }

    flag = listen(data_sock, 5);
    if(flag < 0)
    {
        ERROR("data, listen() failed");
        return -1;
    }

    return data_sock;
}
int init_timeout()
{
    cout<<"[init_timeout] Start"<<endl;
    // timeout.tv_sec = updates_periodic_interval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    cout<<"[init_timeout] End"<<endl;
    return 0;
}
int init_distance_vector()
{
    cout<<"[init_distance_vector] Start"<<endl;
    uint16_t routers = number_of_routers;
    distance_vector = (uint16_t**)malloc(10*10*sizeof(uint16_t*));
    for(uint16_t i = 0; i < 10; i++)
    {
        distance_vector[i] = (uint16_t*)malloc(10*sizeof(uint16_t));
        for(uint16_t j = 0; j < 10; j++)
        {
            distance_vector[i][j] = UINT16_INF;
        }
    }
    cout<<"[init_distance_vector] End"<<endl;
    return 0;
}


int init_router_list(char *cntrl_payload, int offset)
{
    cout<<"[init_router_list] Start"<<endl;
    uint32_t dest_ip;
    bool dest_coonect;
    uint16_t routers = number_of_routers;
    uint16_t dest_id, dest_router_port, dest_data_port, cost, next_hop_id;
    uint16_t *id = (uint16_t*)malloc(sizeof(uint16_t)*10);
    uint16_t *cot = (uint16_t*)malloc(sizeof(uint16_t)*10);

    for(int i = 0; i < routers; i++)
    {
        struct Router router_temp;

        memcpy(&dest_id, cntrl_payload + offset, sizeof(dest_id));
        // dest_id = ntohs(dest_id);
        id[i] = dest_id;
        offset += 2;

        memcpy(&dest_router_port, cntrl_payload + offset, sizeof(dest_router_port));
        // dest_router_port = ntohs(dest_router_port);
        offset += 2;

        memcpy(&dest_data_port, cntrl_payload + offset, sizeof(dest_data_port));
        // dest_data_port = ntohs(dest_data_port);
        offset += 2;

        memcpy(&cost, cntrl_payload + offset, sizeof(cost));
        // cost = ntohs(cost);
        cot[i] = cost;
        offset += 2;

        memcpy(&dest_ip, cntrl_payload + offset, sizeof(dest_ip));
        // dest_ip = ntohl(dest_ip);
        offset += 4;
        if (ntohs(cost) == 0)
        {
            cout<<"find myself"<<endl;
            my_router_id = ntohs(dest_id);
            my_router_port = ntohs(dest_router_port);              
            my_data_port =  ntohs(dest_data_port);                   
            my_router_ip = ntohl(dest_ip);
            this_router_i = i;
            router_temp.next_hop_id = dest_id;
            router_temp.connect = false;
            router_temp.socket = -1;
        }
        else if(ntohs(cost) == UINT16_INF)
        {
            cout<<"find non-direct neigbor"<<endl;
            router_temp.next_hop_id = htons(UINT16_INF);
            router_temp.connect = false;
            router_temp.socket = -2;
        }
        else
        {
            cout<<"find direct neigbor"<<endl;
            router_temp.next_hop_id = dest_id;
            router_temp.connect = true;
            //建立连接 -> 发data ???????
            // router_temp.socket = new_route_conn(dest_id, dest_ip, dest_router_port);
        }

        router_temp.index = i;
        router_temp.dest_id = dest_id;
        router_temp.dest_router_port = dest_router_port;
        router_temp.dest_data_port = dest_data_port;
        router_temp.dest_ip = dest_ip;
        router_temp.cost = cost;
        cout<<"i: "<<i<<", id:"<<ntohs(dest_id)<<", router_port:"<<ntohs(dest_router_port)<<", data_port:"<<ntohs(dest_data_port)<<", ip:"<<getIPfromUint16_t(dest_ip)<<", cost:"<<ntohs(cost)<<endl;
        router_list.push_back(router_temp);


    }
    cout<<"after 1 for"<<endl;
    cout<<"my_router_id:"<<my_router_id<<endl;

    for(uint16_t i = 0;i != number_of_routers; i++)
    {
        cout<<"i:"<<i<<", id[i]"<<id[i]<<", cot[i]"<<ntohs(cot[i])<<endl;
        distance_vector[this_router_i][i] = ntohs(cot[i]);
    }
    free(id);
    free(cot);
    cout<<"[init_router_list] End"<<endl;
    return 0;
}
int init_time_list()
{
    cout<<"[init_time] Start"<<endl;
    for(vector<Router>::iterator it = router_list.begin(); it != router_list.end(); it ++)
    {
        struct Time t;
        t.router_ip_addr = it->dest_ip;
        t.index = it->index;
        t.socket = it->socket;
        gettimeofday(&t.begin_send_time,NULL);
        gettimeofday(&t.begin_expire_time,NULL);
        t.send_time.tv_sec = updates_periodic_interval;
        t.send_time.tv_usec = 0;
        t.expire_time.tv_sec = 3 * updates_periodic_interval;
        t.expire_time.tv_usec = 0;
        t.is_connect = it->connect;
        time_list.push_back(t);
    }
    cout<<"[init_time] End"<<endl;
    return 0;
}
int init_send_dv(int router_socket)
{
    cout<<"[init_send_dv] Start"<<endl;
    for(vector<Router>::iterator it2 = router_list.begin(); it2 != router_list.end(); it2 ++)
    {
        if(it2->connect)
        {
            send_dist_vector(router_socket, it2->dest_ip);
        }
    }
    cout<<"[init_send_dv] End"<<endl;
    return 0;
}

int init_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[init_response] Start"<<endl;

    int offset = 0;
    uint16_t routers;
    memcpy(&routers, cntrl_payload + offset, sizeof(routers));
    number_of_routers = ntohs(routers);
    cout<<"number_of_routers:"<<number_of_routers<<endl;
    offset += 2;
    
    uint16_t time_interval;
    memcpy(&time_interval, cntrl_payload + offset, sizeof(time_interval));
    updates_periodic_interval = ntohs(time_interval);
    cout<<"updates_periodic_interval:"<<updates_periodic_interval<<endl;
    offset += 2;

    init_timeout();
    init_distance_vector();
    init_router_list(cntrl_payload, offset);
    init_time_list(); 

    cout<<"after init_time, create_router_sock:"<<endl;
    router_socket = create_router_sock(my_router_port);
    FD_SET(router_socket, &master_list);
    head_fd = head_fd > router_socket ? head_fd : router_socket;

    cout<<"after create_router_sock, create_data_sock:"<<endl;
    data_socket = create_data_sock(my_data_port);
    FD_SET(data_socket, &master_list);
    head_fd = head_fd > data_socket ? head_fd : data_socket;

    // Send response command 
    cout<<"after create_data_sock, create_response_header:"<<endl;
    char *cntrl_header = create_response_header(sock_index, 1, 0, 0);
    // char *init_response = (char*) malloc(CNTRL_RESP_HEADER_SIZE ); 
    // memcpy(init_response, cntrl_header, CNTRL_RESP_HEADER_SIZE);
    int flag = sendALL(sock_index, cntrl_header, CNTRL_RESP_HEADER_SIZE );
    // sendALL(sock_index, init_response, CNTRL_RESP_HEADER_SIZE );
    if(flag < 0 )cout<<"sendALL failed"<<endl;
    else cout<<"sendALL: "<<flag<<" bytes."<<endl;
    // free(init_response);

    // Send dv to neighbor
    // init_send_dv(router_socket);
    free(cntrl_header);

    cout<<"[init_response] End"<<endl;
    return 0;
}
//---------------------------------------------------------------------------------------------


/* for routing_table_response */
int routing_table_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[routing_table_response] Start"<<endl;
    // uint8_t control_code = 2;
    // uint8_t response_code = 0;
    uint16_t payload_length = 4 * sizeof(uint16_t) * number_of_routers;
    cout<<"payload_length:"<<payload_length<<endl;
    char *routing_table_response = (char*) malloc(CNTRL_RESP_HEADER_SIZE + payload_length);
    routing_table_response = create_response_header(sock_index, 2, 0, payload_length);
    // memcpy(routing_table_response,cntrl_header,CNTRL_RESP_HEADER_SIZE);
    // struct  Router *r;
    uint16_t offset = 0; //////////////////////////////////////////////////////////////////////////
    uint16_t padding = 0;

    cout<<"router_list.size():"<<router_list.size()<<endl;
    // for(vector<Router>::iterator it = router_list.begin(); it != router_list.end(); it ++)
    for(int i = 0 ; i < router_list.size(); i++)
    {
        uint16_t dest_id, next_hop_id, cost;
        dest_id = router_list[i].dest_id;
        next_hop_id = router_list[i].next_hop_id;
        // cost = router_list[i].cost;
        // cout<<i<<"before memcpy"<<endl;

        memcpy(routing_table_response + offset + CNTRL_RESP_HEADER_SIZE, &dest_id, sizeof(uint16_t));
        offset += 2;
        memcpy(routing_table_response + offset + CNTRL_RESP_HEADER_SIZE, &padding, sizeof(uint16_t));
        offset += 2;
        memcpy(routing_table_response + offset + CNTRL_RESP_HEADER_SIZE, &next_hop_id, sizeof(uint16_t));
        offset += 2;

        // cout<<", this_router_i:"<<this_router_i;
        // cout<<", router_list[i].index:"<<router_list[i].index;
        // cout<<", dv:"<<distance_vector[this_router_i][router_list[i].index]<<endl;

        cost = htons(distance_vector[this_router_i][router_list[i].index]);  //?????????????
        memcpy(routing_table_response + offset + CNTRL_RESP_HEADER_SIZE, &cost, sizeof(uint16_t));
        offset += 2;
        cout<<"i:"<<i;
        cout<<", id:"<<ntohs(router_list[i].dest_id);
        cout<<", next_hop_id:"<<ntohs(router_list[i].next_hop_id);
        cout<<", cost[1]:"<<ntohs(router_list[i].cost);
        cout<<", cost[2]:"<<distance_vector[this_router_i][router_list[i].index]<<endl;

    }
    // cout<<"after for"<<endl;
    int flag;
    flag = sendALL(sock_index, routing_table_response, CNTRL_RESP_HEADER_SIZE + payload_length);
    free(routing_table_response); 
    cout<<"send "<<flag<<"bytes to controaller"<<endl;
    cout<<"[routing_table_response] End"<<endl;
    return 0;
}


//---------------------------------------------------------------------------------------------

/* for update_response */
int update_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{ 
    cout<<"[update_response] Start"<<endl;
    uint16_t id;
    uint16_t cost;
    int offset = 0;
    int index = -1;
    memcpy(&id, cntrl_payload, sizeof(uint16_t));
    offset += 2;
    memcpy(&cost, cntrl_payload + offset, sizeof(uint16_t));

    for(vector<Router>::iterator it = router_list.begin(); it != router_list.end(); it ++)
    {
        if(id == it->dest_id)
        {
            it->cost = cost;
            it->connect = true; 
            index = it->index;
            distance_vector[this_router_i][it->index] = ntohs(cost);/////////////////////////////
            // update_dv_table();/////////////////////////////////////// 
        }
    }

    for(vector<Time>::iterator it = time_list.begin(); it != time_list.end(); it ++)
    {
        if(it->index == index)
        {
            gettimeofday(&it->begin_send_time, NULL);
            gettimeofday(&it->begin_expire_time, NULL);
            it->send_time.tv_sec = updates_periodic_interval;
            it->send_time.tv_usec = 0;
            it->expire_time.tv_sec = 3 * updates_periodic_interval;
            it->expire_time.tv_usec = 0;
            it->is_connect = true;
        }
    }

    char *update_response = (char*) malloc(CNTRL_RESP_HEADER_SIZE);
    update_response = create_response_header(sock_index, 3, 0, 0);
    // memcpy(update_response,update_header, CNTRL_RESP_HEADER_SIZE);
    sendALL(sock_index, update_response, CNTRL_RESP_HEADER_SIZE);
    free(update_response); //有必要复制一份？？？
    cout<<"[update_response] End"<<endl;
    return 0;
}



//---------------------------------------------------------------------------------------------

/* for crash_response */
int crash_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[crash_response] Start"<<endl;
    char *crash_response = (char*) malloc(CNTRL_RESP_HEADER_SIZE);
    crash_response = create_response_header(sock_index, 4, 0, 0);
    // memcpy(crash_response,crash_header,CNTRL_RESP_HEADER_SIZE);
    sendALL(sock_index, crash_response, CNTRL_RESP_HEADER_SIZE);
    free(crash_response);
    close(router_socket);
    close(data_socket);
    // close(control_socket);
    cout<<"[crash_response] End"<<endl;
    // exit(0);
    return 0;
}

//---------------------------------------------------------------------------------------------


/* for sendfile_response */
int sendfile_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[sendfile_response] Start"<<endl;
    printf("need to implement sendfile_response()\n");
    cout<<"[sendfile_response] End"<<endl;
    return 0;
}
//---------------------------------------------------------------------------------------------



/* for sendfile_stats_respnse */
int sendfile_stats_respnse(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[sendfile_stats_respnse] Start"<<endl;
    printf("need to implement sendfile_stats_respnse()\n");
    cout<<"[sendfile_stats_respnse] End"<<endl;
    return 0;
}
//---------------------------------------------------------------------------------------------



/* for last_data_packet_response */
int last_data_packet_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[last_data_packet_response] Start"<<endl;
    printf("need to implement last_data_packet_response()\n");
    cout<<"[last_data_packet_response] End"<<endl;
    return 0;
}

//---------------------------------------------------------------------------------------------



/* for penultimate_data_packet_response */
int penultimate_data_packet_response(int sock_index, char *cntrl_payload, uint16_t payload_len)
{
    cout<<"[penultimate_data_packet_response] Start"<<endl;
    printf("need to implement penultimate_data_packet_response()\n");
    cout<<"[penultimate_data_packet_response] End"<<endl;
    return 0;
}

//---------------------------------------------------------------------------------------------
int create_control_sock(uint16_t control_port)
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(control_port);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    return sock;

}

int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, (socklen_t *)&caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    control_conn_list.push_back(fdaccept);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    cout<<"[remove_control_conn] Start"<<endl;
	for(vector<int>::iterator it = control_conn_list.begin(); it != control_conn_list.end(); it ++)
	{
		if(*it == sock_index)
		{
			control_conn_list.erase(it);
			break;
		}
	}
    cout<<"[remove_control_conn] middle"<<endl;
    close(sock_index);
    cout<<"[remove_control_conn] End"<<endl;
}

bool isControl(int sock_index)
{
    for(vector<int>::iterator it = control_conn_list.begin(); it != control_conn_list.end(); it ++)
	{
		if(*it == sock_index)
		{
			return true;
		}
	}
    return false;
}

bool isData(int sock_index)
{
    for(vector<int>::iterator it = data_sock_list.begin(); it != data_sock_list.end(); it ++)
    {
        if(*it == sock_index)
        {
            return true;
        }
    }
    return false;
}

bool control_recv_hook(int sock_index)
{
    cout<<"[control_recv_hook] Start"<<endl;
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    cout<<"before if recvALL"<<endl;
    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        cout<<"inside if recvALL"<<endl;
        remove_control_conn(sock_index);
        cout<<"after remove_control_conn"<<endl;
        free(cntrl_header); ////////////////////////
        cout<<"after free"<<endl;
        return false;
    }

    cout<<"before 2 memcpy"<<endl;
    /* Get control code and payload length from the header */
    memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
    memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
    payload_len = ntohs(payload_len);

    cout<<"before 2 free"<<endl;
    free(cntrl_header);//////////////////////////////
    cout<<"after 2 free"<<endl;

    /* Get control payload */
    if(payload_len != 0)
    {
        cout<<"payload_len != 0"<<endl;
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_index, cntrl_payload, payload_len) < 0)
        {
            cout<<"payload_len != 0, recvALL"<<endl;
            remove_control_conn(sock_index);
            cout<<"payload_len != 0, recvALL, free"<<endl;
            free(cntrl_payload);///////////////////////
            return false;
        }
    }

    cout<<"switch"<<endl;
    /* Triage on control_code */
    switch(control_code)
    {
        case 0: author_response(sock_index);
                break;
        case 1: printf("contral_handler.c / control_recv_hook / init_response.\n");
                init_response(sock_index, cntrl_payload, payload_len);
                break;
        case 2: printf("contral_handler.c / control_recv_hook / routing_table_response.\n");
                routing_table_response(sock_index, cntrl_payload, payload_len);
                break;
        case 3: printf("contral_handler.c / control_recv_hook / update_response.\n");
                update_response(sock_index, cntrl_payload, payload_len);
                break;
        case 4: printf("contral_handler.c / control_recv_hook / crash_response.\n");
                crash_response(sock_index, cntrl_payload, payload_len);
                break;
        case 5: printf("contral_handler.c / control_recv_hook / sendfile_response.\n");
                sendfile_response(sock_index, cntrl_payload, payload_len);
                break;
        case 6: printf("contral_handler.c / control_recv_hook / sendfile_stats_respnse.\n");
                sendfile_stats_respnse(sock_index, cntrl_payload, payload_len);
                break;
        case 7: printf("contral_handler.c / control_recv_hook / last_data_packet_response.\n");
                last_data_packet_response(sock_index, cntrl_payload, payload_len);
                break;
        case 8: printf("contral_handler.c / control_recv_hook / penultimate_data_packet_response.\n");
                penultimate_data_packet_response(sock_index, cntrl_payload, payload_len);
                break;
        default:
                printf("contral_handler.c / control_recv_hook / case default.\n");
                break;
    }

    if(payload_len != 0) free(cntrl_payload);/////////////
    cout<<"[control_recv_hook] End"<<endl;
    return true;
}