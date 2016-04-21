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

#include "updatePacket.hh" 

CLICK_DECLS 
UpdatePacket::UpdatePacket()_timeUpdate(this){
  this->helloSequence = 10;
  this->_myAddr = 0;
  this->_delay = 0;
  this->_period = 5;
  this->_timeout = 1;
}

UpdatePacket::~UpdatePacket(){}

int 
UpdatePacket::initialize(ErrorHandler *errh){
    return 0;
}


void
UpdatePacket::run_timer(Timer*){
  if(timer == &_timerUpdate){
    this->sendUpdate();
  }
  _timerUpdate.reschedule_after_sec(this->_period);
}


int
UpdatePacket::configure(Vector<String>&, ErrorHandler*){
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

/* void push()
 * after receiveing update packet, events invoked:
 * -> update routing table
 * -> update forwarding table
 * -> send ack
 */
void 
UpdatePacket::push(int port, Packet *packet) {
  assert(packet);
	/* it is a update packet with additional input port info*/
	uint8_t* portNum = (uint8_t*)packet->data();
	struct UpdatePacket* updatePacket = (struct UpdatePacket*)(portNum+1);

 	click_chatter("[MulticastRouter] Receiving Update Packet from Source %d with sequence %d from port %d", updatePacket->sourceAddr, updatePacket->sequenceNumber, *portNum);

  /* update routing table && forwarding table */
  uint16_t routingTableRowCount = updatePacket->length;
  uint16_t* cast = (uint16_t*)(updatePacket+1);
  for(uint16_t i = 0; i < routingTableRowCount; i++){
    uint16_t* castSrcAddr = (uint16_t*)cast;
    uint32_t* castCost = (uint32_t*)(castSrcAddr+1);
    cast = (uint32_t*)(castSrcAddr+1);
    uint16_t* castHopCount = (uint16_t*)(castCost+1);
    cast = (uint16_t*)(castHopCount+1);
    uint16_t* castNextHop = (uint16_t*)(castHopCount+1);
    cast = (uint16_t*)(castHopCount+1);
    for(uint16_t i = 0; i< *castHopCount; i++){
        this->routingTable->computeRoutingTable(*castSrcAddr, (*castCost) + 1, *castNextHop); /* castCost + 1 -> 1 hop to its neighbor */
        this->routingTable->computeForwardingTable(*castSrcAddr, (*castCost) + 1, portNum);
        castNextHop+1;
        cast+1;
    }
  }

  /* send back ack */
  this->sendAck(portNum, updatePacket->sequenceNumber, updatePacket->sourceAddr);
}

/* sendUpdate()
 * send self routing Table to neighbors
 */
void
UpdatePacket::sendUpdate(){


  /* 
   * calculate size of routing table 
   *-------------------------------------------------------
   * HashTable<uint16_t, routingTableParam>routingTable;
   * struct routingTableParam:
   *     uint32_t cost,
   *     uint16_t hopCount; 
   *     List<uint16_t> nextHop; 
   *--------------------------------------------------------
 */
  if(ackPacket.get(this->helloSequence == true)){
    this->sequenceNumber++;
  }
  int routingTableSize = 0;
  int routingTableRowCount = 0;
  for(List<uint16_t>::iterator it = this->routingTable.begin(); it != this->routingTable.end(); ++it){
      routingTableSize += (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t)* it.value().hopCount);
      routingTableRowCount++
  }

  WritablePacket *updatePacket = Packet::make(0,0, sizeof(struct UpdatePacket)+ routingTableSize);
  memset(updatePacket->data(), 0, updatePacket->length());
  struct UpdataPacket *format = (struct UpdataPacket*) updatePacket->data();
  format->type = UPDATE;
  format->sourceAddr = this->srcAddr;
  format->sequenceNumber = this->updateSequence;
  format->length = routingTableRowCount; /* length of payload */

  /* write payload as routing table info, with looping struct pointer to write payload*/
  uint16_t* cast = (uint16_t*)(format+1);
  for(HashTable<uint16_t,routingTableParam>::iterator it = this->routingTable.begin(); it; ++it){
    uint16_t* castSrcAddr = (uint16_t*)cast;
    *castSrcAddr = it.key();
    uint32_t* castCost = (uint32_t*)(castSrcAddr+1);
    cast = (uint32_t*)(castSrcAddr+1);
    *castCost = it.value().cost;
    uint16_t* castHopCount = (uint16_t*)(castCost+1);
    cast = (uint16_t*)(castCost+1);
    *castHopCount = it.value().hopCount;
    uint16_t* castNextHop = (uint16_t*)(castHopCount+1);
    for(List<uint16_t>::iterator list = this->routingTable.begin(); list != this->routingTable.end(); ++list){
       *castNextHop = list;
       castNextHop+1;
       cast+1;
    }
  }

  output(0).push(updatePacket);

  click_chatter("[MulticastRouter] Sending Update Message with sequence %d", this->updateSequence);
}


void
UpdataPacket::sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr){

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
EXPORT_ELEMENT(UpdatePacket)

