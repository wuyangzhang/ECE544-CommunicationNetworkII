/*
	Author: WuyangZhang
	Creating Date: Feb 23/ 2016
	Define show packet element
*/


#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "exercise2Packet.hh"

CLICK_DECLS
Exercise2Packet::Exercise2Packet(){}
Exercise2Packet::~Exercise2Packet(){}

int Exercise2Packet::configure(Vector<String> &conf, ErrorHandler *errh){
	return 0;
}


void Exercise2Packet::push(int, Packet *p){
  selfDefinedPacketHead *head = (selfDefinedPacketHead*) p->data();
  selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(head+1);
  click_chatter("Receiving packet content:  %s", payLoad->payload);
  if(head->type == 0){
    output(0).push(p);
    click_chatter("Sending a packet with TYPE %d to PORT 0", head->type);
  }
  if(head->type == 1){
    output(1).push(p);
    click_chatter("Sending a packet with TYPE %d to PORT 1", head->type);

  }
  return;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Exercise2Packet)
