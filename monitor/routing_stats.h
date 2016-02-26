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
#ifndef ROUTING_STATS_H
#define ROUTING_STATS_H

#include <iostream>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <oml2/omlc.h>
#include <stdlib.h>

static OmlMPDef routing_stats_mpd[] = {
    {"mp_index", OML_UINT32_VALUE},
    {"node_id", OML_STRING_VALUE},
    {"in_chunks", OML_UINT64_VALUE},
    {"out_chunks", OML_UINT64_VALUE},
    {"in_ctrl_msgs", OML_UINT64_VALUE},
    {"out_ctrl_msgs", OML_UINT64_VALUE},
    {"stored_chunks", OML_UINT64_VALUE},
    {"error_chunks", OML_UINT64_VALUE},
    {"dropped_chunks", OML_UINT64_VALUE},
    {"in_data_bytes", OML_UINT64_VALUE},
    {"out_data_bytes", OML_UINT64_VALUE},
    {"in_ctrl_bytes", OML_UINT64_VALUE},
    {"out_ctrl_bytes", OML_UINT64_VALUE},
    {NULL, (OmlValueT)0}
};

#ifdef __cplusplus
}
#endif 

class routing_stats{

private:
	OmlMP* oml_mp;
	string node_id;
	uint64_t t_stamp;
	uint64_t in_chunks;
	uint64_t out_chunks;
	uint64_t in_ctrl_msgs;
	uint64_t out_ctrl_msgs;
	uint64_t stored_chunks;
	uint64_t error_chunks;
	uint64_t dropped_chunks;
	uint64_t in_data_bytes;
	uint64_t out_data_bytes;
	uint64_t in_ctrl_bytes;
	uint64_t out_ctrl_bytes;

public:

	routing_stats(const char* id){

            node_id = id;
            if((oml_mp = 
                omlc_add_mp("routing_stats", routing_stats_mpd)) == NULL){
                cout << "FATAL: " 
                << "Unable to add routing_stats OML measurement pt." 
                << endl;
                exit(1);
            }
            in_chunks = out_chunks = 0;
            in_ctrl_msgs = out_ctrl_msgs = 0;
            stored_chunks = error_chunks = dropped_chunks = 0;
            in_data_bytes = out_data_bytes = 0;
            in_ctrl_bytes = out_ctrl_bytes = 0;
	}	

	void inject_into_oml(unsigned mp_index);
	void read_from_click(int sockfd);
};

#endif //ROUTING_STATS_H
