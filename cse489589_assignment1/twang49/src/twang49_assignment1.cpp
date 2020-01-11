/**
 * @ubitname_assignment1
 * @author  Fullname <ubitname@buffalo.edu>
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
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <math.h>

#include "../include/global.h"
#include "../include/logger.h"

#include <algorithm> 
#include <sstream>
#include <string>

using namespace std;

typedef unsigned int Uint;

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define FALSE 0
#define CMD_SIZE 65535
#define MSG_SIZE 65535
#define BUFFER_SIZE 65535
#define INT_MAX_NUM 65535

string my_hostname;
string my_port;
string my_ip;
bool log_status = FALSE;
int my_portnum;
int my_sockfd;
struct addrinfo hints;
struct addrinfo *my_addrinfo;
bool print_log_in = 0;

// to do
//SERVER blocked 问题：不论成功与否，log都会打印两次 ， 
//用户重新登录后，login-list有问题， 会double->清空socketlist(每次logout)  fflush(stdout);

int string2int(string str);

//****struct Socket************
//reference:https://github.com/jessefjxm/CSE589-Text-Chat-Application/blob/master/src/hwang58_assignment1.cpp
struct SocketObject
{
    int cfd;
    int port_num;
    int num_msg_sent;
    int num_msg_rcv;
    string status; 
    string ip;
    string port;
    string hostname;
    vector<string> blockeduser;
    vector<string> msgbuffer;
    bool operator<(const SocketObject &abc) const 
    {
        return string2int(port) < string2int(abc.port);
    }
};
vector<SocketObject> socketlist;


bool is_number(char c);

int is_valid_ip(string ip_addr);

int is_valid_port(string port_number);

int fsize(FILE *fp);

int initiate_p2p_sender(string sender_port, string recv_ip, string recv_port, string file_path);

int initiate_p2p_receiver( string recv_port );


SocketObject* newSocketObject(int cfd, string hostname, string ip, string port) 
{
    SocketObject* hd = new SocketObject;
    hd->cfd = cfd;
    hd->port_num = string2int(port);
    hd->num_msg_sent = 0;
    hd->num_msg_rcv = 0;
    hd->ip = ip;
    hd->port = port;
    hd->status = "logged-in";
    hd->hostname = hostname;
    return hd;
}

SocketObject* is_exist_Socket(string ip, string port) 
{
    //we can delete
    Uint i = 0;
    while(i < socketlist.size())
    {
        SocketObject* hd = &socketlist[i];
        int falg = hd->ip == ip && hd->port == port;
        if(falg)return hd;
        i ++;
    }   
  return NULL;
}

SocketObject* is_exist_Socket(string ip) 
{
    Uint i = 0;
    while(i < socketlist.size())
    {
        SocketObject* hd = &socketlist[i];
        int falg = hd->ip == ip;
        if(falg)return hd;
        i ++;
    }   
  return NULL;
}

// new ===
SocketObject* is_exist_Socket(int cfd) 
{
    Uint i = 0;
    while(i < socketlist.size())
    {
        SocketObject* hd = &socketlist[i];
        int falg = hd->cfd == cfd;
        if(falg)return hd;
        i ++;
    }   
  return NULL;
}



//  ===


int string2int(string str)
{
    int n = str.length(), res = 0, j = 1, i;
    for(i = n - 1; i >= 0; i --)
    {
        res += (str[i] - '0') * j;
        j *= 10;
    }
    return res;
}
string cutoff(char *charmsg)
{
    string tmp = charmsg;
    while(tmp[tmp.length() - 1] == '\n' || tmp[tmp.length() - 1] == ' ' )
    {
        tmp = tmp.substr(0, tmp.length() - 1);
    }
    while(tmp[0] == '\n' || tmp[0] == ' ' )
    {
        tmp = tmp.substr(1, tmp.length());
    }
    return tmp;
}
//******************************LOOGER***FOR***HELP*****************
void print_log_success(string command)
{

    cse4589_print_and_log("[%s:SUCCESS]\n", command.c_str());
}

void print_log_end(string command)
{

    cse4589_print_and_log("[%s:END]\n", command.c_str());
    printf("\n");
}

void print_log_error(string command)
{
    cse4589_print_and_log("[%s:ERROR]\n", command.c_str());
    print_log_end(command);
}


//******************************LOOGER***FOR***BOTH*****************
void print_log_AUTHOR()
{
    string command = "AUTHOR"; //可能是个隐患
    print_log_success(command);
    string ubitname = "twang49";
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname.c_str());
    print_log_end(command);
}

void print_log_IP()
{
    string command = "IP";
    print_log_success(command);
	cse4589_print_and_log("IP:%s\n", my_ip.c_str());
	print_log_end(command);
}

void print_log_PORT()
{
    string command = "PORT";
    print_log_success(command);
	cse4589_print_and_log("PORT:%d\n", my_portnum);
	print_log_end(command);
}

void print_log_LIST()
{
    string command = "LIST";
    sort(socketlist.begin(), socketlist.end());
    print_log_success(command);
	int n = socketlist.size(), i = 0; // make sure socketlist.size()
    cout<<"socketlist.size():"<<n<<endl;
	while(i < n)
	{
        if(socketlist[i].status == "logged-in")
        {
    	    int list_id = i + 1;
    	    string hostname = socketlist[i].hostname;
    	    string ip_addr = socketlist[i].ip;
    	    int port_num = socketlist[i].port_num;
    	    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname.c_str(), ip_addr.c_str(), port_num);
        }
        i++;
	}
    print_log_end(command);
}

//******************************LOOGER***FOR***SERVER*****************
void print_log_STATISTICS()
{
    string command = "STATISTICS";
    sort(socketlist.begin(), socketlist.end());
    int i = 0, n = socketlist.size(); //make sure the socketlist.size()
    print_log_success(command);
    while(i < n)
    {
        int list_id = i + 1;
        string hostname = socketlist[i].hostname;
        int num_msg_sent = socketlist[i].num_msg_sent;
        int num_msg_rcv = socketlist[i].num_msg_rcv;
        string status = socketlist[i].status;
        cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", list_id, hostname.c_str(), num_msg_sent, num_msg_rcv, status.c_str());
        i += 1;
    }
	print_log_end(command);
}

void print_log_BLOCKED(string ip)
{
    string command = "BLOCKED";
    SocketObject *temp, *hd = is_exist_Socket(ip);
    int flag = !is_valid_ip(ip) || hd == NULL;
    if(flag){print_log_error(command); return;}

    print_log_success(command);
    int i = 0;
    while(i < hd->blockeduser.size())
    {
        temp = is_exist_Socket(hd->blockeduser[i]);
        if(temp == NULL){printf("error in print blocked\n"); return;}
        cse4589_print_and_log("%-5d%-35s%-20s%-8s\n", i + 1, temp->hostname.c_str(), temp->ip.c_str(), temp->port.c_str());
        i += 1;
    }
	print_log_end(command);
}

void print_log_RECEIVED_EVENT(string client_ip, string msg)
{
    string command = "RECEIVED";
    print_log_success(command);
    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", client_ip.c_str(), msg.c_str());
	print_log_end(command);
}

void print_log_RELAYED_EVENT(string from_client_ip, string msg, string to_client_ip)
{
    string command = "RELAYED";
    print_log_success(command);
    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_client_ip.c_str(), to_client_ip.c_str(), msg.c_str());
	print_log_end(command);
}


int get_my_ip()   // reference:https://github.com/elliotleee/CSE589_Proj1_SocketProgramming
{
    struct sockaddr_in serv, name;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    const char* kGoogleDnsIp = "8.8.8.8";
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    uint16_t kDnsPort = 53;
    serv.sin_port = htons(kDnsPort);

    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    int temp = connect(temp_sock, (const sockaddr*)&serv, sizeof(serv));
    socklen_t namelen = sizeof(name);
    temp = getsockname(temp_sock, (sockaddr*)&name, &namelen);

    char buffer[256];
    size_t buflen = 256;
    string temp_ip;
    temp_ip = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    my_ip = temp_ip;
    // printf("my_ip:%s\n", my_ip.c_str());
    close(temp_sock);

    return 0;
}

int initialize_server_addr(string port_tmp)
{

    int flag;
    my_port = port_tmp;
    my_portnum = string2int(port_tmp);
    const char *my_char_port = port_tmp.c_str();
    // printf("my_port:%s\n", my_port.c_str());
    // printf("my_portnum:%d\n", my_portnum);

    char temp_hostname[1024]; // size may change
    flag = gethostname(temp_hostname, sizeof(temp_hostname) - 1);
    if(flag < 0)
    {
        printf("gethostname fail!\n");
        return -1;   
    }
    my_hostname = temp_hostname;
    // printf("my_hostname:%s\n", my_hostname.c_str());

    flag = get_my_ip();
    // printf("my_ip:%s\n", my_ip.c_str());
    // if(!flag)
    // {
    //     printf("get_my_ip fail!\n");
    //     return -1;
    // }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    flag = getaddrinfo(NULL, my_char_port, &hints, &my_addrinfo);
    if(flag)
    {
        printf("getaddrinfo fail!\n");
        return -1;   
    }

    // Create socket
    my_sockfd = socket(my_addrinfo->ai_family, my_addrinfo->ai_socktype, my_addrinfo->ai_protocol);
    flag = my_sockfd;
    if(flag < 0)
    {
        printf("socket fail!\n");
        return -1;  
    }
    
    // make pocket resuable
    int opt = 1;
    if (setsockopt(my_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        printf("socket fail!\n");
        return -1; 
    } 


    // bind
    flag = bind(my_sockfd, my_addrinfo->ai_addr, my_addrinfo->ai_addrlen);
    if(flag < 0)
    {
        printf("bind fail!\n");
        return -1;  
    }
    freeaddrinfo(my_addrinfo);
    // printf("bind complete!\n");
    return 0;
}

vector<string> split_string(const string str, const string &seperator)
{
    vector<string> res;
    if(str == "")return res;
    string strs = str + seperator;
    int size = strs.size(), i = 0, m = seperator.size();
    while(i < size)
    {
        while(str[i] == seperator[0])i ++; //--------------------
        int posi = strs.find(seperator, i);
        if(posi < size)
        {
            string tmp = strs.substr(i, posi - i);
            i = posi + m;
            res.push_back(tmp);
        }
    }
    return res;
}

bool is_number(char c)
{
    if(c - '0' >= 0 && c - '9' <= 0)return true;
    else return false;
}

int is_valid_ip(string ip_addr)
{
    vector<string> ip_portion;
    ip_portion = split_string(ip_addr, ".");
    if(ip_portion.size() != 4)return 0;

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < ip_portion[i].size(); j++)
        {
            if(!is_number(ip_portion[i][j]))return 0;
            if(string2int(ip_portion[i]) > 255) return 0;
        }
    }
    return 1;
}

int is_valid_port(string port_number)
{
    for(int j = 0; j < port_number.length(); j++)
    {
        if(!is_number(port_number[j]))return 0;
    }
    int temp_port = string2int(port_number);
    if(temp_port >= 1024 && temp_port <= 49151)return 1;
    else return 0;
}


int server_process(string port_tmp)
{
    printf("server_process\n");

    char charmsg[INT_MAX_NUM]; 
    int flag, server_socket, head_socket, selret, sock_index, fdaccept=0;
    socklen_t caddr_len;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_list, watch_list, readfds;
    vector<string> split_cmd;
    int master_counter = 020;

    flag = initialize_server_addr(port_tmp);
    if(flag)
    {
        printf("initialize_server_addr fail!\n");
        return -1;
    }

    /* Listen */
    if(listen(my_sockfd, BACKLOG) < 0)
    {
        perror("Unable to listen on port");
        return -1;
    }

    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the listening socket */
    FD_SET(my_sockfd, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);

    head_socket = my_sockfd;

    while(TRUE)
    {

        memcpy(&watch_list, &master_list, sizeof(master_list));

        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
        {
            perror("select failed.");
            return -1;
        }

        /* Check if we have sockets/STDIN to process */
        if(selret)
        {
            /* Loop through socket descriptors to check which ones are ready */
            for(sock_index=0; sock_index<=head_socket; sock_index+=1)
            {

                if(FD_ISSET(sock_index, &watch_list))
                {

                    /* Check if new command on STDIN */
                    if (sock_index == STDIN)
                    {
                        printf("stdin\n");
                        char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
                    	memset(cmd, '\0', CMD_SIZE);
                        // if(fgets(cmd, sizeof cmd -1, stdin) == NULL) 
						if(fgets(cmd, CMD_SIZE, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
						
                        //Process PA1 commands here ...
                        string tmp_cmd = cmd;
                        if(cmd[strlen(cmd) - 1] == '\n')tmp_cmd = tmp_cmd.substr(0, tmp_cmd.length() - 1);
                        
                        split_cmd = split_string(tmp_cmd, " ");
                        for(int i = 0; i < split_cmd.size(); i++)
                            // printf("split_cmd[%d]:%s\n", i, split_cmd[i].c_str());

                        if(split_cmd[0] == "AUTHOR")
                        {
                            print_log_AUTHOR(); //finish
                        }
                        else if(split_cmd[0] == "IP")
                        {
                            print_log_IP(); //finish
                        }
                        else if(split_cmd[0] == "PORT")
                        {
                            print_log_PORT(); //finish
                        }
                        else if(split_cmd[0] == "LIST")
                        {
                            print_log_LIST(); //
                        }
                        else if(split_cmd[0] == "STATISTICS")
                        {
                            print_log_STATISTICS(); 
                        }
                        else if(split_cmd[0] == "BLOCKED")
                        {
                            if(split_cmd.size() != 2) print_log_error(split_cmd[0]);
                            else
                            {
                                string temp_ip = split_cmd[1];
                                print_log_BLOCKED(temp_ip); 
                            }
                        }
                        else
                        {
                            printf("check if you type in correct command!!\n\n\n\n");   
                        }

						free(cmd);
                    }
                    /* Check if new client is requesting connection */
                    else if(sock_index == my_sockfd)
                    {
                        printf("new connection socket:: \n");
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(my_sockfd, (struct sockaddr *)&client_addr, &caddr_len);
                        if(fdaccept < 0)
                            perror("Accept failed.");

						// printf("\nRemote Host connected!\n");

                        /* Add to watched socket list */
                        FD_SET(fdaccept, &master_list);  
                        if(fdaccept > head_socket) head_socket = fdaccept;

                        memset(charmsg, '\0', MSG_SIZE);///////dddddd

                        recv(fdaccept, charmsg, MSG_SIZE, 0);

                        string temp_msg = charmsg;

                        printf("and receive msg: %s\n", temp_msg.c_str());

                        vector<string> str_msgp = split_string(temp_msg, " ");


                        if(str_msgp[0] == "LOGIN")
                        {
                            string temp_ip = str_msgp[2];
                            SocketObject* hd = is_exist_Socket(temp_ip);
                            printf("somebody try to log in, IP:%s.\n", temp_ip.c_str());

                            if(hd == NULL)
                            {
                                //new client, create and save the socketobject.
                                string hostname =  str_msgp[1];
                                string host_ip = str_msgp[2];
                                string host_port = str_msgp[3];
                                hd = newSocketObject(fdaccept, hostname, host_ip, host_port);
                                socketlist.push_back(*hd);
                                printf("client first log in, HOSTNAME: %s ,IP: %s ,PORT: %s.\n", hostname.c_str(), host_ip.c_str(), host_port.c_str());
                                
                            }
                            else
                            {
                                // old client, cheack the msgbuffer;
                                hd->status = "logged-in";
                                if(hd->msgbuffer.size())
                                {
                                    printf("client 2+ log in,buffer size != 0, # of messgs = %lu\n", hd->msgbuffer.size());
                                    string msg = "";
                                    vector<string>::iterator it;
                                    it = hd->msgbuffer.begin();
                                    while(it != hd->msgbuffer.end())
                                    {
                                        msg += *it + "\n";  // "\n" "/n";
                                        it ++;
                                    }
                                    hd->msgbuffer.clear();
                                    send(hd->cfd, msg.c_str(), msg.length(), 0);
                                }
                            }

                            // tell the client LIST of other logged-in user info;
                            Uint i = 0;
                            string list_msg = "LIST_LOGIN ";
                            while(i < socketlist.size())
                            {
                                if(socketlist[i].status == "logged-in")
                                {
                                    list_msg += socketlist[i].hostname + " ";
                                    list_msg += socketlist[i].ip + " ";
                                    list_msg += socketlist[i].port + " ";
                                }
                                i++;
                            }
                            flag = send(fdaccept, list_msg.c_str(), strlen(list_msg.c_str()), 0);
                            if(flag > 0)
                            {
                                printf("send mesagge{list} SUCCESS: %s.\n", list_msg.c_str());
                            }
                            else
                            {
                                printf("send mesagge{list} Failed.\n");
                            }
                        }
                        else
                        {
                            printf("server got ?????request, and receive msg: %s\n", temp_msg.c_str());
                        }
                        // else if()
                        // {

                        // }
                        // else if()
                        // {
                            
                        // }
                        // else if()
                        // {
                            
                        // }
                        // else if()
                        // {
                            
                        // }
                        // else
                        // {

                        // }
                        // free();
                    }
                    /* Read from existing clients */
                    else
                    {
                        printf("existing connection socket.\n");
                        /* Initialize buffer to receieve response */
                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);
                        string msg = "";
                        vector<string> msgp;
                        SocketObject* hd;

                        int recv_flag;
                        recv_flag = recv(sock_index, buffer, BUFFER_SIZE, 0);
                        

                        SocketObject* hh = is_exist_Socket(sock_index);
                        if(hh)
                        {
                            printf("request from: \n %s\n%s\n", hh->hostname.c_str(), hh->ip.c_str());
                        }else{
                            printf("No host matched.\n");
                        }
                        printf("recv flag = %d\n", recv_flag);


                        if( recv_flag <= 0)
                        {
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");
                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        }
                        else 
                        {
                        	msg = buffer;
                            // printf("buffer:{%s}, size:{%lu}\n", buffer, strlen(buffer));
                            // printf("msg:{%s}, size:{%lu}\n", msg.c_str(), msg.length());
                            msgp = split_string(msg, " ");
                            printf("got old message :{%s}\n", msg.c_str());
                            if(msgp[0] == "EXIT") // case 5 EXIT
                            {
                                printf("EXIT\n");
                                int i = 0;
                                while(i < socketlist.size())
                                {
                                    if (socketlist[i].cfd == sock_index) 
                                    {
                                        printf("some body exit, find it, delete it\n");
                                        socketlist.erase(socketlist.begin() + i); 
                                        i -= 1;
                                    }
                                    i += 1;
                                }
                            }
                            // ==============XXXXXXXXX====================
                            else if((msgp[0] == "SEND")) //case 9
                            {
                                printf("SEND\n");
                                string sender_ip = msgp[1];
                                string receiver_ip = msgp[2];
                                SocketObject* sender_sc = is_exist_Socket(sender_ip);
                                SocketObject* receiver_sc = is_exist_Socket(receiver_ip);

                                if( sender_sc == NULL || receiver_sc == NULL)
                                {
                                    printf("valid check ip\n");
                                    continue;   //break;?????????????????????
                                }

                                // check if sender is blocked by reveiver
                                vector<string> ::iterator ite;
                                ite = find(receiver_sc->blockeduser.begin(), receiver_sc->blockeduser.end(), sender_ip);
                                if(ite != receiver_sc->blockeduser.end())
                                {
                                    printf("block\n");
                                    continue;  //break;?????????????????????
                                }

                                // statistics
                                receiver_sc->num_msg_rcv = receiver_sc->num_msg_rcv + 1;
                                sender_sc->num_msg_sent = sender_sc->num_msg_sent + 1;

                                // Assemble contents
                                string contents = "";
                                for(int i = 3; i < msgp.size(); i++){
                                    if(i==3) {contents = msgp[i];}
                                    else {contents = contents + " " + msgp[i];}
                                }

                                // check if logged in
                                if(receiver_sc->status == "logged-in")
                                {
                                    // send
                                    send(receiver_sc->cfd, (const char*)msg.c_str(),msg.length(), 0);

                                }else{
                                    // if not logged-in, then buffer the message
                                    receiver_sc->msgbuffer.push_back(msg);
                                    printf("msg:{%s}, buffer_size:%lu\n", msg.c_str(), receiver_sc->msgbuffer.size());

                                }
                                print_log_RELAYED_EVENT(sender_ip, contents, receiver_ip);
                            }
                            // ==============XXXXXXXXX====================
                            else if(msgp[0] == "LOGIN")
                            {
                                string temp_ip = msgp[2];
                                printf("somebody try to log in, ip:%s.\n", temp_ip.c_str());
                                SocketObject* hd = is_exist_Socket(temp_ip);

                                if(hd == NULL)
                                {
                                    //new client, create and save the socketobject.
                                    string hostname =  msgp[1];
                                    string host_ip = msgp[2];
                                    string host_port = msgp[3];
                                    hd = newSocketObject(fdaccept, hostname, host_ip, host_port);
                                    socketlist.push_back(*hd);
                                    printf("client first log in, HOSTNAME: %s ,IP: %s ,PORT: %s.\n", hostname.c_str(), host_ip.c_str(), host_port.c_str());
                                    
                                }
                                else
                                {
                                    // old client, cheack the msgbuffer;
                                    hd->status = "logged-in";
                                    if(hd->msgbuffer.size())
                                    {
                                        printf("client 2+ log in,buffer size != 0\n");
                                        string msg = "";
                                        vector<string>::iterator it;
                                        it = hd->msgbuffer.begin();
                                        int tmp = 1;
                                        while(it != hd->msgbuffer.end())
                                        {
                                            msg = *it + "\n";   // ---"SEND"----------------------------
                                            flag = send(hd->cfd, msg.c_str(), msg.length(), 0);   //????????
                                            it ++;
                                            if (flag > 0)
                                            {
                                                printf("\nsend buffer message to re-log-on client, message#{%d} :{%s}\n", tmp ++, msg.c_str());
                                            }
                                            else
                                            {
                                                // printf("send buffer message to re-log-on client, FAILED\n");
                                            }
                                        }
                                        hd->msgbuffer.clear();
                                    }
                                    else
                                    {
                                        printf("smgbuffer == 0\n");
                                    }
                                }

                                // tell the client LIST of other logged-in user info;
                                Uint i = 0;
                                string list_msg = "LIST_LOGIN ";
                                while(i < socketlist.size())
                                {
                                    if(socketlist[i].status == "logged-in")
                                    {
                                        list_msg += socketlist[i].hostname + " ";
                                        list_msg += socketlist[i].ip + " ";
                                        list_msg += socketlist[i].port + " ";
                                    }
                                    i++;
                                }
                                list_msg = list_msg + "\n";
                                // fdaccept = hd->cfd;
                                flag = send(hd->cfd, list_msg.c_str(), strlen(list_msg.c_str()), 0);//--------------------
                                if(flag > 0)
                                {
                                    printf("send mesagge{list} SUCCESS: %s.\n", list_msg.c_str());
                                }
                                else
                                {
                                    printf("send mesagge{list} Failed.\n");
                                }
                            }
                        
                            else if((msgp[0] == "BLOCK")) //case 2
                            {
                                string from_ip = msgp[1], to_ip = msgp[2];
                                printf("client:{%s} try to block{%s}\n", from_ip.c_str(), to_ip.c_str());
                                flag = !is_valid_ip(from_ip) || !is_valid_ip(to_ip) || is_exist_Socket(from_ip) == NULL || is_exist_Socket(to_ip) == NULL;
                                if(flag){print_log_error(msgp[0]), printf("not valid ip or not exit socket\n"); continue;}
                                int i = 0;
                                SocketObject *hd = is_exist_Socket(from_ip);
                                while(i < hd->blockeduser.size())
                                {
                                    if(hd->blockeduser[i] == to_ip)break;
                                    i++;
                                }
                                if(i >= hd->blockeduser.size())hd->blockeduser.push_back(to_ip);
                                else printf("in blacked list\n");
                                
                            }
                            else if((msgp[0] == "UNBLOCK")) //case 3
                            {
                                string from_ip = msgp[1], to_ip = msgp[2];
                                printf("client:{%s} try to unblock{%s}\n", from_ip.c_str(), to_ip.c_str());
                                flag = !is_valid_ip(from_ip) || !is_valid_ip(to_ip) || is_exist_Socket(from_ip) == NULL || is_exist_Socket(to_ip) == NULL;
                                if(flag){ print_log_error(msgp[0]), printf("not valid ip or not exit socket\n");continue; }
                                int i = 0;
                                SocketObject *hd = is_exist_Socket(from_ip);
                                while(i < hd->blockeduser.size())
                                {
                                    if(hd->blockeduser[i] == to_ip)break;
                                    i++;
                                }
                                if(i >= hd->blockeduser.size())printf("not in blacked list\n");
                                else hd->blockeduser.erase(hd->blockeduser.begin() + i);
                            }
                            else if((msgp[0] == "LOGOUT")) //case 4， LOGOUT
                            {
                                printf("LOGOUT\n");
                                string logout_ip = msgp[1];
                                hd = is_exist_Socket(logout_ip);
                                if(hd == NULL)
                                {
                                    printf("some body want to log out, but we can't find it ????\n");
                                    printf("IP: %s\n", logout_ip.c_str());
                                    // break;
                                }
                                else
                                {
                                    hd->status = "logged-out";
                                    printf("some body want to log out, find it, set status:logged-out\n");
                                }

                            }
                            else if((msgp[0] == "BROADCAST")) //case 6
                            {
                                //Send message: <msg> to all “logged-in” clients.
                                printf("BROADCAST\n");
                                string sender_ip = msgp[1];
                                string receiver_ip = "255.255.255.255";
                                SocketObject* sender_sc = is_exist_Socket(sender_ip);


                                if( sender_sc == NULL )
                                {
                                    printf("invalid sender\n");
                                    continue;   //break;?????????????????????
                                }

                                for(int i = 0; i < socketlist.size(); i++)
                                {
                                    SocketObject receiver_sc_o = socketlist[i];

                                    SocketObject *receiver_sc = &receiver_sc_o;

                                    if(receiver_sc->ip == sender_ip) continue;
                                    
                                    // check if sender is blocked by reveiver
                                    vector<string> ::iterator ite;
                                    ite = find(receiver_sc->blockeduser.begin(), receiver_sc->blockeduser.end(), sender_ip);
                                    if(ite != receiver_sc->blockeduser.end())
                                    {
                                        printf("blocked\n");
                                        continue;  //break;?????????????????????
                                    }

                                    // statistics
                                    receiver_sc->num_msg_rcv = receiver_sc->num_msg_rcv + 1;
                                    sender_sc->num_msg_sent = sender_sc->num_msg_sent + 1;

                                    // Assemble contents
                                    string contents = "";
                                    for(int i = 2; i < msgp.size(); i++){
                                        if(i==2) {contents = msgp[i];}
                                        else {contents = contents + " " + msgp[i];}
                                    }

                                    // check if logged in
                                    if(receiver_sc->status == "logged-in")
                                    {
                                        // send
                                        send(receiver_sc->cfd, (const char*)msg.c_str(),msg.length(), 0);

                                    }else{
                                        // if not logged-in, then buffer the message ？？?不在线的话不需要发消息
                                        // receiver_sc->msgbuffer.push_back(msg);
                                        // printf("msg:{%s}, buffer_size:%lu\n", msg.c_str(), receiver_sc->msgbuffer.size());

                                    }
                                    print_log_RELAYED_EVENT(sender_ip, contents, receiver_ip);

                                }
                                
                            }
                            else if((msgp[0] == "REFRESH")) //case 7
                            {
                                // tell the client LIST of other logged-in user info;
                                string temp_ip = msgp[1];
                                printf("somebody try to refresh, ip:%s.\n", temp_ip.c_str());
                                SocketObject* hd = is_exist_Socket(temp_ip);
                                if(hd == NULL){printf("can't fint the client!\n");}

                                Uint i = 0;
                                string list_msg = "LIST_LOGIN ";
                                while(i < socketlist.size())
                                {
                                    if(socketlist[i].status == "logged-in")
                                    {
                                        list_msg += socketlist[i].hostname + " ";
                                        list_msg += socketlist[i].ip + " ";
                                        list_msg += socketlist[i].port + " ";
                                    }
                                    i++;
                                }
                                flag = send(hd->cfd, list_msg.c_str(), strlen(list_msg.c_str()), 0);//--------------------
                                if(flag > 0)printf("refresh list success:{list} SUCCESS: %s.\n", list_msg.c_str());
                                else printf("refresh list{list} Failed.\n");  
                            }
                            else if((msgp[0] == "P2P_SEND_REQUEST")) 
                            {
                                string sender_ip = msgp[1];
                                string receiver_ip = msgp[2];
                                SocketObject* sender_sc = is_exist_Socket(sender_ip);
                                SocketObject* receiver_sc = is_exist_Socket(receiver_ip);

                                if( sender_sc == NULL || receiver_sc == NULL)
                                {
                                    printf("invalid check ip\n");
                                    continue;   //break;?????????????????????
                                }

                                // check if sender is blocked by reveiver
                                vector<string> ::iterator ite;
                                ite = find(receiver_sc->blockeduser.begin(), receiver_sc->blockeduser.end(), sender_ip);
                                if(ite != receiver_sc->blockeduser.end())
                                {
                                    printf("blocked\n");
                                    continue;  //break;?????????????????????
                                }

                                char str_tmp[4];
                                memset(str_tmp, '\0', 4);
                                master_counter += 10;
                                sprintf(str_tmp, "%d", master_counter);

                                string sender_port = "22" + string(str_tmp); ////////
                                string recv_port = "23" + string(str_tmp);
                                printf("sender port: %s, rec port: %s\n", sender_port.c_str(), recv_port.c_str());
                                string message_to_sender = "";
                                string message_to_receiver = "";

                                message_to_sender = message_to_sender + "P2P_SEND_PORT" + " " + sender_port + " " + recv_port;
                                message_to_receiver = message_to_receiver + "P2P_RECV_PORT" + " " + recv_port;

                                // check if logged in
                                if(receiver_sc->status == "logged-in")
                                {
                                    // send
                                    printf("mess to sender: %s\n", message_to_sender.c_str());
                                    printf("mess to receiver: %s\n", message_to_receiver.c_str());
                                    
                                    send(sender_sc->cfd, (const char*)message_to_sender.c_str(),message_to_sender.length(), 0);

                                    send(receiver_sc->cfd, (const char*)message_to_receiver.c_str(),message_to_receiver.length(), 0);

                                }

                            }
                            else
                            {
                                printf("case which haven't been taken care\n");
                            }
                            fflush(stdout);//------------------------------------------------------------FOR DOUBLE

                        }

                        free(buffer);
                    }
                }
            }
        }
    }
	return 0;
}

