/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
*/

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>

#include "packet.hh"
#include "multicastRouter.hh"

CLICK_DECLS 

MulticastRouter::MulticastRouter() : _timer(this){}
MulticastRouter::~MulticastRouter(){}

void 
MulticastRouter::push(const int port, Packet *p) {
   
    /* classify input packet into: hello, update, ack, data*/
    struct PacketType *format = (struct PacketType*) p->data();
    int packetType = format->type;

     /*Hello packet*/
    if(packetType == HELLO){
        this->receiveHello(port, p);
    }

    /*Update Packet*/
    if(packetType == UPDATE){
        this->receiveUpdate(port, p);
    }

    /*Acknoledge Packet*/
    if(packetType == ACK){

    }

    /*Data Packet*/
    if(packetType == DATA){

    }

}


void
MulticastRouter::sendHello(){

    WritablePacket *helloPacket = Packet::make(0,0,sizeof(struct HelloPacket), 0);
    memset(helloPacket->data(), 0, helloPacket->length());
    struct HelloPacket *format = (struct PacketHeader*) helloPacket->data();
    format->type = HELLO;
    format->srcAddr = this->srcAddr;
    formt->sequenceNumber = this->helloSequence;

    output(0).push(helloPacket);
    //TO DO: how to broadcast packets to all neighbors?
    click_chatter("[MulticastRouter] Sending Hello Message with sequence %d", this->helloSequence);
}


/* ReceiveHello()
 * when receiving a hello packet, router events:
 * -> update routing table
 * -> update forwarding table 
 * -> send ack 
 */
void 
MulticastRouter::receiveHello(const int port, Packet *p){

    struct HelloPacket *format = (struct HelloPacket*) p->data();
    click_chatter("[MulticastRouter] Receiving Hello Packet from Source %d with sequence %d", format->sourceAddr, format->sequenceNumber);

    /* update routing table */
    this->updateRoutingTable(format->sourceAddr, 1, format->sourceAddr);

    /* update forwarding table */
    this->updateForwardingTable(format->sourceAddr, 1, port);

    /* send back ack */
    this->sendAck(port, format->sequenceNumber, format->sourceAddr);
}


/* 
 * sendUpdate()
 * send self routing table to its neighbors
 */
void
MulticastRouter::sendUpdate(){
    WritablePacket *updatePacket = Packet::make(0,0, sizeof(struct UpdatePacket)+ (sizeof(uint16_t)+sizeof(struct routingTableParam)) * this->routingTable.size() );
    memset(updatePacket->data(), 0, updatePacket->length());
    struct UpdataPacket *format = (struct UpdataPacket*) updatePacket->data();
    format->type = UPDATE;
    format->sourceAddr = this->srcAddr;
    format->sequenceNumber = this->updateSequence;
    format->length = (sizeof(uint16_t)+sizeof(struct routingTableParam))*this->routingTable.size(); /* length of payload */

    /* write payload as routing table info, with looping struct pointer to write payload*/
    UpdateInfo *updateinfo = (UpdateInfo*)(format+1);
    for(HashTable<uint16_t,routingTableParam>::iterator it = this->routingTable.begin(); it; ++it){
        updateinfo->sourceAddr = it.key();
        updateinfo->cost = it.value().cost;
        updateinfo->nextHop = it.value().nextHop;
        updateinfo+1;
    }
  
    output(1).push(updatePacket);

    click_chatter("[MulticastRouter] Sending Update Message with sequence %d", this->updateSequence);
}

/* ReceiveUpdate()
 * when receiving a update packet, router events:
 * -> update routing table
 * -> update forwarding table 
 * -> send ack 
 */
void
MulticastRouter::receiveUpdate(const int port, Packet *p){
    struct UpdatePacket *format = (struct UpdatePacket*) p->data();
    click_chatter("[MulticastRouter] Receiving Update Packet from Source %d with sequence %d", format->sourceAddr, format->sequenceNumber);

    /* update routing table */
    this->updateRoutingTable(format->sourceAddr, 1, format->sourceAddr);

    /* update forwarding table */
    this->updateForwardingTable(format->sourceAddr, 1, port);

    /* send back ack */
    this->sendAck(port, format->sequenceNumber, format->sourceAddr);
}


void 
MulticastRouter::sendAck(const int port, const uint8_t sequenceNumber, const uint16_t destinationAddr){

    WritablePacket *ackPacket = Packet::make(0,0,sizeof(struct AckPacket)+5, 0);
    memset(ackPacket->data(), 0, ackPacket->length());
    struct AckPacket *format = (struct AckPacket*) ackPacket->data();
    format->type = ACK;
    format->srcAddr = this->srcAddr;
    formt->sequenceNumber = sequenceNumber;
    formt->destinationAddr = destinationAddr;

    output(port).push(ackPacket);
    click_chatter("[MulticastRouter] Sending Ack to %d", destinationAddr);
}


void 
MulticastRouter::updateRoutingTable(const uint16_t sourceAddr, const uint16_t cost, const uint16_t nextHop){

    struct routingTableParam rtp;
    rtp.cost = cost;
    rtp.nextHop = nextHop;

    if(!this->routingTable[sourceAddr]){
        this->routingTable.set(sourceAddr, rtp);
    }else if(this->routingTable.get(sourceAddr).nextHop != nextHop){
        this->forwardingTable.set(sourceAddr, rtp);
    }
}


void 
MulticastRouter::updateForwardingTable(const uint16_t sourceAddr, const uint16_t cost, const uint8_t port){

    struct forwarTableParam ftp;
    ftp.cost = cost;
    ftp.port = port;

    if(!this->forwardingTable[sourceAddr]){
        this->forwardingTable.set(sourceAddr, ftp);
    }else if(this->forwardingTable.get(sourceAddr).port != port){
        this->forwardingTable.set(sourceAddr, ftp);
    }
}



void 
MulticastRouter::computeRoutingTable(uint16_t destAddr, uint32_t cost, uint16_t nextHop){
    struct routingTableParam rtp;
    rtp.cost = cost;
    rtp.nextHop = nextHop;


    if(!this->routingTable[destAddr]){
        this->forwardingTable.set(sourceAddr, rtp);
    }else if(cost < this->routingTable.get(sourceAddr).cost){
        this->routingTable.set(sourceAddr, rtp);
    }else if(cost = this->routingTable.get(sourceAddr.cost)){

    }
}


void
MulticastRouter::computeForwardingTable(){

}


int 
MulticastRouter::lookUpForwardingTable(uint32_t destAddr){

    int matchPort = -1;
    for(HashTable<int, uint32_t>::iterator it = this->forwardingTable.begin(); it; ++it){
      if(it.value() == destAddr)
            matchPort = it.key();
    }
    return matchPort;
}


void 
MulticastRouter::forwardingPacket(Packet *p, int port){
    output(port).push(p);
    click_chatter("[Router] forwarding packet to port %d", port);
}

void 
MulticastRouter::printRoutingTable(){

}


void
MulticastRouter::printForwardingTable(){

}


CLICK_ENDDECLS 
EXPORT_ELEMENT(MulticastRouter)