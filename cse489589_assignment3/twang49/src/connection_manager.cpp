/**
 * @connection_manager
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <vector>

#include "../include/global.h"
#include "../include/connection_manager.h"
#include "../include/control_handler.h"
#include "../include/control_header_lib.h"

using namespace std;

#define UINT16_INF 65535

/* new  */
uint16_t CONTROL_PORT;

fd_set master_list, watch_list;
int head_fd;
struct timeval timeout;
vector<int> data_sock_list;

extern std::vector<Router>router_list;
extern std::vector<Time>time_list;


struct Router find_router(uint32_t ip)
{
    struct Router result;
    int i = 0;
    for(i = 0; i < router_list.size(); i++)
    {
        if(router_list[i].dest_ip == ip)break;
    }
    result = router_list[i];
    return result;
}

/* for send distance vector */
int send_dist_vector(int router_socket, uint32_t ip)
{
    cout<<"[send_dist_vector] Start"<<endl;

    struct Router des_router = find_router(ip);

    struct sockaddr_in des_addr;
    des_addr.sin_family = AF_INET;
    des_addr.sin_port = des_router.dest_router_port;
    des_addr.sin_addr.s_addr = des_router.dest_ip;
    bzero(&(des_addr.sin_zero), 8);

    int offset = 0;
    int total_length = 8 + number_of_routers * 12;
    char* buffer = (char*)malloc(total_length);

    uint16_t routers, padding, source_router_port;
    uint32_t source_router_ip;
    routers = htons(number_of_routers);
    source_router_port = htons(my_router_port);
    source_router_ip = htonl(my_router_ip);
    padding = 0;

    /* finish header */
    memcpy(buffer, &routers, sizeof(uint16_t));
    offset += 2;
    memcpy(buffer + offset, &source_router_port, sizeof(uint16_t));
    offset += 2;
    memcpy(buffer + offset, &source_router_ip, sizeof(uint32_t));
    offset += 4;

    for(int i = 0; i < router_list.size(); i++)
    {
        uint16_t router_port, router_id, router_cost;
        uint32_t router_ip;
        router_port = router_list[i].dest_router_port;
        router_id = router_list[i].dest_id;
        // router_cost = router_list[i].cost;
        router_ip = router_list[i].dest_ip;

        memcpy(buffer + offset, &router_ip, sizeof(uint32_t));
        offset += 4;

        memcpy(buffer + offset, &router_port, sizeof(uint16_t));
        offset += 2;

        memcpy(buffer + offset, &padding, sizeof(uint16_t));
        offset += 2;

        memcpy(buffer + offset, &router_id, sizeof(uint16_t));
        offset += 2;

        router_cost = htons(distance_vector[this_router_i][router_list[i].index]);
        memcpy(buffer + offset, &router_cost, sizeof(uint16_t));
        offset += 2;
        cout<<"i: "<<i<<", router_port:"<<ntohs(router_list[i].dest_router_port)<<", id"<<ntohs(router_list[i].dest_id);
        cout<<", cost[1]:"<<ntohs(router_list[i].cost)<<", cost[2]:"<<distance_vector[this_router_i][router_list[i].index]<<", ip:"<<ntohl(router_list[i].dest_ip)<<endl;
    }

    int flag = sendto(router_socket, buffer, total_length, 0, (struct sockaddr *)&des_addr, sizeof(struct sockaddr_in));
    if (flag < 0)ERROR("Sendto error.");
    free(buffer);
    cout<<"sent: " << flag << " bytes"<<endl;
    cout<<"[send_dist_vector] End"<<endl;
    return 0;

}

/* for timeout */
struct timeval get_time_diff(struct timeval begin, struct timeval end, struct timeval interval)
{
    struct timeval remain, diff_time;
    remain.tv_sec = end.tv_sec + interval.tv_sec + ((int)end.tv_usec + (int)interval.tv_usec) / 1000000;
    remain.tv_usec = ((int)end.tv_usec + (int)interval.tv_usec) % 1000000;

    int microseconds = (remain.tv_sec - begin.tv_sec ) * 1000000 + (int)remain.tv_usec - (int)begin.tv_usec;
    if(microseconds <= 0)
    {
        diff_time.tv_sec = -100;
        diff_time.tv_usec = -100;
    }
    else
    {
        diff_time.tv_sec = microseconds / 1000000;
        diff_time.tv_usec = microseconds % 1000000;        
    }
    return diff_time;
}

