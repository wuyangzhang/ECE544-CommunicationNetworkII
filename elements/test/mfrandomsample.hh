#ifndef MF_RANDOMSAMPLE_HH
#define MF_RANDOMSAMPLE_HH
#include <elements/standard/randomsample.hh>

#include "mflogger.hh"
CLICK_DECLS

class MF_RandomSample : public RandomSample {
public:
  MF_RandomSample();
  ~MF_RandomSample();
  const char *class_name() const      { return "MF_RandomSample";}
  //void push(int port, Packet *p); 
  Packet* pull(int port); 
private:
  MF_Logger logger; 
}; 

CLICK_ENDDECLS
#endif
