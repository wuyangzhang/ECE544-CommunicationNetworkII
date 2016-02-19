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
        ShowPacket();
	~ShowPacket();
	const char *class_name() const {return "ShowPacket";}
	const char *port_count() const {return "1/1";}
	const char *processing() const {return PUSH;}
	int configure(Vector<String>&, ErrorHandler*);
        void push(int, Packet* p);
  //void showPacket();
};

CLICK_ENDDECLS
#endif