int update_time_list(struct timeval currents)  // 需传入当前时间搓
{
    cout<<"[update_time_list] Start"<<endl;
    struct timeval current;
    gettimeofday(&current, NULL);
    struct timeval expire_time_interval, send_time_interval, diff_expire, diff_send;
    expire_time_interval.tv_sec = 3 * updates_periodic_interval, expire_time_interval.tv_usec = 0;
    send_time_interval.tv_sec = 1 * updates_periodic_interval, send_time_interval.tv_usec = 0;
    for(vector<Time>::iterator it = time_list.begin(); it != time_list.end(); it ++)
    {
        if(!it->is_connect)continue;

        /* is_connect == true */
        diff_expire = get_time_diff(current, it->begin_expire_time, expire_time_interval);
        diff_send = get_time_diff(current, it->begin_send_time, send_time_interval);

        // may need to updata expire_time and send_time;
        it->expire_time.tv_sec = diff_expire.tv_sec;
        it->expire_time.tv_usec = diff_expire.tv_usec;
        it->send_time.tv_sec = diff_send.tv_sec;
        it->send_time.tv_usec = diff_send.tv_usec;

        // if expired or not
        if(diff_expire.tv_sec < 0 || diff_expire.tv_usec < 0)
        {
            cout<<"expired"<<endl;
            it->is_connect = false;
            for(vector<Router>::iterator it2 = router_list.begin(); it2 != router_list.end(); it2 ++)
            {
                if(it2->index == it->index)
                {
                    it2->connect = false;
                    it2->cost = htons(UINT16_INF);
                    it2->next_hop_id = htons(UINT16_INF);//--------------------------------------
                    distance_vector[this_router_i][it2->index] = UINT16_INF;
                }
            }
        }
        // if send or not
        else if(diff_send.tv_sec < 0 || diff_send.tv_usec < 0)
        {
            cout<<"seend"<<endl;
            gettimeofday(&it->begin_send_time, NULL);
            it->send_time.tv_sec = updates_periodic_interval, it->send_time.tv_usec = 0; 
            // it->begin_send_time = cur; /////////////////////////////////////////
            for(vector<Router>::iterator it2 = router_list.begin(); it2 != router_list.end(); it2 ++)
            {
                if(it2->connect && it2->dest_ip == it->router_ip_addr)
                {
                    send_dist_vector(router_socket, it2->dest_ip);
                }
            }

        }
    }
    cout<<"[update_time_list] End"<<endl;
    return 0;
}
struct timeval get_timeout()
{
    cout<<"[get_timeout] Start"<<endl;
    struct timeval result;
    result.tv_sec = 1234567890, result.tv_usec = 0;//--------------------------------
    struct timeval zero;
    zero.tv_sec = zero.tv_usec = 0;
    bool flag = false;
    for(int i = 0; i < time_list.size(); i++)
    {
        cout<<"i:"<<i<<", is_connect:"<<time_list[i].is_connect<<", router_ip_addr:"<<ntohs(time_list[i].router_ip_addr);
        cout<<", begin_expire_time:"<<time_list[i].begin_expire_time.tv_sec<<", begin_send_time:"<<time_list[i].begin_send_time.tv_sec;
        cout<<", expire_time:"<<time_list[i].expire_time.tv_sec<<", send_time:"<<time_list[i].send_time.tv_sec<<endl;
        if(!time_list[i].is_connect)continue;
        
        if(flag)
        {
            struct timeval diff_expire = get_time_diff(result, time_list[i].expire_time, zero);
            if(diff_expire.tv_sec < 0 || diff_expire.tv_usec < 0)
            {
                result.tv_sec = time_list[i].expire_time.tv_sec;
                result.tv_usec = time_list[i].expire_time.tv_usec;
            }
            struct timeval diff_send = get_time_diff(result, time_list[i].send_time, zero);
            if(diff_send.tv_sec < 0 || diff_send.tv_usec < 0)
            {
                result.tv_sec = time_list[i].send_time.tv_sec;
                result.tv_usec = time_list[i].send_time.tv_usec;
            } 

        }
        else
        {
            flag = true;
            result.tv_sec = time_list[i].send_time.tv_sec;
            result.tv_usec = time_list[i].send_time.tv_usec;

            struct timeval diff_expire = get_time_diff(result, time_list[i].expire_time, zero);
            if(diff_expire.tv_sec < 0 || diff_expire.tv_usec < 0)
            {
                result.tv_sec = time_list[i].expire_time.tv_sec;
                result.tv_usec = time_list[i].expire_time.tv_usec;
            }        
        }
        
    }
    cout<<"[get_timeout] End"<<endl;
    return result;
}



