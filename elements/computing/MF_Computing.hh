/*
 * MF_Computing.hh
 *
 */

#ifndef MF_COMPUTING_HH_
#define MF_COMPUTING_HH_

#include <click/element.hh>
#include <click/task.hh>
#include "click_MF.hh"
//#include "MF_Aggregator.hh"

CLICK_DECLS

class MF_Computing : public Element {
public:
	MF_Computing();
	~MF_Computing();

	const char *class_name() const		{ return "MF_Computing"; }
	const char *port_count() const		{ return "1/1"; }
	const char *processing() const		{ return "h/h"; }

	int configure(Vector<String>&, ErrorHandler *);
	Packet *simple_action(Packet *);
    //void push(Packet *p);

private:
	WritablePacket * _pkt;
    int _service_type; 
    
    //Vector<Vector<pkt*>* > _chkFile;
    //ChunkInfo _chkInfo;
    //int total_recv_bytes;
    
    //char chunk_payload[20000000]; //


};

CLICK_ENDDECLS
#endif /* MF_COMPUTING_HH_ */
