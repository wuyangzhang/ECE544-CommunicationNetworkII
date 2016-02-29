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
Excercise2Packet::Excercise2Packet(){}
Excercise2Packet::~Excercise2Packet(){}

int Excercise2Packet::configure(Vector<String> &conf, ErrorHandler *errh){
	return 0;
}


void Excercise2Packet::push(int, Packet *p){
  click_chatter("preparing receiving a pakcet");
  click_chatter("receving a pakcet size %d", p->length());
  selfDefinedPacketHead *head = (selfDefinedPacketHead*) p->data();
  click_chatter("receiving a packet type %d", head->type);
  selfDefinedPacketPayload *payLoad = (selfDefinedPacketPayload*)(head+1);
  click_chatter("Receiving a packet content %s", payLoad->payload);

  return;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Excercise2Packet)
