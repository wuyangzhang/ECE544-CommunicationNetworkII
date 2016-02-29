
/*

compress/uncompress functions requires a pre-installation of the 'libbz2-dev' library

*/
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ether.h>
#include <click/standard/scheduleinfo.hh>
#include <stdio.h>
#include <click/vector.hh>
#include <fcntl.h>
#include <unistd.h>

//#include <click/args.hh>


#include "MF_Computing.hh"
#include "click_MF.hh"
#include "zlib.h"
#include "bzlib.h"

#include "chunk_payload_processing.hh"

#define COMPRESSION 1
#define UNCOMPRESS 2
#define ENCRYPTION 3
#define DECRYPTION 4

#define PKT_NUM_BYTES 1024 // how many bytes a data pkt's payload carries


CLICK_DECLS

MF_Computing::MF_Computing()
{
	//FILE *fp;
}

MF_Computing::~MF_Computing()
{
}


int MF_Computing::configure(Vector<String> &conf, ErrorHandler *errh) {
	if(cp_va_kparse(conf, this, errh,
			"SERVICE_TYPE", cpkP+cpkM, cpUnsigned, &_service_type,
			cpEnd) < 0)
		return -1;
	
	return 0;
}

Packet * MF_Computing::simple_action(Packet *p)
//void MF_CRC::push(Packet *p)
{

	routing_datagram *dgram = (routing_datagram*)(p->data());
	
	Vector<Packet*> *cur_vec = dgram->all_pkts;
	pkt *cur_pkt;	
	Packet *cur_click_pkt;
	
    /* add CRC computation here */
    char crcnum=0;
    char *payload;
    unsigned int ii, jj;
    
    char whole_chunk_payload[2000000];
    char processed_payload[2000000];
    //char zipped_payload[2000000];
    
    int whole_chunk_pos = 0;
    int current_hopID;
    
    int original_pkt_count = p->anno_u32(4);
    
    char r_t_header[ROUTING_HEADER_SIZE+TRANS_HEADER_SIZE];
    routing_header *rheader;
	transport_header *theader;
    
    for(ii=0;ii<cur_vec->size();ii++){
        if (ii==0) //first pkt in a chunk
        {
            cur_pkt = (pkt*)((*cur_vec)[0]->data());
            rheader = (routing_header*)(cur_pkt->data);
            current_hopID = cur_pkt->hop_ID; // pick up the current hopID
            
            click_chatter("-> DST GUID: %d, SRC GUID: %d", ntohl(rheader->dst_GUID), ntohl(rheader->src_GUID));
            payload = (char *)(cur_pkt->data);
            memcpy(r_t_header, payload, ROUTING_HEADER_SIZE+TRANS_HEADER_SIZE);
            payload += ROUTING_HEADER_SIZE+TRANS_HEADER_SIZE;
            for(jj=0;jj<cur_pkt->pld_size;jj++)
            {
                crcnum+=payload[jj];
                whole_chunk_payload[whole_chunk_pos] = payload[jj];
                whole_chunk_pos++;
            }
        }
        else
        {
            cur_pkt = (pkt*)((*cur_vec)[ii]->data());
            payload=cur_pkt->data;
            for(jj=0;jj<cur_pkt->pld_size;jj++)
            {
                crcnum+=payload[jj];
                whole_chunk_payload[whole_chunk_pos] = payload[jj];           
                whole_chunk_pos++;
            }
        }
    }
    
    //remove original pkts	
  	while(cur_vec->size()>0)
  	{
		(*cur_vec)[0]->kill();
		cur_vec->pop_front();  	
	}
  	
  	click_chatter("--> (chunk length:%d, pkt num:%d, CRC:%d)", whole_chunk_pos, original_pkt_count, crcnum);
    //output(0).push(p);
    
    
    // whole_chunk[0, 1, ..., whole_chunk_pos - 1] -> payload of the original chunk
    //for(ii=0;ii<whole_chunk_pos;ii++)
    //		processed_payload[ii] = whole_chunk_payload[ii];
    
    //Byte *compr, *uncompr;
    unsigned int processed_Len, err; /* don't overflow on MSDOS */  //  long comprLen
    //test the zipping here...
    //err = compress(zipped_payload, &comprLen, (const Bytef*)processed_payload, whole_chunk_pos);
    
    //comprLen=2000000;
    //BZ2_bzBuffToBuffCompress(zipped_payload, &comprLen, processed_payload, whole_chunk_pos, 5, 0, 30);
    
    
    
    
	/* computing service here */
	computing_service(_service_type, processed_payload, &processed_Len, whole_chunk_payload, whole_chunk_pos);
        
    //computing_service(unsigned int computing_serv, char *processed_payload, unsigned int *processed_length, char *original_payload, unsigned int ori_length)
    
    
    
    ////////////////////////
    
    // clear all packets : dgram->all_pkts->clear()
    // add a packet : dgram->all_pkts->push_back(_pkt)
    // make new packet : _pkt = Packet::make(sizeof(click_ether), 0, PKT_SIZE + PKT_NUM_BYTES+1, 0); {headroom, 1041, tailroom}
    

	int cur_pkt_num = 0;
	int remaining_byte = processed_Len;
	int max_pkt_payload;
	int cur_bytes = 0;
	do
	{
		_pkt = Packet::make(sizeof(click_ether), 0, PKT_SIZE + PKT_NUM_BYTES+1, 0);
		pkt *pheader = (pkt *)(_pkt->data());
		pheader->type = DATA_PKT;
		pheader->hop_ID = current_hopID;
		pheader->seq_num = cur_pkt_num;
		
		if(cur_pkt_num == 0)
		{
			// first pkt
			max_pkt_payload = PKT_NUM_BYTES - TRANS_HEADER_SIZE - ROUTING_HEADER_SIZE;
		}
		else
		{
			// ordinary pkts
			max_pkt_payload = PKT_NUM_BYTES;
		}
		
		pheader->pld_size = ((max_pkt_payload < remaining_byte) ? max_pkt_payload : remaining_byte);
				
		if (cur_pkt_num == 0)
		{
			memcpy(pheader->data, r_t_header, ROUTING_HEADER_SIZE+TRANS_HEADER_SIZE);
			memcpy(pheader->data+ROUTING_HEADER_SIZE+TRANS_HEADER_SIZE, processed_payload+cur_bytes, pheader->pld_size);
		}
		else
		{
			memcpy(pheader->data, processed_payload+cur_bytes, pheader->pld_size);
		}
		
		cur_pkt_num++;
		cur_bytes += pheader->pld_size;
		remaining_byte -= pheader->pld_size;
		
		// add the newly generated packet
		dgram->all_pkts->push_back(_pkt);
		
	} while (remaining_byte > 0);
	
  	click_chatter("*** new pkt num:%d ***", cur_pkt_num);
  	p->set_anno_u32(4, cur_pkt_num);
    
    return(p);

}

CLICK_DECLS
EXPORT_ELEMENT(MF_Computing)
ELEMENT_REQUIRES(userlevel)
ELEMENT_LIBS(-L/usr/lib/ -lbz2) // library : /usr/lib/libz2.so

// apt-get install libbz2-dev
