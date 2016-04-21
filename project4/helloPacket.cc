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

#include "helloPacket.hh" 


CLICK_DECLS 
HelloPacket::HelloPacket() : _timerHello(this){
	this->helloSequence = 0;
	this->_myAddr = 0;
	this->_delay = 0;
	this->_period = 5;
	this->_timeout = 1;
}

HelloPacket::~HelloPacket(){}

int 
HelloPacket::initialize(ErrorHandler *errh){
	this->_timeHello.initialize(this);
	if(this->_delay > 0 ){
		_timerHello.schedule_after_sec(this->_delay);
	}
    return 0;
}

int
HelloPacket::configure(Vector<String>&, ErrorHandler*){
  if (cp_va_kparse(conf, this, errh,
              "MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_myAddr,
              "DELAY", cpkP, cpUnsigned, &_delay,
              "PERIOD", cpkP, cpUnsigned, &_period,
              "TIME_OUT", cpkP, cpUnsigned, &_timeout,
              "ACK_TABLE", cpkP+cpkM, cpElement, &ackPacket,
              "ROUTING_TABLE", cpkP+cpkM, cpElement, &routingTable,
              cpEnd) < 0) {
	return -1;
  }
  return 0;
}

void
HelloPacket::run_timer(Timer* timer){
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
HelloPacket::push(int port, Packet *packet) {
	assert(packet);
	/* it is a hello packet with additional input port info*/
	uint8_t* portNum = (int*)packet->data();
	struct HelloPacket* helloPacket = (struct HelloPacket*)(portNum+1);

 	click_chatter("[MulticastRouter] Receiving Hello Packet from Source %d with sequence %d from port %d", helloPacket->sourceAddr, helloPacket->sequenceNumber, *portNum);

    /* update routing table */
    this->routingTable->computeRoutingTable(helloPacket->sourceAddr, 1, helloPacket->sourceAddr);

    /* update forwarding table */
    this->routingTable->computeForwardingTable(helloPacket->sourceAddr, 1, *portNum);

    /* send back ack */
    this->sendAck(*portNum, helloPacket->sequenceNumber, helloPacket->sourceAddr);
}


void
HelloPacket::sendHello(){
	if(ackPacket.get(this->helloSequence == true)){
		this->sequenceNumber++;
	}
    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct HelloPacket), 0);
    memset(helloPacket->data(), 0, helloPacket->length());
    struct HelloPacket *format = (struct PacketHeader*) helloPacket->data();
    format->type = HELLO;
    format->srcAddr = this->srcAddr;
    formt->sequenceNumber = this->helloSequence;

    output(0).push(helloPacket);

    click_chatter("[MulticastRouter] Sending Hello Message with sequence %d", this->helloSequence);
}

void
HelloPacket::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){
	WritablePacket *ackPacket = Packet::make(0,0, sizeof(AckPacket));
    memset(ackPacket->data(), 0, ackPacket->length());
    struct AckPacket* format = (struct AckPacket*) ackPacket->data();
    format->type = ACK;
    format->sourceAddr = this->_myAddr;
    format->sequenceNumber = sequenceNumber;
    format->destinationAddr = sourceAddr;

    output(portNum).push(ackPacket);
}
CLICK_ENDDECLS 
EXPORT_ELEMENT(HelloPacket)

