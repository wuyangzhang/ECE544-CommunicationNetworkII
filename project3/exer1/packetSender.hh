#ifndef CLICK_PACKETSENDER_HH 
#define CLICK_PACKETSENDER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include < hashtable.hh>

CLICK_DECLS
	
class PacketSender : public Element {
    public:
        PacketSender();
        ~PacketSender();
        const char *class_name() const { return "PacketSender";}
        const char *port_count() const { return "0/1";}
        const char *processing() const { return PUSH; }
		
        void run_timer(Timer*);
        int initialize(ErrorHandler*);
		void receiveAck(Packet *p);
    private: 
        Timer _timer;
        int sequenceNumber;
        Hashtable<uint32_t,bool> ackSequence；
}; 

CLICK_ENDDECLS
#endif 
