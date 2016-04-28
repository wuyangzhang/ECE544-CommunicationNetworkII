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
#include "packet.hh" 


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

  _timerHello.schedule_after_sec(this->_period);
  return 0;
}

int
HelloModule::configure(Vector<String>&conf, ErrorHandler* errh){
  if (cp_va_kparse(conf, this, errh,
              "MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_myAddr,
              "ACK_TABLE", cpkP+cpkM, cpElement, &ackModule,
              "ROUTING_TABLE", cpkP+cpkM, cpElement, &routingTable,
              "DELAY", cpkP, cpUnsigned, &_delay,
              "PERIOD", cpkP, cpUnsigned, &_period,
              "TIME_OUT", cpkP, cpUnsigned, &_timeout,
              
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


  	struct HelloPacket* helloPacket = (struct HelloPacket*)(portNum+1);

    //click_chatter("[HelloModule] Receiving Hello Message from source %d with sequence %d from port %d", helloPacket->sourceAddr, helloPacket->sequenceNumber, *portNum);

    /* update routing table */
    this->routingTable->computeRoutingTable(helloPacket->sourceAddr, 1, helloPacket->sourceAddr);
    //click_chatter("[HelloModule] call computeRoutingTable!");

    /* update forwarding table */
    this->routingTable->computeForwardingTable(helloPacket->sourceAddr, 1, *portNum);
    //click_chatter("[HelloModule] call computeForwardingTable!");

    /* send back ack */
    this->sendAck(*portNum, helloPacket->sequenceNumber, helloPacket->sourceAddr);
    

    packet->kill();
}


void
HelloModule::sendHello(){
	if(ackModule->ackTable.get(this->helloSequence) == true){
		this->helloSequence++;
	}
    //click_chatter("[HelloModule] Sending Hello Message with sequence %d", this->helloSequence);

    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct HelloPacket), 0);

    memset(helloPacket->data(), 0, helloPacket->length());
    struct HelloPacket *format = (struct HelloPacket*) helloPacket->data();
    format->type = HELLO;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = this->helloSequence;
    output(0).push(helloPacket);

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

    WritablePacket *q = ackPacket->push(sizeof(uint8_t));
    uint8_t *port = (uint8_t*) q->data();
    *port = *((uint8_t*)(&portNum));

    output(1).push(q);
    //click_chatter("[HelloModule] Sending Hello Ack to Dest %d with portNum %d and sequence %d!", sourceAddr,portNum,sequenceNumber);
}
CLICK_ENDDECLS 
EXPORT_ELEMENT(HelloModule)

