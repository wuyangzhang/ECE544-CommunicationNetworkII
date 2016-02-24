#ifndef CLICK_PACKETRECEIVER_HH 
#define CLICK_PACKETPRINTER_HH 
#include <click/element.hh>
#include <click/hashtable.hh>
CLICK_DECLS
class PacketReceiver : public Element {
    public:
        PacketReceiver();
       ~PacketReceiver(); 
        const char *class_name() const { return "PacketReceiver";}
        const char *port_count() const { return "1/1";}
        const char *processing() const { return PUSH; }
        void push(int ,Packet*);
		
};

CLICK_ENDDECLS
#endif
