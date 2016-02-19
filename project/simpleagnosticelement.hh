#ifndef CLICK_SIMPLEAGNOSTICELEMENT_HH
#define CLICK_SIMPLEAGNOSTICELEMENT_HH

#include<click/element.hh>
#include<click/timer.hh>
#include<click/timestamp.hh>
CLICK_DECLS

class SimpleAgnosticElement : public Element{
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
	const char *class_name() const {return "SimpleAgnosticElement";}
	const char *port_count() const {return "1/1";}
	const char *processing() const {return AGNOSTIC;}
	int configure(Vector<String>&, ErrorHandler*);
	void push(int, Packet *);
	Packet* pull(int);
	void run_timer(Timer* timer);
private:
	uint32_t maxSize;
	Timer _timer;

};

CLICK_ENDDECLS
#endif
