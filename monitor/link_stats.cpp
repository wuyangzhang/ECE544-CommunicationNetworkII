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
#include "link_stats.h"
#include "control_socket.h"

void link_stats::inject_into_oml(unsigned mp_index) {

    for (int i = 0; i < link_cnt; i++) {

        OmlValueU v[13];

        omlc_zero_array(v, 13);

        omlc_set_uint32(v[0], mp_index);
        omlc_set_string(v[1], links[i].label.c_str());
        omlc_set_string(v[2], node_id.c_str());
        omlc_set_string(v[3], links[i].nbr_id.c_str());
        omlc_set_double(v[4], links[i].bitrate_mbps);
        omlc_set_uint32(v[5], links[i].s_ett_usec);
        omlc_set_uint32(v[6], links[i].l_ett_usec);
        omlc_set_int64(v[7], links[i].in_pkts);
        omlc_set_int64(v[8], links[i].out_pkts);
        omlc_set_int64(v[9], links[i].in_bytes);
        omlc_set_int64(v[10], links[i].out_bytes);
        omlc_set_double(v[11], links[i].in_tput_mbps);
        omlc_set_double(v[12], links[i].out_tput_mbps);

        omlc_inject(oml_mp, v);

        omlc_reset_string(v[1]);
        omlc_reset_string(v[2]);
        omlc_reset_string(v[3]);
    }

#ifdef DEBUG
    string nbr_id_set = "";
    string bitrate_mbps_set = "";
    string s_ett_usec_set = "";
    string l_ett_usec_set = "";
    string in_pkts_set = "";
    string out_pkts_set = "";
    string in_bytes_set = "";
    string out_bytes_set = "";
    string in_tput_mbps_set = "";
    string out_tput_mbps_set = "";
    
    char tmp[256];

    for (int i = 0; i < link_cnt; i++) {
        nbr_id_set += links[i].nbr_id;
        sprintf(tmp, "%f", links[i].bitrate_mbps);
        bitrate_mbps_set += tmp;
        sprintf(tmp, "%u", links[i].s_ett_usec);
        s_ett_usec_set += tmp;
        sprintf(tmp, "%u", links[i].l_ett_usec);
        l_ett_usec_set += tmp;
        sprintf(tmp, "%llu", links[i].in_pkts);
        in_pkts_set += tmp;
        sprintf(tmp, "%llu", links[i].out_pkts);
        out_pkts_set += tmp;
        sprintf(tmp, "%llu", links[i].in_bytes);
        in_bytes_set += tmp;
        sprintf(tmp, "%llu", links[i].out_bytes);
        out_bytes_set += tmp;
        sprintf(tmp, "%f", links[i].in_tput_mbps);
        in_tput_mbps_set += tmp;
        sprintf(tmp, "%f", links[i].out_tput_mbps);
        out_tput_mbps_set += tmp;

        if (i != (link_cnt - 1)) {
            nbr_id_set += ",";
            bitrate_mbps_set += ",";
            s_ett_usec_set += ",";
            l_ett_usec_set += ",";
            in_pkts_set += ",";
            out_pkts_set += ",";
            in_bytes_set += ",";
            out_bytes_set += ",";
            in_tput_mbps_set += ",";
            out_tput_mbps_set += ",";
        }
    }


    char msg[4096];
    sprintf(msg, "LINKS(node: %s):\n"
        "ts cnt [nbrs] [bitrate] [s_ett] [l_ett] [i-P] [o-P] [i-B] [o-B] [i-mbps] [o-mbps] \n"
        "%llu %u [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s]", 
        node_id.c_str(),
        t_stamp,
        link_cnt,
        nbr_id_set.c_str(),
        bitrate_mbps_set.c_str(),
        s_ett_usec_set.c_str(), l_ett_usec_set.c_str(),
        in_pkts_set.c_str(), out_pkts_set.c_str(), 
        in_bytes_set.c_str(), out_bytes_set.c_str(), 
        in_tput_mbps_set.c_str(), out_tput_mbps_set.c_str()
        );
    cout << "DEBUG: " << msg << endl;
#endif //DEBUG

}


int link_stats::get_link_index(string guid) {

    for (int i = 0; i < link_cnt; i++) {
        if(!links[i].nbr_id.compare(guid))return i;
    }
    return -1; // not found
}

