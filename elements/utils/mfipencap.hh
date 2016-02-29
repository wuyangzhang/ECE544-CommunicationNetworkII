#ifndef MF_IPENCAP_HH
#define MF_IPENCAP_HH

#include <click/element.hh>
#include <click/glue.hh>
#include <click/atomic.hh>
#include <clicknet/ip.h>

#include "mf.hh"
#include "mflogger.hh"
#include "mfarptable.hh"

CLICK_DECLS

class MF_IPEncap : public Element { 

    public:

        MF_IPEncap();
        ~MF_IPEncap();

        const char *class_name() const		{ return "MF_IPEncap"; }
        const char *port_count() const		{ return PORTS_1_1; }

        int configure(Vector<String> &, ErrorHandler *);
        int initialize(ErrorHandler *);

        Packet *simple_action(Packet *);

    private:
 
        click_ip _iph;
        atomic_uint32_t _id;

        Element *_ARPTable;

        MF_Logger logger;

        inline void update_cksum(click_ip *, int) const;

};

CLICK_ENDDECLS
#endif //MF_IPENCAP_HH
