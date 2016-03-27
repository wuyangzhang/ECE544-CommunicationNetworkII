#ifndef CLICK_PACKETCLIENT_HH 
#define CLICK_PACKETCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <click/hashtable.hh>

CLICK_DECLS
	
class PacketClient : public Element {
    public:
        PacketClient();
        ~PacketClient();
        const char *class_name() const { return "PacketClient";}
        const char *port_count() const { return "1/1";}
        const char *processing() const { return PUSH; }
		
        void run_timer(Timer*);
        int initialize(ErrorHandler*);
        int configure(Vector<String>&, ErrorHandler);
        void push(int, Packet *p);
        void helloMessage();
        void sendRequest();
    private: 
        Timer _timer;
        uint32_t sequenceNumber;
        HashTable<uint32_t,bool> ackSequenceTable;
        String srcAddr;
        String destAddr;
}; 

CLICK_ENDDECLS
#endif 