int ipToIndex(uint32_t ipaddr)
{
    int ret;
    for(int i = 0; i < router_list.size(); i++){
        if(router_list[i].dest_ip == ipaddr)
        {
            ret = router_list[i].index;
            break;
        }
    }
    return ret;
}

int IDtoIndex(uint16_t ID)
{
    int ret;
    for(int i = 0; i < router_list.size(); i++){
        if(router_list[i].dest_id == ID)
        {
            ret = router_list[i].index;
            break;
        }
    }
    return ret;
}


void update_dv_table()
{
    cout<<"[update_dv_table] Start"<<endl;
    for(int i = 0; i < router_list.size(); i++)
    {
        if(router_list[i].connect)
        {
            for(int j = 0; j <  router_list.size(); j++)
            {
                //////////////////////////----------------------------------------------------
                
                if(distance_vector[this_router_i][router_list[i].index] == UINT16_INF) continue;
                if(distance_vector[router_list[i].index][router_list[j].index] == UINT16_INF) continue;
                if((distance_vector[this_router_i][router_list[j].index]) > (distance_vector[this_router_i][router_list[i].index]) + (distance_vector[router_list[i].index][router_list[j].index]))
                {
                    router_list[j].next_hop_id = router_list[i].dest_id;
                    distance_vector[this_router_i][router_list[j].index] = distance_vector[this_router_i][router_list[i].index]+distance_vector[router_list[i].index][router_list[j].index];
                    
                }
            }
        }
    }

    /* handle dead router  */
    for(int i = 0; i < router_list.size(); i++)
    {
        if(!router_list[i].connect)
        {
            if(distance_vector[IDtoIndex(router_list[i].next_hop_id)][router_list[i].index]==UINT16_INF)
            {
                distance_vector[this_router_i][router_list[i].index] = UINT16_INF;
            }
        }
    }


    /* check if INF */
    for(int i = 0; i < router_list.size(); i++)
    {
        if(distance_vector[this_router_i][router_list[i].index] == UINT16_INF)
        {
            router_list[i].next_hop_id = htons(UINT16_INF);
        }
    }



    cout<<"[update_dv_table] End"<<endl;
}




void print_dv_table()
{
    cout<<"[print_dv_table] Start"<<endl;
    for(int i = 0; i < router_list.size(); i++)
    {
        int indexFrom = router_list[i].index;
        cout<<"from: ID - " << ntohs(router_list[i].dest_id)<<endl;
        for(int j = 0; j < router_list.size(); j++)
        {
            cout<<"To: "<<ntohs(router_list[j].dest_id)<<" "<<distance_vector[indexFrom][router_list[j].index]<<"; ";
        }
        cout<<endl;
    }
    cout<<"[print_dv_table] End"<<endl;
}

