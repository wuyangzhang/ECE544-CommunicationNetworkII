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

	 
	 Packet *p0 = packet->clone();
 	 Packet *p1 = packet->clone();
	 Packet *p2 = packet->clone();
	 Packet *p3 = packet->clone();
	 Packet *p4 = packet->clone();
	
	 
	 /*
     WritablePacket *p0 = Packet::make(0,0,packet->length(),0);
     memcpy(p0->data(), packet->data(),packet->length());

     
     WritablePacket *p1 = Packet::make(0,0,packet->length(),0);
     memcpy(p1->data(), packet->data(), packet->length());

     WritablePacket *p2 = Packet::make(0,0,packet->length(),0);
     memcpy(p2->data(), packet->data(),packet->length());

     WritablePacket *p3 = Packet::make(0,0,packet->length(),0);
     memcpy(p3->data(), packet->data(),packet->length());

     WritablePacket *p4 = Packet::make(0,0,packet->length(),0);
     memcpy(p4->data(), packet->data(),packet->length());
	*/
     
     
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

