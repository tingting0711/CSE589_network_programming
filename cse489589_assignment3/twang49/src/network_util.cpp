// *
//  * @network_util
//  * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
//  * @version 1.0
//  *
//  * @section LICENSE
//  *
//  * This program is free software; you can redistribute it and/or
//  * modify it under the terms of the GNU General Public License as
//  * published by the Free Software Foundation; either version 2 of
//  * the License, or (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful, but
//  * WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//  * General Public License for more details at
//  * http://www.gnu.org/copyleft/gpl.html
//  *
//  * @section DESCRIPTION
//  *
//  * Network I/O utility functions. send/recvALL are simple wrappers for
//  * the underlying send() and recv() system calls to ensure nbytes are always
//  * sent/received.
 

#include <stdlib.h>
#include <sys/socket.h>

#include <cstdio>
#include <algorithm>
#include <iostream>

#include <../include/network_util.h>

using namespace std;

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
    cout<<"[recvALL] Start"<<endl;
    cout<<"[nbytes]"<<nbytes<<endl;
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);

    cout<<"[bytes]"<<bytes<<endl;
    cout<<"[recvALL] End"<<endl;
    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}