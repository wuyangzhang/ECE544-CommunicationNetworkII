#ifndef CLICK_PACKETSERVER_HH 
#define CLICK_PACKETSERVER_HH 
#include <click/element.hh>
#include <click/hashtable.hh>
CLICK_DECLS
class PacketServcer : public Element {
    public:
        PacketServcer();
       ~PacketServcer(); 
        const char *class_name() const { return "PacketServcer";}
        const char *port_count() const { return "1/1";}
        const char *processing() const { return PUSH; }

        void run_timer(Timer*);
        int initialize(ErrorHandler*);
        void push(int ,Packet*);
	    void helloMessage();
	private:
        Timer _timer;


};

CLICK_ENDDECLS
#endif