/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_UPDATEPACKET_HH 
#define CLICK_UPDATEPACKET_HH 
  
#include <click/element.hh>
#include <click/timer.hh>

#include "updatePacket.hh"
#include "packets.hh"
#include "ackPacket.hh"

CLICK_DECLS

class UpdatePacket : public Element {
  public:
    UpdatePacket();
    ~UpdatePacket();

    const char *class_name() const { return "UpdatePacket";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }
    void run_timer(Timer*);
    int initialize(ErrorHandler*);
    int configure(Vector<String>&, ErrorHandler*);

    void push(int port, Packet *packet);

    void sendUpdate();
    void receiveUpdate(const int port, Packet *p);

  private:
    AckPacket* ackPacket;
    RoutingTable* routingTable;
    Timer _timerUpdate;
    uint16_t _myAddr;
    uint32_t _delay;
    uint32_t _period;
    uint32_t _timeout;
    uint8_t updateSequence;

}; 

CLICK_ENDDECLS
#endif 
