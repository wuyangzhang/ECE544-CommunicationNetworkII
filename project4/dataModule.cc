/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>

#include "dataModule.hh"
#include "packet.hh"
CLICK_DECLS 

DataModule::DataModule(){}

DataModule::~DataModule(){}

int 
DataModule::initialize(ErrorHandler* errh){
    return 0;
}


int
DataModule::configure(Vector<String>&conf, ErrorHandler* errh){
	if (cp_va_kparse(conf, this, errh,
           "ROUTING_TABLE", cpkP+cpkM, cpElement, &routingTable,
    		cpEnd) < 0) {
  	 	return -1;
  	}
  	return 0;
}


/* data module 
 * receiving a packet. 
 * check destination -> get forwarding port -> send out 
 *	uint8_t type;
 * 	uint16_t sourceAddr;
 * 	uint8_t k_value;
 * 	uint8_t sequenceNumber;
 * 	uint16_t destinationAddr1;
 * 	uint16_t destinationAddr2;
 * 	uint16_t destinationAddr3;
 * 	uint16_t length;
 *
 */
void 
DataModule::push(int port, Packet *packet) {

	struct DataPacket *dataPacket = (struct DataPacket *) packet->data();
	int forwardingPort = this->routingTable->lookUpForwardingTable(dataPacket->destinationAddr1);
	if(forwardingPort == -1){
		click_chatter("[DataModule] error: can not find forwarding port");
		packet->kill();
	}
	output(forwardingPort).push(packet);
}


CLICK_ENDDECLS 
EXPORT_ELEMENT(DataModule)

