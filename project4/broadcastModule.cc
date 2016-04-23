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
	 Packet *p0 = packet->clone();
 	 Packet *p1 = packet->clone();
	 Packet *p2 = packet->clone();
	 Packet *p3 = packet->clone();
	 Packet *p4 = packet->clone();
	*/


     WritablePacket *p01 = packet->uniqueify();
          output(0).push(p01);

     WritablePacket *p11 = p01->uniqueify();
     	 output(1).push(p11);

     WritablePacket *p21 = p11->uniqueify();
     	 output(2).push(p21);

     WritablePacket *p31 = p21->uniqueify();
     	 output(3).push(p31);

     WritablePacket *p41 = p31->uniqueify();
	 	 output(4).push(p41);

     /*
	 output(0).push(p0);
	 output(1).push(p11);
	 output(2).push(p2);
	 output(3).push(p3);
	 output(4).push(p4);
	 */

//	 packet->kill();

//	 p0->kill();
//	 p1->kill();
//	 p2->kill();
//	 p3->kill();
//	 p4->kill();

}

/* deprecated
void
BroadcastModule::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){

}
*/
CLICK_ENDDECLS 
EXPORT_ELEMENT(BroadcastModule)

