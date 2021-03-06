/*
 *  Copyright (c) 2010-2013, Rutgers, The State University of New Jersey
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the organization(s) stated above nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef PACKET_STATS_H
#define PACKET_STATS_H

//max number of router ports that stats can be reported for
#define MAX_PORTS 8

#include <iostream>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <oml2/omlc.h>

#include <stdlib.h>

static OmlMPDef packet_stats_mpd[] = {
  {"mp_index", OML_UINT32_VALUE},
  {"node_id", OML_STRING_VALUE},
  {"port_id", OML_STRING_VALUE},
  {"in_pkts", OML_UINT64_VALUE},
  {"out_pkts", OML_UINT64_VALUE},
  {"errors", OML_UINT64_VALUE},
  {"dropped", OML_UINT64_VALUE},
  {"in_bytes", OML_UINT64_VALUE},
  {"out_bytes", OML_UINT64_VALUE},
  {"in_tput_mbps", OML_DOUBLE_VALUE},
  {"out_tput_mbps", OML_DOUBLE_VALUE},
  {NULL, (OmlValueT)0}
};

struct per_port_stats{

    string port_id;
    uint64_t in_pkts;
    uint64_t out_pkts;
    uint64_t errors;
    uint64_t dropped;
    uint64_t in_bytes;
    uint64_t out_bytes;
    double in_tput_mbps;
    double out_tput_mbps;
};



#ifdef __cplusplus
}
#endif 

class packet_stats{

    private:
	OmlMP* oml_mp;
	
	string node_id;
        uint64_t t_stamp;
        int num_ports;
        struct per_port_stats pps[MAX_PORTS];

    public:

	packet_stats(const char* id) {

            node_id = id;
            if ((oml_mp = 
                omlc_add_mp("packet_stats", packet_stats_mpd)) == NULL) {
                cout << "FATAL: " 
                << "Unable to add packet_stats OML measurement pt." 
                << endl;
                exit(1);
            }
            num_ports = 0;
	}	

	void inject_into_oml(unsigned mp_index);
	void read_from_click(int sockfd);
};

#endif //PACKET_STATS_H
