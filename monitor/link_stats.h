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
#ifndef LINK_STATS_H
#define LINK_STATS_H

#include <iostream>

#define MAX_NEIGHBOR_CNT 10

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#include <oml2/omlc.h>

#include <stdlib.h>

static OmlMPDef link_stats_mpd[] = {
    {"mp_index", OML_UINT32_VALUE},
    {"link_label", OML_STRING_VALUE},
    {"node_id", OML_STRING_VALUE},
    {"nbr_id", OML_STRING_VALUE},
    {"bitrate_mbps", OML_DOUBLE_VALUE},
    {"s_ett_usec", OML_UINT32_VALUE},
    {"l_ett_usec", OML_UINT32_VALUE},
    {"in_pkts", OML_UINT64_VALUE},
    {"out_pkts", OML_UINT64_VALUE},
    {"in_bytes", OML_UINT64_VALUE},
    {"out_bytes", OML_UINT64_VALUE},
    {"in_tput_mbps", OML_DOUBLE_VALUE},
    {"out_tput_mbps", OML_DOUBLE_VALUE},
    {NULL, (OmlValueT)0}
};

struct link {
    string label;
    string nbr_id;
    float bitrate_mbps;
    uint32_t s_ett_usec;
    uint32_t l_ett_usec;
    uint64_t in_pkts;
    uint64_t out_pkts;
    uint64_t in_bytes;
    uint64_t out_bytes;
    float in_tput_mbps;
    float out_tput_mbps;
};

typedef struct link link_t;

#ifdef __cplusplus
}
#endif 

class link_stats {

    private:
        OmlMP* oml_mp;
        string node_id;
        uint64_t t_stamp;
        int32_t link_cnt;
        link_t links[MAX_NEIGHBOR_CNT];		

        int get_link_index(string);

    public:

        link_stats(const char* id) {

            node_id = id;
            if((oml_mp = 
                omlc_add_mp("link_stats", link_stats_mpd)) == NULL) {
                cout << "FATAL: " 
                << "Unable to add link_stats OML measurement pt." 
                << endl;
                exit(1);
            }
            link_cnt = 0;
        }

        void inject_into_oml(unsigned mp_index);
        void read_from_click(int sockfd);
};

#endif //LINK_STATS_H