int connect_to_host(char *server_ip, int server_port)
{
    int fdsocket, len;
    struct sockaddr_in remote_server_addr;

    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(fdsocket < 0)
       perror("Failed to create socket");

    bzero(&remote_server_addr, sizeof(remote_server_addr));
    remote_server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &remote_server_addr.sin_addr);
    remote_server_addr.sin_port = htons(server_port);

    if(connect(fdsocket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) < 0)
        perror("Connect failed");

    return fdsocket;
}

int client_process(string port_tmp)
{
    // printf("client_process\n");
    initialize_server_addr(port_tmp);

    log_status = FALSE;
    int flag, server_socket, head_socket, selret, sock_index, fdaccept=0, fdmax = my_sockfd;
    socklen_t caddr_len;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_list, watch_list, readfds;
    struct addrinfo *servinfo;
    int log_in_time = 0;

    // below variables are for p2p
    string sender_port, recv_ip, recv_port, file_path; 
    string ssss_ip, ssss_port;


	while(TRUE)
    {
		printf("\n[PA1-Client@CSE489/589]$ ");
        FD_ZERO(&readfds);
		fflush(stdout);

		char *charmsg = (char*) malloc(sizeof(char)*MSG_SIZE);
   	    memset(charmsg, '\0', MSG_SIZE);

        // set proper monitor list
        if(log_in_time > 0)
        {
            FD_SET(fileno(stdin), &readfds);
            FD_SET(my_sockfd, &readfds);
            fdmax = my_sockfd;

        }else{
            FD_SET(fileno(stdin), &readfds);
            fdmax = 0;
        }

        // set select
        select(fdmax + 1, &readfds, NULL, NULL, NULL);

        // if activity from keyboard
        if (FD_ISSET(fileno(stdin), &readfds))
        {   
            char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
            memset(cmd, '\0', CMD_SIZE);
            if(fgets(cmd, CMD_SIZE, stdin) == NULL) //Mind the newline character that will be written to cmd
                exit(-1);

            printf("stdin GET command:%s\n", cmd);

            string tmp_cmd = cmd;
            if(cmd[strlen(cmd) - 1] == '\n')
            {
                tmp_cmd = tmp_cmd.substr(0, tmp_cmd.length() - 1);
            }
            // printf("\nI got tmp_cmd :{%s}, size:{%lu}\n", tmp_cmd.c_str(), tmp_cmd.length());
            vector<string> split_cmd = split_string(tmp_cmd, " ");
            // printf("\nI got split_cmd[0] :{%s}, size:{%lu}\n", split_cmd[0].c_str(), split_cmd[0].length());
            fflush(stdin);

            if(split_cmd[0] == "AUTHOR")
            {
                print_log_AUTHOR();
            }
            else if(split_cmd[0] == "IP")
            {
                print_log_IP();
            }
            else if(split_cmd[0] == "PORT")
            {
                print_log_PORT();
            }
            else if(split_cmd[0] == "LIST" && log_status) 
            {
                print_log_LIST();
            }
            else if(split_cmd[0] == "LOGOUT" && log_status)
            {  
                string msg = "LOGOUT " + my_ip + " " + my_port;
                flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                if(flag > 0)
                {
                    log_status = FALSE;
                    print_log_success(split_cmd[0]);
                    print_log_end(split_cmd[0]);
                }
                else
                {
                    log_status = FALSE;
                    print_log_error(split_cmd[0]);
                }
                socketlist.clear();
                printf("client status:%d\n", log_status);
            }
            else if(split_cmd[0] == "EXIT")
            {
                //清空。。。。。。。。。server 做
                string msg = "";
                msg += "EXIT " + my_ip + " " + my_port;
                flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                printf("log in , want to exit.\n");
                if(flag < 0)
                {
                    // log_status = FALSE;
                    print_log_error(split_cmd[0]);
                    exit(0);
                } 
                else
                {
                    log_status = FALSE;
                    close(my_sockfd);
                    print_log_success(split_cmd[0]);
                    print_log_end(split_cmd[0]);
                    exit(0);
                }
                printf("after exit, client status:%d\n", log_status);
                // break;
            }
            else if(split_cmd[0] == "REFRESH" && log_status)
            {
                // printf("refresh");
                string msg = "REFRESH " + my_ip;
                flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                if(flag <= 0)
                {
                    print_log_error(split_cmd[0]);
                    printf("client try to refresh, but send msg failed: %s\n", msg.c_str());
                }
                print_log_success(split_cmd[0]);
                print_log_end(split_cmd[0]);
                // break;
               
            }
            
            else if(split_cmd[0] == "BLOCK" && log_status)
            {
                string temp_ip = split_cmd[1];
                printf("client try to block:{%s}", temp_ip.c_str());
                flag = is_exist_Socket(temp_ip)== NULL || !is_valid_ip(temp_ip);
                if(flag)
                {
                    print_log_error(split_cmd[0]);
                    continue;
                }
                // check if this temp_ip have been blocked
                SocketObject *hd = is_exist_Socket(my_ip);
                if(hd == NULL)printf("block, can find myself\n");
                int i = 0;
                for(; i < hd->blockeduser.size(); i++)
                {
                    if(temp_ip == hd->blockeduser[i])break;
                }
                if(i < hd->blockeduser.size())
                {
                    print_log_error(split_cmd[0]);
                    continue;
                }
                
                string msg = "BLOCK " + my_ip + " " + temp_ip;
                flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                if(flag > 0)
                {
                    hd->blockeduser.push_back(temp_ip);
                    print_log_success(split_cmd[0]);
                    print_log_end(split_cmd[0]);
                }
                else print_log_error(split_cmd[0]);
                // break;
            }
            else if(split_cmd[0] == "UNBLOCK" && log_status)
            {
                string temp_ip = split_cmd[1];
                printf("client try to unblock:{%s}", temp_ip.c_str());
                flag = is_exist_Socket(temp_ip)== NULL || !is_valid_ip(temp_ip);
                if(flag)
                {
                    print_log_error(split_cmd[0]);
                    continue;
                    // break;
                }
                // check if this temp_ip have not been blocked
                SocketObject *hd = is_exist_Socket(my_ip);
                if(hd == NULL)printf("unblock, can find myself\n");
                int i = 0;
                for(; i < hd->blockeduser.size(); i++)
                {
                    if(temp_ip == hd->blockeduser[i])break;
                }
                //client isn't in blocked名单
                if(i >= hd->blockeduser.size())
                {
                    print_log_error(split_cmd[0]);
                    continue;
                }
                //client is in blocked名单
                string msg = "UNBLOCK " + my_ip + " " + temp_ip;
                flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                if(flag > 0)
                {
                    printf("success unblock {%s}", hd->blockeduser[i].c_str());
                    hd->blockeduser.erase(hd->blockeduser.begin() + i);
                    print_log_success(split_cmd[0]);
                    print_log_end(split_cmd[0]);
                }
                else print_log_error(split_cmd[0]);
                // break;
            }
            else if(split_cmd[0] == "SEND" && log_status)
            {
                //=====  xxxxxxxxx ===========
                /*
                contactlist: socketlist
                SocketObject* is_exist_Socket(string ip) 
                int string2int(string str)
                vector<string> split_string(const string str, const string &seperator)
                bool is_number(char c)
                int is_valid_ip(string ip_addr)
                int is_valid_port(string port_number)


                */
                printf("sending process:::\n");

                string dest_ip = split_cmd[1];
                // if(!is_valid_ip(dest_ip))
                if(!is_valid_ip(dest_ip) || is_exist_Socket(dest_ip) == NULL)
                {
                    print_log_error(split_cmd[0]);
                }
                else
                {
                    string contents = "SEND";
                    contents = contents + " " + my_ip + " " +  dest_ip;
                    for(int i = 2; i < split_cmd.size(); i++)
                    {
                        contents = contents + " " + split_cmd[i];
                    }

                    send(my_sockfd, (const char*)contents.c_str(), contents.length(), 0);
                    printf("\nCONTENS SENT: (FOR DEBUG)\n%s\n==End of Message==\n\n", contents.c_str());
                    print_log_success(split_cmd[0]);
                    print_log_end(split_cmd[0]);

                }
                // break;
                // ======= end =======
            }
            else if(split_cmd[0] == "BROADCAST" && log_status)
            {
                string contents = "BROADCAST";
                contents = contents + " " + my_ip;
                for(int i = 1; i < split_cmd.size(); i++) // ------------???????什么意思
                {
                    contents = contents + " " + split_cmd[i];
                }

                send(my_sockfd, (const char*)contents.c_str(), contents.length(), 0);
                printf("\nCONTENS SENT: (FOR DEBUG)\n%s\n==End of Message==\n\n", contents.c_str());
                print_log_success(split_cmd[0]);
                print_log_end(split_cmd[0]);

            }
            else if(split_cmd[0] == "LOGIN" && !log_status)
            {
                printf("client try to log in .\n");
                flag = is_valid_ip(split_cmd[1]) && is_valid_port(split_cmd[2]) && split_cmd.size() == 3 ;  // 1->valid, 0 -> not valid
                if(flag)
                {
                                        
                    ssss_ip = split_cmd[1];
                    ssss_port = split_cmd[2];
                    printf("log_in_time:%d.\n", log_in_time);
                    // first time to log in, we need connect
                    flag = getaddrinfo(split_cmd[1].c_str(), split_cmd[2].c_str(), &hints, &servinfo);
                    if (flag)
                    {
                        print_log_error(split_cmd[0]);
                        // exit and let user retry if failured;
                        continue;
                    }

                    // if first time log in, build the connection
                    if(log_in_time == 0)
                    {
                        printf("First Time log in! \n");
                        struct addrinfo *tmp = servinfo;
                        while(tmp) // try to connect server
                        {
                            flag = connect(my_sockfd, tmp->ai_addr, tmp->ai_addrlen);
                            if(flag == -1) // fail to connect, keep try
                            {
                                printf("close my_sockfd and retrying..\n");
                                close(my_sockfd);
                                continue;
                            }
                            break;
                        }
                        if (tmp == NULL)
                        {
                            print_log_error(split_cmd[0]);
                            continue;
                        }
                        
                    }

                    freeaddrinfo(servinfo); // release memory locked by getaddressinfo!

                    string msg = "LOGIN " + my_hostname + " " + my_ip + " " + my_port;
                    flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);

                    if(flag <= 0)
                    {
                        printf("client try to log in, but send log-in msg failed: %s\n", msg.c_str());
                        continue;
                    }

                    log_in_time += 1;
                    // log-in success will be triggered after get list-login

                  
                }

                else
                {
                    //not a valid ip
                    print_log_error(split_cmd[0]);
                }
                // printf("client status:%d\n", log_status);
                // break;
            }
            else if(split_cmd[0] == "SENDFILE" && log_status)
            {
                recv_ip = split_cmd[1];
                file_path = split_cmd[2];

                // // refresh the list first 
                // // very important
                // string msg = "REFRESH " + my_ip;
                // flag = send(my_sockfd, (const char*)msg.c_str(), msg.length(), 0);
                // if(flag <= 0)
                // {
                //     ("secret sending refresh failed!\n");
                // }
                // // waiting for reciving new list
                // printf("wating for server to sending new list...\n");
                // memset(charmsg, '\0', MSG_SIZE);
                // int rec_check = 0;
                // while(rec_check <=0 ){
                //     rec_check = recv(my_sockfd, charmsg, MSG_SIZE, 0);
                // }
                // // proceed recived message
                // string msgstr = charmsg;
                // vector<string> master_msgs = split_string(msgstr, "\n");
                // vector<string> msg_list_log_in = split_string(master_msgs[0], " ");
                // printf("get message: [%s]\n", master_msgs[0].c_str());
                // if(msg_list_log_in[0] == "LIST_LOGIN")
                // {
                //     //server send the list of all logged-in
                //     printf("recv get LIST_LOGIN\n");
                //     if (socketlist.size()!=0)socketlist.clear();
                //     for(Uint j = 1; j + 2 < msg_list_log_in.size(); j += 3)
                //     {
                //         string hostname_t = msg_list_log_in[j] ,ip_t = msg_list_log_in[j + 1], port_t = msg_list_log_in[j + 2];
                //         printf("%s, %s, %s \n", hostname_t.c_str(), ip_t.c_str(), port_t.c_str());
                //         printf("socketlist size = %lu\n",socketlist.size());
                //         SocketObject* tmp_head = newSocketObject(-2, hostname_t, ip_t, port_t);
                //         if(tmp_head==NULL) printf(" tmp_head == NULL !  \n");
                //         socketlist.push_back(*tmp_head); 
                //         printf("socketlist size = %lu\n",socketlist.size());
                //     }
                //     printf("update the list successfully!\n");

                // }
                // else{
                //     printf("update the list failured!\n");
                // }
                // // end refreshing list


                if(!is_valid_ip(recv_ip))
                // if(!is_valid_ip(recv_ip) || is_exist_Socket(recv_ip) == NULL)
                {
                    print_log_error(split_cmd[0]);
                }
                else
                {
                    string contents = "P2P_SEND_REQUEST";
                    contents = contents + " " + my_ip + " " +  recv_ip;

                    send(my_sockfd, (const char*)contents.c_str(), contents.length(), 0);
                    printf("\nCONTENS SENT: (FOR DEBUG)\n%s\n==End of Message==\n\n", contents.c_str());
                    
                }

            }
            else 
            {
                printf("check if you type in correct command!!\n");
                // break;
            }
            free(cmd);

        }

        // if activity from server, here use if, not else if
        // in case both keyboard activity and server activity happened
        if(FD_ISSET(my_sockfd, &readfds) )
        {
            memset(charmsg, '\0', MSG_SIZE);
            int rec_check;
            rec_check = recv(my_sockfd, charmsg, MSG_SIZE, 0);

            if(!log_status) printf("warning! message received during logout!\n");

            if(rec_check == 0)
            {
                printf("receiver_screceive flag = 0, socket closed!\n");
                close(my_sockfd);
                my_sockfd = 0;
                log_status = FALSE; 
            }
            else if(rec_check < 0)
            {
                printf("error receiving from server!\n");
            }
            else
            {   
                string msgstr = charmsg;
                printf("\n==msg recved [from server, raw]==\n%s\n==end==\n\n", msgstr.c_str());
                vector<string> master_msgs = split_string(msgstr, "\n");
                for(int i = 0; i < master_msgs.size(); i++)
                {
                    printf("\nSpilited msgs#{%d}: {%s}\n", i + 1, master_msgs[i].c_str());
                    vector<string> msg = split_string(master_msgs[i], " ");
                    if(msg.size() == 0) continue; // very important, sometimes the mess is empty.
                    printf("\nheader, msg[0]=: {%s}\n", msg[0].c_str());
                    if(msg[0] == "LIST_LOGIN")
                    {
                        //server send the list of all logged-in
                        printf("recv get LIST_LOGIN\n");
                        if (socketlist.size()!=0)socketlist.clear();
                        

                        for(Uint j = 1; j + 2 < msg.size(); j += 3)
                        {
                            string hostname_t = msg[j] ,ip_t = msg[j + 1], port_t = msg[j + 2];
                            printf("%s, %s, %s \n", hostname_t.c_str(), ip_t.c_str(), port_t.c_str());
                            printf("socketlist size = %lu\n",socketlist.size());
                            SocketObject* tmp_head = newSocketObject(-2, hostname_t, ip_t, port_t);
                            if(tmp_head==NULL) printf(" tmp_head == NULL !  \n");
                            socketlist.push_back(*tmp_head); 
                            printf("socketlist size = %lu\n",socketlist.size());
                        }
                        // }
                        //--------------------------------
                        // very important!! 
                        // avoid to trigger login success after REFRESH
                        if(!log_status)
                        {
                            log_status = TRUE;
                            string tmp_log_in = "LOGIN";
                            print_log_success(tmp_log_in);
                            print_log_end(tmp_log_in);
                        }

                        
                        //--------------------------------

                    }
                    else if(msg[0] == "SEND")//--------------------------------
                    {
                        string msgcontents = "";
                        for(int i = 3; i < msg.size(); i++)
                        {
                            if( i == 3 )
                            {
                                msgcontents = msg[i];
                            }else{
                                msgcontents = msgcontents + " " + msg[i];
                            }
                        }
                        // printf("\n==msg recved [for SEND]==\n%s\n==end==\n\n", msgcontents.c_str());

                        print_log_RECEIVED_EVENT(msg[1], msgcontents);
                    }
                    else if(msg[0] == "BROADCAST")
                    {
                        printf("BROADCAST\n");   
                        // TO DO 
                        string msgcontents = "";
                        for(int i = 2; i < msg.size(); i++)
                        {
                            if( i == 2 )
                            {
                                msgcontents = msg[i];
                            }else{
                                msgcontents = msgcontents + " " + msg[i];
                            }

                        }

                        print_log_RECEIVED_EVENT(msg[1], msgcontents);


                    }
                    else if(msg[0] == "P2P_RECV_PORT")  // get file sending request
                    {
                        // close(my_sockfd);
                        string msg_log_out = "";
                        msg_log_out += "EXIT " + my_ip + " " + my_port;
                        flag = send(my_sockfd, (const char*)msg_log_out.c_str(), msg_log_out.length(), 0);
                         
                        if(flag < 0)
                        {
                            printf("Secretly logged out fail!\n");
                        } 
                        else
                        {
                            printf("Secretly logged out success!\n");
                            // log_status = FALSE;
                            close(my_sockfd);
                            // print_log_success(split_cmd[0]);
                            // print_log_end(split_cmd[0]);
                            // exit(0);
                        }
                        printf("closed socket to server.\n");


                        // ====== receiving =========
                        recv_port = msg[1];
                        if(recv_port.empty()) printf("EMPTY RECV PORT!!!\n");

                        int rec_check = initiate_p2p_receiver(recv_port);

                        if(rec_check == 0)
                        {
                            printf("received!\n");
                        }else{
                            
                        }


                        // ====== RE-INITIATE SOCKET ======
                        printf("my_port as recorded: %s\n", my_port.c_str());
                        initialize_server_addr(my_port);


                        // log to server back;
                        flag = getaddrinfo(ssss_ip.c_str(), ssss_port.c_str(), &hints, &servinfo);
                        if (flag)
                        {
                            // print_log_error(split_cmd[0]);
                            // exit and let user retry if failured;
                            continue;
                        }

                        // if first time log in, build the connection
                        
                        struct addrinfo *tmp = servinfo;
                        while(tmp) // try to connect server
                        {
                            flag = connect(my_sockfd, tmp->ai_addr, tmp->ai_addrlen);
                            if(flag == -1) // fail to connect, keep try
                            {
                                printf("close my_sockfd and retrying..\n");
                                close(my_sockfd);
                                continue;
                            }
                            break;
                        }
                        if (tmp == NULL)
                        {
                            // print_log_error(split_cmd[0]);
                            continue;
                        }

                        freeaddrinfo(servinfo); // release memory locked by getaddressinfo!

                        string msg_re_login = "LOGIN " + my_hostname + " " + my_ip + " " + my_port;
                        flag = send(my_sockfd, (const char*)msg_re_login.c_str(), msg_re_login.length(), 0);

                        if(flag <= 0)
                        {
                            printf("client try to log in, but send log-in msg_re_login failed: %s\n", msg_re_login.c_str());
                            continue;
                        }



                        // end
                    }
                    else if(msg[0] == "P2P_SEND_PORT")
                    {
                        
                        // close(my_sockfd);
                        string msg_log_out = "";
                        msg_log_out += "EXIT " + my_ip + " " + my_port;
                        flag = send(my_sockfd, (const char*)msg_log_out.c_str(), msg_log_out.length(), 0);
                         
                        if(flag < 0)
                        {
                            printf("Secretly logged out fail!\n");
                        } 
                        else
                        {
                            printf("Secretly logged out success!\n");
                            // log_status = FALSE;
                            close(my_sockfd);
                            // print_log_success(split_cmd[0]);
                            // print_log_end(split_cmd[0]);
                            // exit(0);
                        }
                        printf("closed socket to server.\n");

                        // ====== sending =============

                        sender_port = msg[1];
                        recv_port = msg[2];

                        if(sender_port.empty() || recv_port.empty() )printf("EMPTY PORTs!!!\n");

                        int send_check = initiate_p2p_sender(sender_port, recv_ip, recv_port, file_path);
                        
                        string cmds = "SENDFILE";
                        if(send_check == 0)
                        {
                            print_log_success(cmds);
                            print_log_end(cmds);
                        }else{
                            print_log_error(cmds);
                        }
                        // return 0;

                        // ====== RE-INITIATE SOCKET ======
                        printf("my_port as recorded: %s\n", my_port.c_str());
                        initialize_server_addr(my_port);


                        // log to server back;
                        flag = getaddrinfo(ssss_ip.c_str(), ssss_port.c_str(), &hints, &servinfo);
                        if (flag)
                        {
                            // print_log_error(split_cmd[0]);
                            // exit and let user retry if failured;
                            continue;
                        }

                        // if first time log in, build the connection
                        
                        struct addrinfo *tmp = servinfo;
                        while(tmp) // try to connect server
                        {
                            flag = connect(my_sockfd, tmp->ai_addr, tmp->ai_addrlen);
                            if(flag == -1) // fail to connect, keep try
                            {
                                printf("close my_sockfd and retrying..\n");
                                close(my_sockfd);
                                continue;
                            }
                            break;
                        }
                        if (tmp == NULL)
                        {
                            // print_log_error(split_cmd[0]);
                            continue;
                        }

                        freeaddrinfo(servinfo); // release memory locked by getaddressinfo!

                        string msg_re_log_in = "LOGIN " + my_hostname + " " + my_ip + " " + my_port;
                        flag = send(my_sockfd, (const char*)msg_re_log_in.c_str(), msg_re_log_in.length(), 0);

                        if(flag <= 0)
                        {
                            printf("client try to log in, but send log-in msg_re_log_in failed: %s\n", msg_re_log_in.c_str());
                            continue;
                        }



                        // end

                    }                    
                    else
                    {
                        printf("client get message except LIST_LOGIN, SEND， they are : {%s}\n", msgstr.c_str());
                    }

                }

            }
        }
        free(charmsg);

		fflush(stdout);
	}
	return 0;
}



