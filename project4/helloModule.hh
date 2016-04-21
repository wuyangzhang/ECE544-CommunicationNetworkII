/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_HELLOMODULE_HH 
#define CLICK_HELLOMODULE_HH 
  
#include <click/element.hh>
#include <click/timer.hh>

#include "routingTable.hh"
#include "packets.hh"
#include "ackPacket.hh"

CLICK_DECLS

class HelloModule : public Element {
  public:
    HelloModule();
    ~HelloModule();

    const char *class_name() const { return "HelloModule";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }
    void run_timer(Timer*);
    int initialize(ErrorHandler*);
    int configure(Vector<String>&, ErrorHandler*);

    void push(int port, Packet *packet);

    void sendHello();
    void receiveHello(const int port, Packet *p);

    void sendAck(const uint8_t portNum, const uint8_t sequenceNumber, const uint16_t sourceAddr);

  private:
    RoutingTable* routingTable;
    AckModule* ackModule;
    Timer _timerHello;
    uint8_t helloSequence;
    uint16_t _myAddr;
    uint32_t _delay;
    uint32_t _period;
    uint32_t _timeout;

}; 

CLICK_ENDDECLS
#endif 
