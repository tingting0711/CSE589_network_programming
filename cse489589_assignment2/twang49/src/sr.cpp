#include "../include/simulator.h"

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

#include "../include/simulator.h"
#include <numeric>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
using namespace std;


int nextseqnumber;
int acknumber;
int window_n;

// to send list
vector<pkt> tosend_pkt_list;
struct pkt_time : public pkt
{	
	public:
		// log time when sent
		float stime; 
};
// sent, but unacked list
vector<pkt_time> unacked_pkttm_list;

// received ack mesg
vector<pkt> recv_pkt_list;
int recv_seqnum;



float RTT = 20.0;

 

int updatetimer()
{

	////////////////////modified 1 //////////////////////////////
	if(unacked_pkttm_list.empty())return 0;

	float curr_time = get_sim_time();
    float earliest_expected = unacked_pkttm_list.begin()->stime;
    for (int i = 0; i < unacked_pkttm_list.size(); i++)
    {
        struct pkt_time* tmp = &unacked_pkttm_list[i];
        if (tmp->stime < earliest_expected)
        {
            earliest_expected = tmp->stime;
        }
    }


	starttimer(0, earliest_expected - curr_time);
	return 0;
}



int check_sum(struct pkt* packet)
{
	/*
		compute checksum
	*/
	int sum = 0;
	sum += packet->seqnum;
	sum += packet->acknum;
	int i = 0;
	while(i<20){
		sum += packet->payload[i];
		i++;
	}
	return sum;
}


// static pkt* create_pkt(struct msg *message, int seqnum, int acknum)
// {
// 	/*                
// 	convert message to pkt with checksum info
// 	*/
// 	struct pkt* new_packet = new pkt();
// 	new_packet->seqnum = seqnum;
// 	new_packet->acknum = acknum;
// 	strncpy(new_packet->payload, message->data, 20);
// 	new_packet->checksum = check_sum(new_packet);
// 	return new_packet;
// }

pkt* create_pkt_char(char *message, int seqnum, int acknum)
{
	/*                
	convert message to pkt with checksum info
	*/
	struct pkt* new_packet = new pkt();
	new_packet->seqnum = seqnum;
	new_packet->acknum = acknum;
	memset(new_packet->payload, 0, 20);
	strncpy(new_packet->payload, message, 20);
	new_packet->checksum = check_sum(new_packet);
	return new_packet;
}

// pkt_time* create_pkt_time(struct pkt* pack)
// {
// 	struct pkt_time* pkt_with_timer = new pkt_time();
// 	pkt_with_timer->seqnum = pack->seqnum;
// 	pkt_with_timer->acknum = pack->acknum;
// 	pkt_with_timer->checksum = pack->checksum;
// 	strncpy(pkt_with_timer->payload, pack->payload, 20);
// 	pkt_with_timer->stime = get_sim_time()+RTT;
// 	return pkt_with_timer;
// }


pkt_time* create_pkt_time(char *message, int seqnum, int acknum, int checksum)
{
	struct pkt_time* pkt_with_timer = new pkt_time();
	pkt_with_timer->seqnum = seqnum;
	pkt_with_timer->acknum = acknum;
	pkt_with_timer->checksum = checksum;
	strncpy(pkt_with_timer->payload, message, 20);
	pkt_with_timer->stime = get_sim_time() + RTT;
	return pkt_with_timer;
}




void sendpkt()
{
	////////////////////////////////////////////////////////////////
	struct pkt nextmsg = tosend_pkt_list.back();
	struct pkt_time* nextmsgtime = create_pkt_time(nextmsg.payload, nextmsg.seqnum, nextmsg.acknum, nextmsg.checksum);
	
	tolayer3(0, nextmsg);


	if (unacked_pkttm_list.size() == 0)
	{
		/* if new sending, restart timer */
		starttimer(0, RTT);
	}
	unacked_pkttm_list.push_back(*nextmsgtime);
	tosend_pkt_list.pop_back();
}


bool check_in_unacked_sq(int sn)
{
	for(int i = 0; i < unacked_pkttm_list.size(); i++)
	{
		pkt_time* tmp = &unacked_pkttm_list[i];
		if(tmp->seqnum == sn)
		{
			return true;
		}
	}
	return false;
}

pkt_time* find_unacked_pkt(int sn)
{
	for(int i = 0; i < unacked_pkttm_list.size(); i++)
	{
		pkt_time* tmp = &unacked_pkttm_list[i];
		if(tmp->seqnum == sn)
		{
			return tmp;
		}
	}
	return NULL;
}

void remove_from_unackedlist(int sn)
{
	for(int i = 0; i < unacked_pkttm_list.size(); i++)
	{
		pkt_time* tmp = &unacked_pkttm_list[i];
		if(tmp->seqnum == sn)
		{
			unacked_pkttm_list.erase(unacked_pkttm_list.begin()+i);
			return;
		}
	}
	return;
}


////////////////////////////////////////////////////
///////////////below for receiver///////////////////
////////////////////////////////////////////////////


bool check_in_received_sq(int sn)
{
	for(int i = 0; i < recv_pkt_list.size(); i++)
	{
		pkt* tmp = &recv_pkt_list[i];
		if(tmp->seqnum == sn)
		{
			return true;
		}
	}
	return false;
}


struct pkt* search_in_received_sq(int sn)
{
	for(int i = 0; i < recv_pkt_list.size(); i++)
	{
		pkt* tmp = &recv_pkt_list[i];
		if(tmp->seqnum == sn)
		{
			return tmp;
		}
	}
	return NULL;
}



