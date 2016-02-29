/*
	Author: WuyangZhang
	Creating Date: Feb 23/ 2016
	Define show packet element
*/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "simpleagnosticelement.hh"

CLICK_DECLS
SimpleAgnosticElement::SimpleAgnosticElement(){}
SimpleAgnosticElement::~SimpleAgnosticElement(){}

int SimpleAgnosticElement::configure(Vector<String> &conf, ErrorHandler *errh){
	return 0;
}


Packet* SimpleAgnosticElement::pull(int){
	Packet *p = input(0).pull();
	if(p == 0)return 0;
	click_chatter("Receiving a packet size %d", p->length());

	selfDefinedPacketHead *head = (selfDefinedPacketHead*) p->data();
	selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(head+1);
	click_chatter("Receiving a packet content %s", payLoad->payload;



}
CLICK_ENDDECLS
EXPORT_ELEMENT(SimpleAgnosticElement)