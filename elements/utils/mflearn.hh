#ifndef MF_LEARN_HH
#define MF_LEARN_HH

#include <click/element.hh>
#include <clicknet/ether.h>

#include "mf.hh"
#include "mflogger.hh"
#include "mfarptable.hh"

CLICK_DECLS

class MF_Learn : public Element { 
 public:
  MF_Learn();
  ~MF_Learn();

  const char *class_name() const		{ return "MF_Learn"; }
  const char *port_count() const		{ return PORTS_1_1; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);

  Packet *simple_action(Packet *);

 private:

  uint8_t in_port;

  Element *arp_tbl_p;

  MF_Logger logger;

};

CLICK_ENDDECLS
#endif //MF_LEARN_HH
