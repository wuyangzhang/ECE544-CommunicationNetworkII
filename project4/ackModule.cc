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

#include "ackModule.hh" 
#include "packet.hh"

CLICK_DECLS 
AckModule::AckModule(){}

AckModule::~AckModule(){}

int 
AckModule::initialize(ErrorHandler *errh){
    return 0;
}

void 
AckModule::push(int port, Packet *packet) {
	assert(packet);
	struct AckPacket* ackPacket = (struct AckPacket*)packet->data();

	ackTable.set(ackPacket->sequenceNumber, true);
	click_chatter("[MulticastRouter] Receiving Update Packet from Source %d with sequence %d", ackPacket->sourceAddr, ackPacket->sequenceNumber);

	packet->kill();
}


void
AckModule::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){

}
CLICK_ENDDECLS 
EXPORT_ELEMENT(AckPacket)

