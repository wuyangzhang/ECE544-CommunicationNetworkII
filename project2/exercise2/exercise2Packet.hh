#ifndef CLICK_EXERCISE2PACKET_HH
#define CLICK_EXERCISE2PACKET_HH

#include<click/element.hh>
CLICK_DECLS

class Exercise2Packet : public Element{
public:
	struct selfDefinedPacketHead{
		uint8_t type;
		uint32_t length;
	};
	struct selfDefinedPacketPayload{
		const char* payload;
	};

    Exercise2Packet();
	~Exercise2Packet();
	const char *class_name() const {return "ExercisePacket";}
	const char *port_count() const {return "1/1";}
	const char *processing() const {return PUSH;}
	int configure(Vector<String>&, ErrorHandler*);
    void push(int, Packet* p);
  //void ExercisePacket();
};

CLICK_ENDDECLS
#endif
