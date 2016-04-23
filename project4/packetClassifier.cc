/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>

#include "packetClassifier.hh" 
#include "packet.hh"

CLICK_DECLS 
PacketClassifier::PacketClassifier(){

}

PacketClassifier::~PacketClassifier(){}

int 
PacketClassifier::initialize(ErrorHandler *errh){
    return 0;
}

/* classify packet according to packet type */
void 
PacketClassifier::push(int port, Packet *p) {

	assert(p);
	struct PacketType *header = (struct PacketType *) p->data();

	if(header->type == HELLO ) {
		/* add port info into packet */
		WritablePacket *q = p->push(sizeof(uint8_t));
		uint8_t *portNum = (uint8_t*) q->data();
		*portNum = *((uint8_t*)(&port));
		output(0).push(q);

	} else if(header->type == UPDATE) {
		/* add port info into packet */
		WritablePacket *q = p->push(sizeof(uint8_t));
		uint8_t *portNum = (uint8_t*) q->data();
		*portNum = *((uint8_t*)(&port));
		output(1).push(q);

	} else if(header->type == ACK) {
		output(2).push(p);

	} else if(header->type == DATA){
		struct DataPacket *format = (struct DataPacket*) p->data();
		click_chatter("[PacketClassifier] Receive data packet from source %d to %d , %d", format->sourceAddr, format->destinationAddr1, format->destinationAddr2);
		output(3).push(p);

	}else{
		click_chatter("[PacketClassifier] Invalid packet type, packet has been killed!");
		p->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketClassifier)

