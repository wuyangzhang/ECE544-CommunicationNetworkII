/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_ACKMODULE_HH 
#define CLICK_ACKMODULE_HH 
  
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS

class AckModule : public Element {
  public:
    AckModule();
    ~AckModule();

    const char *class_name() const { return "AckModule";}
    const char *port_count() const { return "2/1-";}
    const char *processing() const { return PUSH; }

    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);

    void sendAck(Packet *packet);

    /* maintain ack table for hello, update, data */
    HashTable<uint8_t, bool>ackTable;
		
}; 

CLICK_ENDDECLS
#endif 
