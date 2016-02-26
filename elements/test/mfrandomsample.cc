#include <click/config.h>
#include <click/args.hh>
//#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/error.hh>

#include "mfrandomsample.hh"

CLICK_DECLS

MF_RandomSample::MF_RandomSample():RandomSample(),logger(MF_Logger::init()) {
}

MF_RandomSample::~MF_RandomSample() {
}

Packet* MF_RandomSample::pull( int port ){
  Packet *ret_pkt = RandomSample::pull(port);
  if ( ret_pkt == 0 ) {
  } else {
    //logger.log( MF_DEBUG, "random sample test"); 
  }
  return ret_pkt; 
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_RandomSample)
ELEMENT_REQUIRES(userlevel)
