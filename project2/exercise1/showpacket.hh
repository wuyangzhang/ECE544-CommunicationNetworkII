#ifndef CLICK_SHOWPACKET_HH
#define CLICK_SHOWPACKET_HH

#include<click/element.hh>
CLICK_DECLS

class ShowPacket : public Element{
public:
	struct selfDefinedPacketHead{
		uint8_t type;
		uint32_t length;
	};
	struct selfDefinedPacketPayload{
		const char* payload;
	};
	SimpleAgnosticElement();
	~SimpleAgnosticElement();
	const char *class_name() const {return "ShowPacket";}
	const char *port_count() const {return "1/0";}
	const char *processing() const {return PULL;}
	int configure(Vector<String>&, ErrorHandler*);
	Packet* pull(int);
};

CLICK_ENDDECLS
#endif