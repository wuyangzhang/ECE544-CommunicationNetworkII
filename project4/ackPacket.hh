/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_ACKPACKET_HH 
#define CLICK_ACKPACKET_HH 
  
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS

class AckPacket : public Element {
  public:
    AckPacket();
    ~AckPacket();

    const char *class_name() const { return "AckPacket";}
    const char *port_count() const { return "1/1";}
    const char *processing() const { return PUSH; }

    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);

    void sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr);

    /* maintain ack table for hello, update, data */
    HashTable<uint8_t, bool>ackTable;
		
}; 

CLICK_ENDDECLS
#endif 
