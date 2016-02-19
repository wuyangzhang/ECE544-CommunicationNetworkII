/*
	Author: WuyangZhang
	Creating Date: Feb 23/ 2016
	Define show packet element
*/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "showpacket.hh"

CLICK_DECLS
ShowPacket::ShowPacket(){}
ShowPacket::~ShowPacket(){}

int ShowPacket::configure(Vector<String> &conf, ErrorHandler *errh){
	return 0;
}


void ShowPacket::push(int, Packet *p){
  click_chatter("preparing receiving a pakcet");
  click_chatter("receving a pakcet size %d", p->length());
  selfDefinedPacketHead *head = (selfDefinedPacketHead*) p->data();
  click_chatter("receiving a packet type %d", head->type);
  selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(head+1);
  click_chatter("Receiving a packet content %s", payLoad->payload);

  return;
}


/*
void ShowPacket::showPacket(){
	Packet *p = input(0).pull();
	if(p == 0)return;
	click_chatter("Receiving a packet size %d", p->length());
`	
	selfDefinedPacketHead *head = (selfDefinedPacketHead*) p->data();
	selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(head+1);
	click_chatter("Receiving a packet content %s", payLoad->payload);
	

}
*/
CLICK_ENDDECLS
EXPORT_ELEMENT(ShowPacket)
