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
#include "packet_stats.h"
#include "control_socket.h"

void packet_stats::inject_into_oml(unsigned mp_index){

    OmlValueU v[11];

    for (int i = 0; i < num_ports; i++) {

        omlc_zero_array(v, 11);

        omlc_set_uint32(v[0], mp_index);
        omlc_set_string(v[1], node_id.c_str());
        omlc_set_string(v[2], pps[i].port_id.c_str());
        omlc_set_int64(v[3], pps[i].in_pkts);
        omlc_set_int64(v[4], pps[i].out_pkts);
        omlc_set_int64(v[5], pps[i].errors);
        omlc_set_int64(v[6], pps[i].dropped);
        omlc_set_int64(v[7], pps[i].in_bytes);
        omlc_set_int64(v[8], pps[i].out_bytes);
        omlc_set_double(v[9], pps[i].in_tput_mbps);
        omlc_set_double(v[10], pps[i].out_tput_mbps);

        omlc_inject(oml_mp, v);

        omlc_reset_string(v[1]);
        omlc_reset_string(v[2]);

#ifdef DEBUG
        char msg[500];
        sprintf(msg, "PKTS(node %s: port %s):\n"
            "ts i-P o-P er-P dr-P i-B o-B i-mbps o-mbps\n" 
            "%llu %llu %llu %llu %llu %llu %llu %.1f %.1f", 
            node_id.c_str(),
            pps[i].port_id.c_str(),
            t_stamp,
            pps[i].in_pkts, pps[i].out_pkts, 
            pps[i].errors, pps[i].dropped,
            pps[i].in_bytes, pps[i].out_bytes,
            pps[i].in_tput_mbps, pps[i].out_tput_mbps
            );
        cout << "DEBUG: " << msg << endl;
#endif //DEBUG
    }

    //reset reported stats
    num_ports = 0;
}

void packet_stats::read_from_click(int sockfd){

    char buf[100];

    timeval nowtime;
    gettimeofday(&nowtime, NULL);
    t_stamp = nowtime.tv_sec; 

    //TODO gather stats for all reported ports
    //for now, these stats are aggregated 
    num_ports = 1;
    pps[0].port_id = "0";
    // in packet count
    control_socket::sendCommand(sockfd, "read inCntr_pkt.count\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::IN_PKT_COUNT_QUERY);
    pps[0].in_pkts = atol(buf);
    // out packet count
    control_socket::sendCommand(sockfd, "read outCntr_pkt.count\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::OUT_PKT_COUNT_QUERY);
    pps[0].out_pkts = atol(buf);

    //TODO errors not being reported
    pps[0].errors = 0;

    //TODO this is only the kernel drops, internal buffer drops need to be
    //accounted as well
    control_socket::sendCommand(sockfd, "read fd_core.kernel_drops\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::DROPPED_PKT_COUNT_QUERY);
    pps[0].dropped = atol(buf);

    control_socket::sendCommand(sockfd, "read inCntr_pkt.byte_count\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::IN_BYTE_COUNT_QUERY);
    pps[0].in_bytes = atol(buf);

    control_socket::sendCommand(sockfd, "read outCntr_pkt.byte_count\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
            control_socket::OUT_BYTE_COUNT_QUERY);
    pps[0].out_bytes = atol(buf);

    // TODO: Counter element reports bit rate as a weighted avg. 
    // Alternatively, we could track byte count at prev timestamp and 
    // compute the tput for the period
    control_socket::sendCommand(sockfd, "read inCntr_pkt.bit_rate\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::IN_BIT_RATE_QUERY);
    pps[0].in_tput_mbps = atof(buf)/(1024 * 1024);

    control_socket::sendCommand(sockfd, "read outCntr_pkt.bit_rate\n"); 
    control_socket::receiveData(sockfd, buf, 100, 
        control_socket::OUT_BIT_RATE_QUERY);
    pps[0].out_tput_mbps = atof(buf)/(1024 * 1024);
}
