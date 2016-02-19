/*
	Author: WuyangZhang
	Creating Date: Feb 23/ 2016
	Define simple agnostic element
*/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "simpleagnosticelement.hh"

CLICK_DECLS
SimpleAgnosticElement::SimpleAgnosticElement()
  :_timer(this)
{}
SimpleAgnosticElement::~SimpleAgnosticElement(){}

int SimpleAgnosticElement::configure(Vector<String> &conf, ErrorHandler *errh){
	/*
		check valid packet size
	*/
	if (cp_va_kparse(conf, this, errh, "MAXPACKETSIZE", cpkM, cpInteger, &maxSize, cpEnd) < 0) return -1;
	if (maxSize <= 0) return errh->error("maxsize should be larger than 0");
	_timer.initialize(this);
	_timer.schedule_after_msec(100);
	return 0;
}


void SimpleAgnosticElement::push(int, Packet *p){
	click_chatter("Receiving a packet size %d", p->length());

	
	//	int tailroom = 0;
	//int packetsize = sizeof(selfDefinedPacketHead) + sizeof(selfDefinedPacketPayload);
	//int headroom = sizeof(click_ip) + sizeof(click_tcp) + sizeof(click_ether);
	//int headroom = 0;
	//WritablePacket *packet = Packet::make(headroom, 0, packetsize, tailroom);
	//if(packet == 0) return click_chatter("ERROR: can not make packet!");
	/* write the packet header */

	/*
	selfDefinedPacketHead* header = (selfDefinedPacketHead*) packet->data();
	header->type = 0;
	header->length = sizeof(selfDefinedPacketPayload);
	//int offsetToPayload = size(selfDefinedPacketHead);
	selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(header+1);
	payLoad->payload = "I am selfDefinedPacketPayload";
	*/
	/*periodic generate packet.*/
	//create a packe here 
	output(0).push(p);
	
}


Packet* SimpleAgnosticElement::pull(int){
	Packet *p = input(0).pull();
	if(p == 0)return 0;
	click_chatter("Receiving a packet size %d", p->length());

	if(p->length() > maxSize)
		p->kill();
	else
		return p;


}

void SimpleAgnosticElement::run_timer(Timer* t){
	Timestamp now = Timestamp::now_steady();
	click_chatter("generate a new selfDefined packet at %{timestamp}\n", &now);
	int tailroom = 0;
        int packetsize = sizeof(selfDefinedPacketHead) + sizeof(selfDefinedPacketPayload);
        int headroom = 0;
        WritablePacket *packet = Packet::make(headroom, 0, packetsize, tailroom);
        if(packet == 0) return click_chatter("ERROR: can not make packet!");
        /* write the packet header */
        selfDefinedPacketHead* header = (selfDefinedPacketHead*) packet->data();
        header->type = 0;
        header->length = sizeof(selfDefinedPacketPayload);
        //int offsetToPayload = size(selfDefinedPacketHead);                                                                                                                         
        selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(header+1);
        payLoad->payload = "I am selfDefinedPacketPayload";

        /*periodic generate packet.*/
        //create a packe here                                                                                                                                                               
	//this->push(0,packet);
	output(0).push(packet);
	_timer.reschedule_after_sec(3);

}

CLICK_ENDDECLS
EXPORT_ELEMENT(SimpleAgnosticElement)
