/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>

#include "broadcastModule.hh"
CLICK_DECLS 

BroadcastModule::BroadcastModule(){}

BroadcastModule::~BroadcastModule(){}

int 
BroadcastModule::initialize(){
    return 0;
}


/* broadcast up to 5 ports! */
void 
BroadcastModule::push(int port, Packet *packet) {

	 /*
	 duplicate expensive packet model !
	 WritablePacket *copy0 = packet->uniqueify();
	 WritablePacket *copy1 = packet->uniqueify();
	 WritablePacket *copy2 = packet->uniqueify();
	 WritablePacket *copy3 = packet->uniqueify();
	 WritablePacket *copy4 = packet->uniqueify();


	 output(0).push(copy0);
	 output(1).push(copy1);
	 output(2).push(copy2);
	 output(3).push(copy3);
	 output(4).push(copy4);
	*/
	 Packet *p0 = packet->clone();
 	 Packet *p1 = packet->clone();
	 Packet *p2 = packet->clone();
	 Packet *p3 = packet->clone();
	 Packet *p4 = packet->clone();

	 output(0).push(p0);
	 output(1).push(p1);
	 output(2).push(p2);
	 output(3).push(p3);
	 output(4).push(p4);

	 packet->kill();

}

/* deprecated
void
BroadcastModule::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){

}
*/
CLICK_ENDDECLS 
EXPORT_ELEMENT(BroadcastModule)