int update_dv(int router_socket)
{


    cout<<"[update_dv] Start"<<endl;
    print_dv_table();

    struct sockaddr_in from_addr;    
    int numbytes = 0;

    /*[# of routers: 16 bits]
     *[source routing port: 16 bits]
     *[source IP : 32 bits]
     *[sum = 64 bits, or 8 bytes]

     *update item = 
     *[router IP : 32 bits]
     *[port: 16bits] [padding: 16 bits]
     *[ID: 16 bits]  [cost: 16 bits]
     *[sum = 96 bits, or 12 bytes]
    */

    /*receive routing update packet*/
    int length = 8 + number_of_routers * 12;
    char *buffer = (char*)malloc(length);
    socklen_t addr_len = sizeof(struct sockaddr);
    int flag = recvfrom(router_socket, buffer, length, 0, (struct sockaddr *)&from_addr, &addr_len);
    if (flag < 0)
    {
        ERROR("[update_dv], Recvfrom error.");
        exit(1);
    }
    //先parse

    /*extract packet header*/
    uint16_t number_of_update_fields, source_router_port;       
    uint32_t source_ip_addr;           
    int offset = 0;    


    memcpy(&number_of_update_fields, buffer + offset, sizeof(uint16_t));
    offset += 2;
    memcpy(&source_router_port, buffer + offset, sizeof(uint16_t));
    offset += 2;
    memcpy(&source_ip_addr, buffer + offset, sizeof(uint32_t));
    offset += 4;


    /*extract items*/
    uint16_t num = ntohs(number_of_update_fields);
    uint16_t padding;
    uint32_t *receive_ip = (uint32_t*)malloc(num * sizeof(uint32_t));
    uint16_t *receive_port = (uint16_t*)malloc(num * sizeof(uint16_t));
    uint16_t *receive_id = (uint16_t*)malloc(num * sizeof(uint16_t));
    uint16_t *receive_cost = (uint16_t*)malloc(num * sizeof(uint16_t));

    int i = 0;
    while(i < num)
    {
        memcpy(&receive_ip[i],buffer + offset,sizeof(uint32_t));
        offset += 4;
        memcpy(&receive_port[i],buffer + offset,sizeof(uint16_t));
        offset += 2;
        memcpy(&padding,buffer + offset,sizeof(uint16_t));
        offset += 2;   
        memcpy(&receive_id[i],buffer + offset,sizeof(uint16_t));
        offset += 2;
        memcpy(&receive_cost[i],buffer + offset,sizeof(uint16_t));
        offset += 2;
        i += 1;
    }


    /*find the sender index*/
    int senderIndex = ipToIndex(source_ip_addr);

    /*write distance vector in distance_vector 2d array*/    
    for(int i = 0; i < num; i++)
    {
        distance_vector[senderIndex][ipToIndex(receive_ip[i])] = ntohs(receive_cost[i]);
    }

    /*recompute dv table*/

    update_dv_table();

    /*update timer*/
    for(int i = 0; i < time_list.size(); i++)
    {
        if(time_list[i].is_connect && time_list[i].index == senderIndex)/////////////////////////////////
        {
            cout<<"expire timer updated"<<endl;
            gettimeofday(&(time_list[i].begin_expire_time),NULL);
            time_list[i].expire_time.tv_sec  = 3 * updates_periodic_interval;
            time_list[i].expire_time.tv_usec = 0;
            break;
        }
    }

    print_dv_table();

    cout<<"[update_dv] End"<<endl;
    free(buffer);
    free(receive_ip);
    free(receive_port);
    free(receive_id);
    free(receive_cost);

    return 0;
}
int new_data_conn(int data_socket)
{
    int newfd;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    newfd = accept(data_socket, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
    if(newfd < 0)
    {
        ERROR("new_data_conn, accept error!");
    }
    data_sock_list.push_back(newfd);
    return newfd;
}



void main_loop()
{
    timeout.tv_sec = 200000;
    int selret, sock_index, fdaccept;
    cout<<"[main_loop] Start"<<endl;
    while(true)
    {
        watch_list = master_list;
        cout<<"tv.sec:"<<timeout.tv_sec<<endl;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &timeout);
        cout<<"selret :"<<selret<<endl;

        if(selret < 0)
            ERROR("select failed.");

        if(!selret)
        {
            cout<<"selret == 0"<<endl;
            //selret == 0, represent timeout
            struct timeval current;
            update_time_list(current);
            timeout = get_timeout(); 
            cout<<"timeout event, new timeout:"<<timeout.tv_sec<<":"<<timeout.tv_usec<<endl;
        }

        /* Loop through file descriptors to check which ones are ready */
        for(sock_index=0; sock_index<=head_fd; sock_index+=1)
        {

            if(FD_ISSET(sock_index, &watch_list))
            {
                if(sock_index == 0)
                {
                    close(control_socket);
                    close(data_socket);
                    close(router_socket);
                    cout<<"server close"<<endl;
                    return;
                }

                /* control_socket */
                if(sock_index == control_socket)
                {
                    cout<<"sock_index = control_socket"<<endl;
                    fdaccept = new_control_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                /* router_socket */
                else if(sock_index == router_socket)
                {
                    cout<<"receive neighbor's dv"<<endl;
                    update_dv(router_socket);
                    //call handler that will call recvfrom() .....
                    // ToDo
                    cout<<"sock_index = router_socket"<<endl;
                }

                /* data_socket */
                else if(sock_index == data_socket)
                {
                    // int newfd = new_data_conn(data_index);
                    // FD_SET(newfd, &master_list);
                    // if(newfd > head_fd)head_fd = newfd;
                    cout<<"sock_index = data_socket"<<endl;
                }

                /* Existing connection */
                else{
                    cout<<"sock_index = else";
                    if(isControl(sock_index))
                    {
                        cout<<", isControl()"<<endl;
                        if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                    }
                    // else if (isData(sock_index))
                    // {
                    //     if(!data_recv_file(sock_index)) FD_CLR(sock_index, &master_list);
                    // }
                    else
                    {
                        cout<<", else"<<endl;
                        ERROR("Unknown socket index");
                    }
                }
            }
        }
    }
    cout<<"[main_loop] End"<<endl;
}

void init(uint16_t control_port)
{
    CONTROL_PORT = control_port;
    control_socket = create_control_sock(CONTROL_PORT);

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop();
}