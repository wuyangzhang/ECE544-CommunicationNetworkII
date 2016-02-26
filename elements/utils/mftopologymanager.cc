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
#include <click/config.h>
#include <click/confparse.hh>
#include <click/router.hh>
#include <click/etheraddress.hh>
#include <click/straccum.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/handlercall.hh>
#include <click/packet_anno.hh>
#include <click/userutils.hh>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mftopologymanager.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_TopologyManager::MF_TopologyManager(): num_neighbors(0), logger(MF_Logger::init()){
}

MF_TopologyManager::~MF_TopologyManager(){
}

int MF_TopologyManager::configure(Vector<String> &conf, ErrorHandler *errh){

    int ret;

    ret = cp_va_kparse(conf, this, errh,
            "MY_GUID", cpkP+cpkM, cpInteger, &my_guid,   
            //full path
            "TOPO_FILE", cpkP+cpkM, cpString, &topo_file,
            "ARP_TABLE", cpkP+cpkM, cpElement, &arp_tbl_p,
            cpEnd);

    FILE * file = fopen(topo_file.c_str(), "r");

    if(!file){
        logger.log(MF_CRITICAL, 
            "topo_mngr: Could not open %s", topo_file.c_str());
        //continue without topology enforcement
        return ret;
    }
    logger.log(MF_INFO, 
    	"topo_mngr: Using topology file '%s'", topo_file.c_str());

    uint32_t guid, nbr_cnt, nbr_guid;

    bool done = false;

    //max size of a line in the topo file
    int TMP_LINE_SIZE = 1024;
    char line[TMP_LINE_SIZE];
    char *buf;
    int line_no = 1;
    int res; 
    while(!feof(file)){
        //skip over empty lines
        if((res = fscanf(file, "%a[ \t\v\f]\n", &buf)) == 1) {
            free(buf);
            logger.log(MF_TRACE, 
                "topo_mngr: skipping empty line in topo file at line# %u",
                line_no);
            line_no++;
            continue;
        }
        //skip over comment lines
        if((res = fscanf(file, " #%a[^\n]\n", &buf)) == 1) {
            free(buf);
            logger.log(MF_TRACE, 
                "topo_mngr: skipping comment line in topo file at line# %u",
                line_no);
            line_no++;
            continue;
        }
        // read 'guid nbr_cnt'
        if((res = fscanf(file, " %u %u", &guid, &nbr_cnt)) == EOF) {
            break;
        } else if (res != 2) {
            logger.log(MF_FATAL, 
                "topo_mngr: Error 1 while reading topo file at line %u",
                line_no);
            exit(-1);

        }

        if (guid == my_guid) {
            num_neighbors = nbr_cnt;
            logger.log(MF_INFO, 
                "topo_mngr: Total neighbors: %u", num_neighbors);
        }
        for(unsigned i = 0; i < nbr_cnt; i++){
            if(fscanf(file,"%d", &nbr_guid) != 1){
                logger.log(MF_FATAL, 
                    "topo_mngr: Error 2 while reading topo file at line %u",
                    line_no);
                exit(-1);
            }
            if(guid == my_guid){
                guid_array[i] = nbr_guid;
                logger.log(MF_INFO, 
                    "topo_mngr: Neighbor guid: %u", guid_array[i]);
            }
        }
        //skip anything else on this line
        if(!fgets(line, TMP_LINE_SIZE, file)) {
            logger.log(MF_FATAL, 
                "topo_mngr: Error while reading topo file at line %u",
                line_no);
            exit(-1);
        }
        line_no++;
    }
    return ret;
}

bool MF_TopologyManager::is_neighbor(uint32_t r_guid){

    for(unsigned i=0; i < num_neighbors; i++){
        if(guid_array[i] == r_guid){
            return true;
        }
    }
    return false;
}

void MF_TopologyManager::push(int port, Packet *p){

    assert(port == 0);

    rcvd_pkt * header = (rcvd_pkt *) p->data();

    uint32_t type = ntohl(header->type);
    /* 
     * All MF pkts have first word (a byte should do) as type 
     * field, but only rtg control packets have source guids in 
     * their L2 payloads. The remaining require a L2 src check.
     * Skip check on non-control pkts since they are not bcast
     * and are usually initiated after control link is est. 
     */

    if(!(type == LP_PKT || type == LP_ACK_PKT || type == LSA_PKT)){
        output(0).push(p);
        return;
    }
    uint32_t src_guid = ntohl(header->source);

    if(is_neighbor(src_guid)){
        output(0).push(p);
    }else{
        //kept at DEBUG since same LAN expts are the common case
        logger.log(MF_DEBUG, 
            "topo_mngr: Dropping pkt from non-neighbor GUID %u! (safely ignore if node on same LAN)",
            src_guid);
        p->kill();      		
    }

}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_TopologyManager)
