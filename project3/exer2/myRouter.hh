#ifndef CLICK_MYROUTER_HH 
#define CLICK_MYROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <click/hashtable.hh>
#include <click/string.hh>
CLICK_DECLS
/*
    functions in my router:
    1. receive helloMessage and update Routing table
        client just sends out the packet.(client includes its addr and the dest addr in the pakcet)
        We use RouterPort to directly connect client and router. 
        Router should find out which port is connected with the incoming packet and record that in the routing table. 

    2. Forwarding the packet. 



*/
class MyRouter : public Element {
    public:
        MyRouter();
        ~MyRouter();
        const char *class_name() const { return "MyRouter";}
        const char *port_count() const { return "3-/3-";} /* each router has at least 1 port */
        const char *processing() const { return PUSH; }
	   
        void push(int, Packet *p);
        void updateForwardingTable(int port, String in_addr);
        int lookUpForwardingTable(String destAddr);
        void forwardingPacket(Packet *p, int port);
        void sendRequest();
    private: 
        Timer _timer;
        HashTable<int, uint32_t> forwardingTable;
        uint32_t srcAddr;
        uint32_t destAddr;
}; 

CLICK_ENDDECLS
#endif 
