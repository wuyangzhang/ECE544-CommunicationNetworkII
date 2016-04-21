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

#include "helloModule.hh" 


CLICK_DECLS 
HelloModule::HelloModule() : _timerHello(this){
	this->helloSequence = 0;
	this->_myAddr = 0;
	this->_delay = 0;
	this->_period = 5;
	this->_timeout = 1;
}

HelloModule::~HelloModule(){}

int 
HelloModule::initialize(ErrorHandler *errh){
	this->_timerHello.initialize(this);
	if(this->_delay > 0 ){
		_timerHello.schedule_after_sec(this->_delay);
	}
    return 0;
}

int
HelloModule::configure(Vector<String>&conf, ErrorHandler* errh){
  if (cp_va_kparse(conf, this, errh,
              "MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_myAddr,
              "DELAY", cpkP, cpUnsigned, &_delay,
              "PERIOD", cpkP, cpUnsigned, &_period,
              "TIME_OUT", cpkP, cpUnsigned, &_timeout,
              "ACK_TABLE", cpkP+cpkM, cpElement, &ackModule,
              "ROUTING_TABLE", cpkP+cpkM, cpElement, &routingTable,
              cpEnd) < 0) {
	return -1;
  }
  return 0;
}

void
HelloModule::run_timer(Timer* timer){
	if(timer == &_timerHello){
		this->sendHello();
	}
	_timerHello.reschedule_after_sec(this->_period);
}



/* void push()
 * after receiveing hello packet, events invoked:
 * -> update routing table
 * -> update forwarding table
 * -> send ack
 */
void 
HelloModule::push(int port, Packet *packet) {
	assert(packet);
	/* it is a hello packet with additional input port info*/
	uint8_t* portNum = (uint8_t*)packet->data();
	struct HelloPacket* HelloPacket = (struct HelloPacket*)(portNum+1);

 	click_chatter("[MulticastRouter] Receiving Hello Packet from Source %d with sequence %d from port %d", HelloPacket->sourceAddr, HelloPacket->sequenceNumber, *portNum);

    /* update routing table */
    this->routingTable->computeRoutingTable(HelloPacket->sourceAddr, 1, HelloPacket->sourceAddr);

    /* update forwarding table */
    this->routingTable->computeForwardingTable(HelloPacket->sourceAddr, 1, *portNum);

    /* send back ack */
    this->sendAck(*portNum, HelloPacket->sequenceNumber, HelloPacket->sourceAddr);
}


void
HelloModule::sendHello(){
	if(ackModule->get(this->helloSequence == true)){
		this->helloSequence++;
	}
    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct HelloPacket), 0);
    memset(HelloPacket->data(), 0, HelloPacket->length());
    struct HelloPacket *format = (struct HelloPacket*) helloPacket->data();
    format->type = HELLO;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = this->helloSequence;

    output(0).push(HelloPacket);

    click_chatter("[MulticastRouter] Sending Hello Message with sequence %d", this->helloSequence);
}

void
HelloModule::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){
	  WritablePacket *ackPacket = Packet::make(0,0, sizeof(struct AckPacket),0);
    memset(ackPacket->data(), 0, ackPacket->length());
    struct AckPacket* format = (struct AckPacket*) ackPacket->data();
    format->type = ACK;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = sequenceNumber;
    format->destinationAddr = sourceAddr;

    output(portNum).push(ackPacket);
}
CLICK_ENDDECLS 
EXPORT_ELEMENT(HelloModule)

