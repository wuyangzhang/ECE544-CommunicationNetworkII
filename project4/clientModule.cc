/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>

#include "clientModule.hh"
#include "packet.hh"

CLICK_DECLS 

ClientModule::ClientModule() : _timerClient(this) {	
  this->helloSequence = 0;
  this->dataSequence = 10;
  this->_myAddr = 0;
  this->_otherAddr1 = 0;
  this->_otherAddr2 = 0;
  this->_otherAddr3 = 0;
  this->_delay = 0;
  this->_period = 5;
  this->_timeout = 1;
}

ClientModule::~ClientModule(){}


int 
ClientModule::initialize(ErrorHandler* errh){
	this->_timerClient.initialize(this);

 	if(this->_delay > 0 ){
    	_timerClient.schedule_after_sec(this->_delay);
  	}

  	_timerClient.schedule_after_sec(this->_period);

    return 0;
}


void
ClientModule::run_timer(Timer* timer){
  if(timer == &_timerClient){
    this->sendData();
    this->sendHello();
  }
  _timerClient.reschedule_after_sec(this->_period);
}


int
ClientModule::configure(Vector<String>&conf, ErrorHandler* errh){
 if (cp_va_kparse(conf, this, errh,
              "MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_myAddr,
              "OHTER_ADDR1", cpkP, cpUnsigned, &_otherAddr1,
              "OHTER_ADDR2", cpkP, cpUnsigned, &_otherAddr2,
              "OHTER_ADDR3", cpkP, cpUnsigned, &_otherAddr3,
              "DELAY", cpkP, cpUnsigned, &_delay,              
              "PERIOD", cpkP, cpUnsigned, &_period,
              "TIME_OUT", cpkP, cpUnsigned, &_timeout,
              cpEnd) < 0) {
  	return -1;
  }
  return 0;
}

void 
ClientModule::push(int port, Packet *packet) {
	struct DataPacket *format = (struct DataPacket*) packet->data();
	if(format->type == DATA){
		click_chatter("[ClientModule] Receiving packet from source %d", format->sourceAddr);
		this->sendAck(format->sourceAddr, format->sequenceNumber);
	}
	packet->kill();
}


/*Data Packet Format:
	uint8_t type;
  	uint16_t sourceAddr;
  	uint8_t k_value;
  	uint8_t sequenceNumber;
  	uint16_t destinationAddr1;
  	uint16_t destinationAddr2;
  	uint16_t destinationAddr3;
  	uint16_t length;
 */
void
ClientModule::sendData(){
	 if(ackTable.get(this->dataSequence) == true){
    	this->dataSequence++;
     }
  
	 WritablePacket *dataPacket = Packet::make(0,0, sizeof(struct DataPacket)+ 5, 0);
	 memset(dataPacket->data(), 0, dataPacket->length());
	 struct DataPacket *format = (struct DataPacket*) dataPacket->data();
	 format->type = DATA;
	 format->sourceAddr = this->_myAddr;
	 format->k_value = 1;
	 format->destinationAddr1 = this->_otherAddr1;
	 format->destinationAddr2 = this->_otherAddr2;
	 format->destinationAddr3 = this->_otherAddr3;
	 format->sequenceNumber = this->dataSequence;
	 format->length = 5;

	 output(0).push(dataPacket);
}

void
ClientModule::sendAck(uint16_t destinationAddr, uint8_t sequenceNumber){
    WritablePacket *ackPacket = Packet::make(0,0, sizeof(AckPacket), 0);
    memset(ackPacket->data(), 0, ackPacket->length());
    struct AckPacket* format = (struct AckPacket*) ackPacket->data();
    format->type = ACK;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = sequenceNumber;
    format->destinationAddr = destinationAddr;

    output(0).push(ackPacket);
}

void 
ClientModule::sendHello(){
    if(ackTable.get(this->helloSequence) == true){
		this->helloSequence++;
	}

    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct HelloPacket), 0);
    memset(helloPacket->data(), 0, helloPacket->length());
    struct HelloPacket *format = (struct HelloPacket*) helloPacket->data();
    format->type = HELLO;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = this->helloSequence;
    output(0).push(helloPacket);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(ClientModule)