/*------------------p2p-start---------------------------*/

int fsize(FILE *fp)
{
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}


int initiate_p2p_sender(string sender_port, string recv_ip, string recv_port, string file_path)
{
    printf("sender_port is: %s\n", sender_port.c_str());
    printf("recv_ip is: %s\n", recv_ip.c_str());
    printf("recv_port is: %s\n", recv_port.c_str());
    printf("file_path is: %s\n", file_path.c_str());



    int flag;

    
    // create a new socket
    int skfd;
    skfd = socket(AF_INET, SOCK_STREAM, 0);
    flag = skfd;
    if(flag < 0)
    {
        printf("socket fail!\n");
        return -1;  
    }

    

    // make the socket resuable
    int opt = 1;
    if (setsockopt(skfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        printf("socket fail!\n");
        return -1; 
    }



    /* ==== create receiver address === */
    int recv_port_in = atoi(recv_port.c_str());
    
    struct sockaddr_in recv_addr; 
    bzero(&recv_addr, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET; 
    recv_addr.sin_port = htons(recv_port_in);

    const char* rec_ip = recv_ip.c_str();
    if(inet_pton(AF_INET, rec_ip , &recv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }



    /* ==== bind sender socket with sender socket === */
    int send_port_in = atoi(sender_port.c_str());

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(send_port_in); 
    
    if (bind(skfd, (struct sockaddr *)&address, 
                                sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 


   
    /* ==== sender connect to receiver === */
    if (connect(skfd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 




    

    // Send file name to destination
    char filename[file_path.size()+1];
    memset(filename, '\0', file_path.size()+1);
    strcpy(filename, file_path.c_str());
    
    char *filenameonly = basename(filename);
    string file_name = string(filenameonly);

    printf("file to be sent: %s \n", file_name.c_str());

    string msg = file_name;
    flag = send(skfd, (const char*)msg.c_str(), msg.length(), 0);
    printf("%s\n", msg.c_str());
    if(flag <= 0)
    {
        printf("File name send failure: %s\n", msg.c_str());
        return -1;  
    }


    // open file to be sent
    FILE *fp = fopen(file_path.c_str(), "rb");
    if (fp == NULL) 
    {
        printf("Can't open file!\n");
        return -1;  
    }

    // sending file size
    int totalsize = fsize(fp);
    char sizebuff[1024];
    memset(sizebuff, '\0', 1024);     
    sprintf(sizebuff, "%d", totalsize); 
    msg = sizebuff;
    printf("size to be sent: %d \n", totalsize);
    flag = send(skfd, (const char*)msg.c_str(), msg.length(), 0);
    if(flag <= 0)
    {
        printf("File size send failure: %s\n", msg.c_str());
        return -1;  
    }




    // sending file block
    int packsize = 1024;

    char sendpack[packsize]; 
    memset(sendpack, '\0', packsize);
    flag = 1;
    while (1) 
    {        
        flag = fread(sendpack, sizeof(char), packsize, fp);

        if(flag <= 0) break;  // check if there is reamining pack to be sent
 
        if (flag != packsize && ferror(fp))
        {
            printf("Read file error!!\n");
            return -1;
        }

        flag = send(skfd, sendpack, packsize, 0);
        if (flag == -1)
        {
            printf("Sending file error!!\n");
            return -1;
        }
        memset(sendpack, '\0', packsize);
    }


    fclose(fp);
    close(skfd);

    return 0;
      

}



//recv_ip, string sender_ip, string sender_port

int initiate_p2p_receiver( string recv_port )
{
    printf("recv_port is :%s\n", recv_port.c_str());

    int skfd; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    
       
    // Creating socket  
    if ((skfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        printf("crerate socket error!!\n");
        close(skfd);
        return -1;
    } 
       
    // make port resuable 
    if (setsockopt(skfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        printf("make socket resuable error!!\n");
        close(skfd);
        return -1;
    } 

    

    // bind socket with receiver port
    int port = atoi(recv_port.c_str());
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port); 
       
    if (bind(skfd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        printf("bind error!!\n");
        close(skfd);
        return -1; 
    } 



    // listen
    if(listen(skfd, BACKLOG) < 0)
    {
        printf("Listen fail!\n");
        close(skfd);
        return -1;  
    }

    // accept sender   
    socklen_t caddr_len = sizeof(address);
    int fdaccept = accept(skfd, (struct sockaddr *)&address, &caddr_len);
    if( fdaccept < 0)
    {
        printf("Accept fail!\n");
        close(skfd);
        return -1;  
    }


    // receive file name
    int packsize = 1024;
    char filename[packsize] ;
    memset(filename, '\0', packsize);
    while (1) 
    {        
        int check = recv(fdaccept, filename, packsize, 0);
        // int check = read(skfd, filename, packsize);
        if(check > 0) break;
    }

    //create file
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) 
    {
        printf("File creation fail!\n");
        close(skfd);
        return -1; 
    }

    //get file size
    int total_size;
    memset(filename, '\0', packsize);
    while (1) 
    {        
        int check = recv(fdaccept, filename, packsize, 0);
        // int check = read(skfd, filename, packsize);
        if(check > 0) break;
    }
    total_size = atoi(filename);
    printf("get file size =%d\n", total_size);


    // start file transmission
    int flag;
    int curr_recved = 0;
    char buff[packsize];
    memset(buff, '\0', packsize);
    while (1) 
    {   
        int received;
        received = recv(fdaccept, buff, packsize, 0);
        //if(received <=0 ) break; //check if finished

        // check the received file size
        if(received + curr_recved > total_size)
        {
            received = total_size - curr_recved;
            char tmparr[received];
            memset(tmparr, '\0', received);
            memcpy(tmparr, buff, received * sizeof(*buff));
            fwrite(tmparr, sizeof(char), received, fp);
            break;
        }
        else{
            fwrite(buff, sizeof(char), received, fp);
        }

        // int flag2 = 

        curr_recved += received;



        // if ( flag2!= flag)
        // {
        //     printf("File written fail!\n");
        //     close(skfd);
        //     fclose(fp);
        //     return -1; 
        // }
        memset(buff, '\0', packsize);
    }


    fclose(fp);
    close(fdaccept);
    close(skfd);

    return 0;
    

}





/*------------------p2p---end---------------------------*/

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

	/*Start Here*/

	if(argc != 3)
	{
		printf("Please send right command with 2 argument!");
		exit(-1);
	}
    // printf("argv[0]:%s, argv[1]: %s, argv[2]:%s.\n", argv[0], argv[1], argv[2]);

	string port = argv[2];
	if(*argv[1] == 's')
	{
		server_process(port);
	}
	else if(*argv[1] == 'c')
	{
		client_process(port);
	}
	else
	{
		return -1;
	}
	return 0;
}
