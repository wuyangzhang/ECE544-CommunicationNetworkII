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
#include "routing_stats.h"
#include "control_socket.h"

void routing_stats::inject_into_oml(unsigned mp_index){

    OmlValueU v[13];

    omlc_zero_array(v, 13);

    omlc_set_uint32(v[0], mp_index);
    omlc_set_string(v[1], node_id.c_str());
    omlc_set_uint64(v[2], in_chunks);
    omlc_set_uint64(v[3], out_chunks);
    omlc_set_uint64(v[4], in_ctrl_msgs);
    omlc_set_uint64(v[5], out_ctrl_msgs);
    omlc_set_uint64(v[6], stored_chunks);
    omlc_set_uint64(v[7], error_chunks);
    omlc_set_uint64(v[8], dropped_chunks);
    omlc_set_uint64(v[9], in_data_bytes);
    omlc_set_uint64(v[10], out_data_bytes);
    omlc_set_uint64(v[11], in_ctrl_bytes);
    omlc_set_uint64(v[12], out_ctrl_bytes);

    omlc_inject(oml_mp, v);

    omlc_reset_string(v[1]);

#ifdef DEBUG
    char msg[500];
    sprintf(msg, "ROUTING(node: %s):\n"
        "ts i-C o-C s-C e-C d-C i-dB o-dB i-cP o-cP i-cB o-cB\n"
        "%llu %llu %llu %llu %llu %llu %llu %llu %llu %lld %llu %llu", 
        node_id.c_str(),
        t_stamp,
        in_chunks, out_chunks, 
        stored_chunks, error_chunks, dropped_chunks,
        in_data_bytes, out_data_bytes,
        in_ctrl_msgs, out_ctrl_msgs, 
        in_ctrl_bytes, out_ctrl_bytes
        );
    cout << "DEBUG: " << msg << endl;
#endif
}

void routing_stats::read_from_click(int sockfd){

	char buf[100];

	timeval nowtime;
	gettimeofday(&nowtime, NULL);
	t_stamp = nowtime.tv_sec;

	control_socket::sendCommand(sockfd, "read inCntr_chunk.count\n"); 
	control_socket::receiveData(sockfd, buf, 100, 
            control_socket::RCVD_CHK_COUNT_QUERY);
	in_chunks = atol(buf);

	control_socket::sendCommand(sockfd, "read inCntr_chunk.byte_count\n"); 
	control_socket::receiveData(sockfd, buf, 100, 
            control_socket::RCVD_CHK_BYTES_QUERY);
	in_data_bytes = atol(buf);


	control_socket::sendCommand(sockfd, "read seg.sent_chk_count\n"); 
	control_socket::receiveData(sockfd, buf, 100, 
            control_socket::SENT_CHK_COUNT_QUERY);
	out_chunks = atol(buf);
        //TODO: obtain stat directly once chunk manager is used for storage
	stored_chunks = in_chunks - out_chunks;
}
