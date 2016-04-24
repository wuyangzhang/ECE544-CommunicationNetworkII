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

#include "packet.hh"
#include "ackModule.hh" 

CLICK_DECLS 
AckModule::AckModule(){}

AckModule::~AckModule(){}

int 
AckModule::initialize(ErrorHandler* errh){
    return 0;
}

void 
AckModule::push(int port, Packet *packet) {
	assert(packet);
	if(port == 0){
		//[0]ack, receive an ack, update table
		struct AckPacket* ackPacket = (struct AckPacket*)packet->data();
		ackTable.set(ackPacket->sequenceNumber, true);
		//click_chatter("[AckModule] Receiving Ack Packet from Source %d with sequence %d", ackPacket->sourceAddr, ackPacket->sequenceNumber);
		packet->kill();
	}else if(port ==1){
		//[1]ack , send ack
		this->sendAck(packet);
	}
	
}

void
AckModule::sendAck(Packet *packet){
	uint8_t* port = (uint8_t*)packet->data();
	packet->pull(sizeof(uint8_t));
	output(*port).push(packet);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(AckModule)