pkt* create_ACK_pkt(int seqnum, int acknum)
{
	////////////////////////////////////////////////////////////////
	// create_pkt_char(char *message, int seqnum, int acknum)
	char mess[20];
	memset(mess, 0, 20);
	struct pkt* ackpk = create_pkt_char(mess, seqnum, acknum); 
	ackpk->checksum = seqnum+acknum;
	return ackpk;
}


void remove_from_recvlist(int sn)
{
	for(int i = 0; i < recv_pkt_list.size(); i++)
	{
		pkt* tmp = &recv_pkt_list[i];
		if(tmp->seqnum == sn)
		{
			recv_pkt_list.erase(recv_pkt_list.begin()+i);
			return;
		}
	}
	return;
}





//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	tosend_pkt_list.insert(tosend_pkt_list.begin(),*create_pkt_char(message.data, nextseqnumber,acknumber));
	printf("[A - in] %d,%d,%.20s\n", tosend_pkt_list.begin()->seqnum, tosend_pkt_list.begin()->acknum, tosend_pkt_list.begin()->payload);
    nextseqnumber++;
    int unacked_num = unacked_pkttm_list.size();
    int tosend_num = tosend_pkt_list.size();
    cout<<"progress2"<<endl;
    if(unacked_num < window_n && tosend_num > 0)
    {
   	 	cout<<"progress2"<<endl;
   	 	sendpkt();
   	 	cout<<"progress3"<<endl;
    }
    cout<<"progress4"<<endl;

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
// bool check_in_unacked_sq(int sn)   //////////
// static pkt_time* find_unacked_pkt(int sn) //////
	cout<<"progress5"<<endl;
	bool check1 = check_in_unacked_sq(packet.seqnum);
	bool check2 = (packet.checksum == (packet.acknum+packet.seqnum));
	if( check1 && check2 )
	{
		stoptimer(0);
		remove_from_unackedlist(packet.seqnum);
		updatetimer();
		if(unacked_pkttm_list.size() < window_n && tosend_pkt_list.size() > 0)
		{
			sendpkt();
		}

	}
	cout<<"progress6"<<endl;

}


int find_earliest_unacked_seq()
{
	float earliest_senttime = unacked_pkttm_list.begin()->stime;
	int earliest_seq = unacked_pkttm_list.begin()->seqnum;
	for(int i = 0; i < unacked_pkttm_list.size(); i++)
	{
		struct pkt_time* tmp = &unacked_pkttm_list[i];
		if (tmp->stime < earliest_senttime)
		{
			earliest_senttime = tmp->stime;
			earliest_seq = tmp->seqnum;
		}
	}
	return earliest_seq;

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	cout<<"progress7"<<endl;
	// float earliest_senttime = unacked_pkttm_list.begin()->stime;
	int earliest_seq = find_earliest_unacked_seq();
	// for(int i = 0; i < unacked_pkttm_list.size(); i++)
	// {
	// 	struct pkt_time* tmp = &unacked_pkttm_list[i];
	// 	if (tmp->stime < earliest_senttime)
	// 	{
	// 		earliest_senttime = tmp->stime;
	// 		earliest_seq = tmp->seqnum;
	// 	}

	// }

	struct pkt_time* earliest_pckt = find_unacked_pkt(earliest_seq);
	earliest_pckt->stime = get_sim_time() + RTT;
	struct pkt *conts = create_pkt_char(earliest_pckt->payload, earliest_pckt->seqnum, earliest_pckt->acknum);
	tolayer3(0, *conts);
	updatetimer();
	cout<<"progress8"<<endl;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	cout<<"progress0"<<endl;
	nextseqnumber = 0;
	acknumber = 1;
	tosend_pkt_list.clear();
	unacked_pkttm_list.clear();
	window_n = getwinsize();
	cout<<"progress00"<<endl;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	cout<<"progress11"<<endl;
	bool check;
	check = (check_sum(&packet) != packet.checksum);
	if(check) return;
	printf("[B - ACK1] %d,%d,%.20s\n",  packet.seqnum, packet.acknum, packet.payload);
 

	struct pkt *ackpk = create_ACK_pkt(packet.seqnum ,1);

	if( check_in_received_sq(packet.seqnum))
	{
		tolayer3(1, *ackpk);
		return;
	}

	if( packet.seqnum < recv_seqnum)
	{
		
		tolayer3(1, *ackpk);
		return;
	}

	tolayer3(1, *ackpk);
	recv_pkt_list.push_back(packet);

	while(true)
	{
		if(recv_pkt_list.empty()) break;
		cout<<"progress9999999999"<<endl;

		int minseq = recv_pkt_list.begin()->seqnum;
		// struct pkt* minpkt;
		for(int i = 0; i < recv_pkt_list.size(); i++)
		{
			struct pkt* tmp = &recv_pkt_list[i];
			if(tmp->seqnum < minseq)
			{
				minseq = tmp->seqnum;
				// minpkt = &recv_pkt_list[i];
			}
		}
		struct pkt* minpkt = search_in_received_sq(minseq);
		cout<<minseq<<endl;
		cout<<recv_seqnum<<endl;
		cout<<minpkt->payload<<endl;
		if(minseq == recv_seqnum)
		{
			tolayer5(1, minpkt->payload);
			remove_from_recvlist(minseq);
			recv_seqnum++;
		}
		else
		{
			break;
		}

	}
	cout<<"progress12"<<endl;





}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	cout<<"progress1000"<<endl;
	recv_seqnum = 0;
	recv_pkt_list.clear();
	cout<<"progress1001"<<endl;
}
