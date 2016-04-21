/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_BROADCASTMODULE_HH 
#define CLICK_BROADCASTMODULE_HH 
  
#include <click/element.hh>

CLICK_DECLS

class BroadcastModule : public Element {
  public:
    BroadcastModule();
    ~BroadcastModule();

    const char *class_name() const { return "BroadcastModule";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }

    void push(int port, Packet *packet);
    int initialize();

    //void sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr);

    /* maintain ack table for hello, update, data */
		
}; 

CLICK_ENDDECLS
#endif 
