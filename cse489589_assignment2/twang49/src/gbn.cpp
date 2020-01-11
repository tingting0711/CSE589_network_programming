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

/* called from layer 5, passed the data to be sent to other side */

struct time_pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
   float time;
};

#define iter vector<pkt>::iterator
#define time_iter vector<time_pkt>::iterator

float delay = 1, RTT = 35.0; 
int WINDOWSIZE = 1;
int acknum, seqnum, b_nextSeqnum, a_nextSeqnum;
vector<pkt> waitingPacketlist;
vector<time_pkt> timePacketlist, ackTimePacketlist;

int check_sum(struct pkt* packet, bool flag=false)
{
	int sum = 0, i = 0;
	sum += packet->acknum + packet->seqnum;
	if (flag) return sum;
	while(i < 20)
	{
		sum += packet->payload[i ++];
	}
	return sum;
}
// new_packet(acknum, seqnum, message.data);
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

int send_packet(struct pkt* packet)
{
	tolayer3(0, *packet);
	return 0;
}

struct time_pkt* to_timepacket(struct pkt* packet)
{
	struct time_pkt* timePacket = new time_pkt();
	int temp_seqnum = packet->seqnum, temp_acknum = packet->acknum;
	timePacket->acknum = temp_acknum;
	timePacket->seqnum = temp_seqnum;
	memcpy(timePacket->payload, packet->payload, 20);
	timePacket->checksum = packet->checksum;
	timePacket->time = get_sim_time() + RTT;
	return timePacket;

}

struct pkt* to_packet(struct time_pkt* timePacket)
{
	struct pkt* packet = new pkt();
	int temp_seqnum = timePacket->seqnum, temp_acknum = timePacket->acknum;
	packet->acknum = temp_acknum;
	packet->seqnum = temp_seqnum;
	memcpy(packet->payload, timePacket->payload, 20);
	packet->checksum = timePacket->checksum;
	return packet;
}

float get_next_timetick()
{
	float now = get_sim_time();
	float min_tick = timePacketlist.begin()->time;
	float res = min_tick - now;
	return res;
}

int update_timer()
{
	if(timePacketlist.empty())return -1;
	starttimer(0, get_next_timetick());
	return 0;
}

int send_next_packet()
{
	// struct pkt packet = waitingPacketlist.front();
	struct pkt packet = waitingPacketlist.back();
	struct time_pkt* timePacket = to_timepacket(&packet);

	timePacketlist.push_back(*timePacket);
	send_packet(&packet);

	if(timePacketlist.size() == 1)starttimer(0, RTT);

	// waitingPacketlist.erase(waitingPacketlist.begin());
	waitingPacketlist.pop_back();
	return 0;
}

struct pkt* make_ack_packet(int seqnum)
{
	struct pkt* packet = new pkt();
	packet->seqnum = seqnum;
	packet->acknum = 1;
	memset(packet->payload, 0, sizeof(packet->payload));
	packet->checksum = seqnum + 1;
	return packet;
}

int remove_send_packet(int seqnum)
{
	time_iter it;
	it = timePacketlist.begin();
	while(it < timePacketlist.end())
	{
		if(it->seqnum == seqnum)
		{
			timePacketlist.erase(it);
			return 0;
		}
		it ++;
	}
	return 0;
}

int remove_ack_send_packet(int seqnum)
{
	time_iter it;
	it = ackTimePacketlist.begin();
	while(it < ackTimePacketlist.end())
	{
		if(it->seqnum == seqnum)
		{
			ackTimePacketlist.erase(it);
			return 0;
		}
		it ++;
	}
	return 0;
}

struct time_pkt* in_sending_time_packet(int seqnum)
{
	int i = 0;
	while(i < timePacketlist.size())
	{
		if(timePacketlist[i].seqnum == seqnum)
		{
			time_pkt* packet = & timePacketlist[i];
			return packet;
		}
		i ++;
	}
	printf("something wrong in in_sending_packet, can't find this seqnum:%d\n", seqnum);
	return NULL;
}

struct time_pkt* in_ack_sending_packet(int seqnum)
{
	int i = 0;
	while(i < ackTimePacketlist.size())
	{
		if(ackTimePacketlist[i].seqnum == seqnum)
		{
			time_pkt* timePacket = & ackTimePacketlist[i];
			return timePacket;
		}
		i++;
	}
	return NULL;
}


void A_output(struct msg message)
{
	struct pkt* packet = new_packet(acknum, seqnum, message.data);
	// waitingPacketlist.push_back(*packet);
	waitingPacketlist.insert(waitingPacketlist.begin(), *packet);
	seqnum += 1;
	bool flag1 = timePacketlist.size() < WINDOWSIZE;
	bool flag2 = waitingPacketlist.size() > 0;
	if(flag1 && flag2)
	{
		send_next_packet();
	}

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	struct time_pkt* timePacket = in_sending_time_packet(packet.seqnum);
	float time = get_sim_time();
	bool flag =  packet.acknum && timePacket && packet.seqnum >= a_nextSeqnum && packet.checksum == (packet.seqnum + packet.acknum);
	if(timePacket!= NULL && packet.checksum == check_sum(&packet, true))
	{
		bool flag1 = time - timePacket->time + RTT < delay;
		if(flag1)return;

		bool flag2 = in_ack_sending_packet(packet.seqnum)!= NULL;
		if(flag2)return;

		// if(packet.seqnum < a_nextSeqnum)return;

		ackTimePacketlist.push_back(*timePacket);

		if(packet.seqnum == a_nextSeqnum)
		{
			stoptimer(0);
			while(ackTimePacketlist.size() != 0)
			{
				if(ackTimePacketlist.begin()->seqnum == a_nextSeqnum)
				{
					remove_send_packet(a_nextSeqnum);
					remove_ack_send_packet(a_nextSeqnum);
					a_nextSeqnum += 1;
				}
				else break;
			}
			while(timePacketlist.size() < WINDOWSIZE && waitingPacketlist.size() > 0)
			{
				send_next_packet();
			}
			update_timer();
			return;
		}

	}
	return;

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	ackTimePacketlist.clear();
	time_iter it = timePacketlist.begin();
	float eps = 0;
	while(it < timePacketlist.end())
	{
		struct time_pkt* timePacket = in_sending_time_packet(it->seqnum);
		timePacket->time = get_sim_time() + RTT + eps * 2;
		struct pkt* packet = to_packet(timePacket);
		send_packet(packet);
		it ++;
		eps += 1;
	}
	starttimer(0,RTT);//miaomiaomiao??

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	WINDOWSIZE = getwinsize();
	seqnum = acknum = a_nextSeqnum = 0;
	waitingPacketlist.clear();
	timePacketlist.clear();
	ackTimePacketlist.clear();
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
		tolayer5(1, packet.payload);
		b_nextSeqnum += 1;
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	b_nextSeqnum = 0;
}