void link_stats::read_from_click(int sockfd) {

	char buf[1024];

	timeval nowtime;
	gettimeofday(&nowtime, NULL);
	t_stamp = nowtime.tv_sec;

	control_socket::sendCommand(sockfd, "read nbr_tbl.neighbor_table\n");
	control_socket::receiveData(sockfd, buf, 1024, 
        control_socket::NEIGHBOR_TABLE_QUERY);
	//format: [guid1,sett1,lett1],[guid2,sett2,lett2],...,[]
	char *curr = buf;
	char *begin = NULL;
	int lnk_index = 0;
	int index = -1;
	link_cnt = 0;
	while (*curr) {
            if (begin == NULL && *curr == '[') {
                begin = curr;
                index = 0;
            } else if(begin != NULL && *curr == ']') {
                if(index == 1){ curr++; continue; }//empty entry '[]'
                string link_str(begin+1, index-1);
                std::istringstream iss(link_str);
                string token = "";
                //nbr id
                if (getline(iss, token, ',')) {
                        links[lnk_index].nbr_id = token;
                        //link label: node_id->nbr_id
                        links[lnk_index].label = node_id + "->" + token;
                } else {
                        printf("error parsing neigbor table: %s\n", buf);
                        break;
                }
                //s_ett
                if (getline(iss, token, ',')) {
                        links[lnk_index].s_ett_usec 
                                                = atol(token.c_str());
                } else {
                        printf("error parsing neigbor table: %s\n", buf);
                        break;
                }
                //l_ett
                if (getline(iss, token, ',')) {
                        links[lnk_index].l_ett_usec = atol(token.c_str());
                } else {
                        printf("error parsing neigbor table: %s\n", buf);
                        break;
                }
                //TODO currently no bitrate reported by Click
                links[lnk_index].bitrate_mbps = 0.0;
                //not reported together, zero out
                links[lnk_index].in_pkts = 0;
                links[lnk_index].in_bytes = 0;
                links[lnk_index].in_tput_mbps = 0.0;
                links[lnk_index].out_pkts = 0;
                links[lnk_index].out_bytes = 0;
                links[lnk_index].out_tput_mbps = 0.0;

                lnk_index++; 

                link_cnt++;
                begin = NULL;
            } else if (begin == NULL && *curr == ']') {
                printf("error parsing neigbor table: %s\n", buf);
                break;
            }
            curr++;
            index++;
	}

    //per link traffic stats obtained from the port learning/resolving element
    //TODO we'd want to aggregate these stats along with link state data
    //above into a single origin for consistent and simple export

	control_socket::sendCommand(sockfd, "read paint.port_traffic_table\n");
	control_socket::receiveData(sockfd, buf, 1024, 
        control_socket::PORT_TRAFFIC_TABLE_QUERY);
	//format: [guid1,port_id1,out_pkts1,out_bytes1],
    //          [guid2,port_id1,out_pkts1,out_bytes1],...,[]
	curr = buf;
	begin = NULL;
	lnk_index = 0;
	while (*curr) {
            if (begin == NULL && *curr == '[') {
                begin = curr;
                index = 0;
            } else if(begin != NULL && *curr == ']') {
                if(index == 1){ curr++; continue; }//empty entry '[]'
                string link_str(begin+1, index-1);
                std::istringstream iss(link_str);
                string token = "";
                if (getline(iss, token, ',')) {
                        //nbr id, obtain index into previously filled 
                        //link entries from GSTARs neighbor table
                        //abort parsing if link isn't announced by nbr tbl
                        if((lnk_index = get_link_index(token)) < 0) {
                            printf("WARN: Can't find guid %s from "
                                "traffic table in link table\n", token.c_str());
                        }
                } else {
                        printf("error parsing traffic table: %s\n", buf);
                        break;
                }
                if(lnk_index >= 0) {
                    //port_id
                    if (getline(iss, token, ',')) {
                            //TODO not considered for stats yet
                    } else {
                            printf("error parsing traffic table: %s\n", buf);
                            break;
                    }
                    //out_pkts
                    if (getline(iss, token, ',')) {
                            links[lnk_index].out_pkts = atol(token.c_str());
                    } else {
                            printf("error parsing traffic table: %s\n", buf);
                            break;
                    }
                    //out_bytes
                    if (getline(iss, token, ',')) {
                            links[lnk_index].out_bytes = atol(token.c_str());
                    } else {
                            printf("error parsing traffic table: %s\n", buf);
                            break;
                    }
                    //out_tput_mbps
                    //TODO require to remember bytes cnt from prev timestamp 
                }
                begin = NULL;
            } else if (begin == NULL && *curr == ']') {
                printf("error parsing traffic table: %s\n", buf);
                break;
            }
            curr++;
            index++;
	}
}
