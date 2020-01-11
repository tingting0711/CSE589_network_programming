#include "../include/simulator.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
using namespace std;
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define iter vector<pkt>::iterator

float RTT = 20.0; 
int WINDOWSIZE = 1;
int acknum, seqnum, b_nextSeqnum;
vector<pkt> waitingPacketlist, sendingPacketlist;


int check_sum(struct pkt* packet)
{
	int sum = 0, i = 0;
	sum += packet->acknum + packet->seqnum;
	while(i < 20)
	{
		sum += packet->payload[i ++];
	}
	return sum;
}
struct pkt* new_packet(int ack, int seq, char* data)
{
	struct pkt* packet = new pkt();
	int tmp_ack = ack, tmp_seq = seq;
	packet->seqnum = tmp_seq;
	packet->acknum = tmp_ack;
	memcpy(packet->payload, data, 20);
	packet->checksum = check_sum(packet);
	return packet;
}
struct pkt* make_ack_packet(int seqnum)
{
	struct pkt* packet = new pkt();
	packet->seqnum = seqnum;
	packet->acknum = 1;
	memset(packet->payload, 0, sizeof(packet->payload));
	packet->checksum = check_sum(packet);
	return packet;
}

int send_packet(struct pkt* packet)
{
	tolayer3(0, *packet);
	starttimer(0, RTT);
	return 0;
}

int send_next_packet()
{
	struct pkt packet = waitingPacketlist.front();
	sendingPacketlist.push_back(packet);
	send_packet(& packet);
	waitingPacketlist.erase(waitingPacketlist.begin());
	return 0;
}

int remove_send_packet(int seqnum)
{
	iter it;
	it = sendingPacketlist.begin();
	while(it < sendingPacketlist.end())
	{
		if(it->seqnum == seqnum)
		{
			sendingPacketlist.erase(it);
			return 0;
		}
		it ++;
	}
	return 0;
}

struct pkt* in_sending_packet(int seqnum)
{
	int i = 0;
	while(i < sendingPacketlist.size())
	{
		if(sendingPacketlist[i].seqnum == seqnum)
		{
			pkt* packet = & sendingPacketlist[i];
			return packet;
		}
		i ++;
	}
	struct pkt* temp = new pkt();
	printf("something wrong in in_sending_packet, can't find this seqnum:%d\n", seqnum);
	return temp;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	struct pkt* packet = new_packet(acknum, seqnum, message.data);
	waitingPacketlist.push_back(*packet);
	seqnum += 1;
	bool flag1 = sendingPacketlist.size() < WINDOWSIZE;
	bool flag2 = waitingPacketlist.size() > 0;
	if(flag1 && flag2)
		send_next_packet();
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	struct pkt* pack = in_sending_packet(packet.seqnum);
	bool flag1 = packet.checksum == check_sum(&packet);
	bool flag2 = packet.acknum == 1;
	if(flag1 && flag2 && pack)
	{
		stoptimer(0);
		remove_send_packet(packet.seqnum);
		flag1 = sendingPacketlist.size() < WINDOWSIZE;
		flag2 = waitingPacketlist.size() > 0;
		if(flag1 && flag2)
		{
			send_next_packet();
		}

	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	// struct pkt packet = sendingPacketlist.front();
	send_packet(&sendingPacketlist.front());
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	sendingPacketlist.clear();
	waitingPacketlist.clear();
	seqnum = 0;
	acknum = seqnum + 1; //acknum = 1;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	bool flag = packet.checksum == check_sum(&packet);
	if(!flag) return;

	struct pkt* ackPacket = make_ack_packet(packet.seqnum);
	tolayer3(1, *ackPacket);
	if(packet.seqnum == b_nextSeqnum)
	{
		// tolayer3(1, *ackPacket);
		tolayer5(1, packet.payload);
		b_nextSeqnum = packet.seqnum + 1;
		return;
	}
	// else
	// {
	// 	tolayer3(1, *ackPacket);
	// 	return;
	// }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	b_nextSeqnum = 0;
}
