/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring

*/

#ifndef CLICK_CLIENTMODULE_HH 
#define CLICK_CLIENTMODULE_HH 
  
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS

class ClientModule : public Element {
  public:
    ClientModule();
    ~ClientModule();

    const char *class_name() const { return "ClientModule";}
    const char *port_count() const { return "1/1-";}
    const char *processing() const { return PUSH; }

    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);
    int configure(Vector<String>&conf, ErrorHandler* errh);

    void run_timer(Timer*);
    void sendData();
    void sendHello();
    void sendAck(uint16_t destionAddr, uint8_t sequence);
  private:
    uint16_t _myAddr;
    uint8_t k_value;
    uint16_t _otherAddr1;
    uint16_t _otherAddr2;
    uint16_t _otherAddr3;
    uint32_t _delay;
    uint32_t _period;
    uint32_t _timeout;
    uint8_t dataSequence;
    uint8_t helloSequence;
    Timer _timerHello;
    Timer _timerData;
    HashTable<uint8_t,bool> ackTable;


}; 

CLICK_ENDDECLS
#endif 
