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
#include <click/error.hh>
#include <click/standard/scheduleinfo.hh>

#include <pthread.h>

#include "mfchunkmanager.hh"
#include "mffunctions.hh"
#include "mflogger.hh"

CLICK_DECLS

MF_ChunkManager::MF_ChunkManager() 
    : _cleanup_task(this), _cleanup_timer(&_cleanup_task), _cleanup_chk_id(0), 
     _cleanup_chk_cnt(0), _cleanup_running_flag(false), 
     _cleanup_threshhold(DEFAULT_CLEANUP_THRESHHOLD),
     _cleanup_percentage(DEFAULT_CLEANUP_PERCENTAGE),
     _cleanup_period_sec(DEFAULT_CLEANUP_PERIOD_SEC), 
     logger(MF_Logger::init()) {

  _chunkIDTable = new HashTable< uint32_t, Chunk* >();
  chunk_id = 1;  //chunk's index in table is starting from 1
  _dstGUIDTable = new HashTable< uint32_t, Vector<Chunk*>* >();
  pthread_mutex_init(&_chunkIDTable_mtx, NULL);
  pthread_mutex_init(&_dstGUIDTable_mtx, NULL); 
}

MF_ChunkManager::~MF_ChunkManager() {
  delete _chunkIDTable;
  delete _dstGUIDTable; 
}

int MF_ChunkManager::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_kparse(conf, this, errh,
      "CLEANUP_PERIOD_SEC", cpkP, cpUnsigned, &_cleanup_period_sec,
      "CLEANUP_THRESHHOLD", cpkP, cpUnsigned, &_cleanup_threshhold,
      "CLEANUP_PERCENTAGE", cpkP, cpUnsigned, &_cleanup_percentage,
    cpEnd) < 0)
    return -1;
  return 0;
}

int MF_ChunkManager::initialize(ErrorHandler *errh) {
  logger.log(MF_DEBUG, 
             "chk_mngr: cleanup period: %u sec, threshhold: %u, percentage: %u",
              _cleanup_period_sec, _cleanup_threshhold, _cleanup_percentage); 
  ScheduleInfo::initialize_task(this, &_cleanup_task, false, errh);
  _cleanup_timer.initialize(this);
  _cleanup_timer.schedule_after_sec(_cleanup_period_sec);
  return 0; 
}

bool MF_ChunkManager::run_task(Task *) {
  logger.log(MF_DEBUG, "chk_mngr: periodically clean up");
  if (!_cleanup_running_flag) {
    cleanup();
  } else {
    logger.log(MF_DEBUG, "chk_mngr: cleannup is running, next schedule time"); 
  }
  _cleanup_timer.reschedule_after_sec(_cleanup_period_sec); 
}

Chunk* MF_ChunkManager::alloc(uint32_t pkt_cnt) {
  Chunk *chunk = new Chunk(chunk_id, pkt_cnt);
  //chunk->setChunkID(chunk_id);
  logger.log( MF_DEBUG, 
    "chk_mngr: allocate a new chunk, chunk_id: %u", chunk_id); 
  pthread_mutex_lock(&_chunkIDTable_mtx);
  _chunkIDTable->find_insert( chunk_id, chunk);
  chunk_id++;
  pthread_mutex_unlock(&_chunkIDTable_mtx); 
  return chunk; 
}

bool MF_ChunkManager::addToDstGUIDTable(uint32_t cid) {
  Chunk *chunk = get(cid, ChunkStatus::ST_INITIALIZED);
  if(!chunk) {
    logger.log( MF_ERROR, 
      "chk_mngr: chunks in two table are different"); 
    return false;  
  } else {
    uint32_t dst_GUID = chunk->getDstGUID().to_int();
    //insert into a hashtable named dstGUIDTable, key is dst_guid 
    DstGUIDTable::iterator it = _dstGUIDTable->find(dst_GUID);
    //if there are multiple chunks with same dst guid in the table 

    if( it != _dstGUIDTable->end() ) {
      it.value()->push_back(chunk);
      logger.log(MF_DEBUG,
                 "chk_mngr: dstGUID_table have dst_GUID: %u entry", dst_GUID);
    } else {
      Vector<Chunk*> *cv = new Vector<Chunk*>();
      cv->push_back(chunk);
      pthread_mutex_lock(&_dstGUIDTable_mtx);
      _dstGUIDTable->find_insert(dst_GUID, cv);
      pthread_mutex_unlock(&_dstGUIDTable_mtx);
      logger.log(MF_DEBUG,
                 "chk_mngr: add chunk_id %u to dst_GUID: %u entry in dstGUID_table",
                 cid, dst_GUID);
    }
    return true; 
  }
}

bool MF_ChunkManager::removeFromDstGUIDTable(uint32_t cid, uint32_t dst_GUID) {
  Chunk *chunk = get(cid, ChunkStatus::ST_INITIALIZED);
  if(!chunk) {
    logger.log( MF_ERROR,
      "chk_mngr: chunks in two table are different");
    return false;
  } else {
    //insert into a hashtable named dstGUIDTable, key is dst_guid
    DstGUIDTable::iterator it = _dstGUIDTable->find(dst_GUID);
    //if there are multiple chunks with same dst guid in the table

    if( it != _dstGUIDTable->end() ) {
      logger.log(MF_DEBUG,
                 "chk_mngr: dstGUID_table have dst_GUID: %u entry", dst_GUID);
      pthread_mutex_lock(&_dstGUIDTable_mtx);
      Vector<Chunk*>::iterator it2 = it.value()->begin();
      bool deleted = false;
	  while (it2 != it.value()->end() && !deleted) {
		if ((*it2)->getChunkID() == cid) {
		  it2 = it.value()->erase(it2);
		  deleted = true;
		  break;
		} else {
		  ++it2;
		}
	  }
      pthread_mutex_unlock(&_dstGUIDTable_mtx);
    } else {
      logger.log(MF_DEBUG,
                 "chk_mngr: dst_GUID: %u does not have an entry in the table",
                 dst_GUID);
      return false;
    }
    return true;
  }
}

Chunk* MF_ChunkManager::alloc(Packet * first_pkt, uint32_t pkt_cnt, 
                              uint16_t status_mask) {
  Vector<Packet*> *pkt_vec = new Vector<Packet*> (pkt_cnt, NULL);
  pkt_vec->at(0) = first_pkt;
  return alloc(pkt_vec, status_mask);  
}

Chunk* MF_ChunkManager::alloc(Packet * first_pkt, uint32_t pkt_cnt, 
                              const ChunkStatus &status_mask) {
  return alloc(first_pkt, pkt_cnt, status_mask.get()); 
}

Chunk* MF_ChunkManager::alloc(Vector<Packet*> *pkt_vec, uint16_t status_mask) {
  if (_chunkIDTable->find(chunk_id) != _chunkIDTable->end()) {
    logger.log( MF_ERROR, "chk_mngr: chunk_id exists, cannot allocate chunk");
    return NULL;
  }
  //TODO: chunk ID should be accessed only in a thread safe area
  Chunk *chunk = new Chunk(chunk_id, pkt_vec);
  uint32_t dst_GUID = chunk->getDstGUID().to_int();
  //chunk->setChunkID(chunk_id);
  //chunk->setStatus(status_mask);
  logger.log(MF_DEBUG,
    "chk_mngr: allocate a new chunk with dst_GUID: %u, and chunk_Id %u",
    dst_GUID, chunk_id);
  //insert into a hashtable named _chunkIDTable, key is ChunkId
  pthread_mutex_lock(&_chunkIDTable_mtx);
  _chunkIDTable->find_insert(chunk_id, chunk);
  chunk_id++;
  pthread_mutex_unlock(&_chunkIDTable_mtx);

  //insert into a hashtable named dstGUIDTable, key is dst_guid 
  HashTable<uint32_t, Vector<Chunk*>* >::iterator it =
                                           _dstGUIDTable->find(dst_GUID);
  //if there are multiple chunks with same dst guid in the table 
  pthread_mutex_lock(&_dstGUIDTable_mtx);
  if( it != _dstGUIDTable->end() ) {
    it.value()->push_back(chunk);
    logger.log(MF_DEBUG,
      "chk_mngr: dstGUID_table have dst_GUID: %u entry", dst_GUID);
  } else {
    Vector<Chunk*> *cv = new Vector<Chunk*>();
    cv->push_back(chunk);
    _dstGUIDTable->find_insert(dst_GUID, cv);
    logger.log(MF_DEBUG,
      "chk_mngr: add dst_GUID: %u entry to dstGUID_table",
      dst_GUID);
  }
  pthread_mutex_unlock(&_dstGUIDTable_mtx);
  return chunk;
}

Chunk* MF_ChunkManager::alloc (Vector<Packet*> *pkt_vec, 
                                 const ChunkStatus &status_mask) {
  return alloc(pkt_vec, status_mask.get()); 
}

Chunk* MF_ChunkManager::alloc_dup (Chunk* chunk) {
  Chunk* dup_chunk = new Chunk(*chunk);
  dup_chunk->setChunkID(chunk_id); 
  uint32_t dst_GUID = chunk->getDstGUID().to_int(); 
  logger.log(MF_DEBUG, 
    "chk_mngr: allocate a duplicated chunk with new chunk_id: %u",
    dup_chunk->getChunkID());
  pthread_mutex_lock(&_chunkIDTable_mtx);
  _chunkIDTable->find_insert( chunk_id, chunk);
  chunk_id++;
  pthread_mutex_unlock(&_chunkIDTable_mtx);
  //insert into a hashtable named dstGUIDTable, key is dst_guid 
  HashTable<uint32_t, Vector<Chunk*>* >::iterator it =
                                           _dstGUIDTable->find(dst_GUID);
  //if there are multiple chunks with same dst guid in the table 
  pthread_mutex_lock(&_dstGUIDTable_mtx);
  if( it != _dstGUIDTable->end() ) {
    it.value()->push_back(chunk);
    logger.log(MF_DEBUG,
      "chk_mngr: dstGUID_table have dst_GUID: %u entry", dst_GUID);
  } else {
    Vector<Chunk*> *cv = new Vector<Chunk*>();
    cv->push_back(chunk);
    _dstGUIDTable->find_insert(dst_GUID, cv);
    logger.log(MF_DEBUG,
      "chk_mngr: add dst_GUID: %u entry to dstGUID_table",
      dst_GUID);
  }
  pthread_mutex_unlock(&_dstGUIDTable_mtx);
  return dup_chunk; 
}

void MF_ChunkManager::push_to_port(Packet *pkt, uint32_t port_num, 
                                     uint32_t delay_msec) {
  chk_internal_trans_t *chk_trans = (chk_internal_trans_t*)pkt->data();
  switch (port_num) {
  case PORT_TO_NET_BINDER:
    logger.log( MF_INFO,
      "chk_mngr: push chunk sid: %u, chunk_id %u, to net_binder",
      ntohl(chk_trans->sid), chk_trans->chunk_id);
    break;
  case PORT_TO_SEG:
    logger.log(MF_INFO,
      "chk_mngr: push chunk sid: %u, chunk_id: %u, to segmentor",
      ntohl(chk_trans->sid), chk_trans->chunk_id);
    break; 
  default:
    logger.log(MF_ERROR,
      "chk_mngr: push chunk sid %u, chunk_id %u to undefined port %u",
      ntohl(chk_trans->sid), chk_trans->chunk_id, port_num);
    break; 
  }

  if (delay_msec == 0) {    //default: push pkt immediately
    output(port_num).push(pkt);
    return;
  }

  timer_data_t *td = (timer_data_t*)malloc(sizeof(timer_data_t));
  td->me = this;
  td->pkt = pkt;
  td->port_num = port_num;
  Timer *delay_timer = new Timer(&MF_ChunkManager::handleDelayPush, (void*)td);
  delay_timer->initialize(this); 
  delay_timer->schedule_after_msec(delay_msec);
}

void MF_ChunkManager::handleDelayPush(Timer *timer, void *data) {
  timer_data_t *td = (timer_data_t*)data; 
  td->me->output(td->port_num).push(td->pkt); 
  delete timer; 
}


Chunk* MF_ChunkManager::get( uint32_t cid, uint16_t status_mask = 0) {
  HashTable<uint32_t, Chunk*>::iterator it = _chunkIDTable->find(cid);
  if ( it != _chunkIDTable->end() ) {
    Chunk *chk_ret = it.value(); 
    if (chk_ret->getStatus().is(ChunkStatus::ST_READY_TO_DEL)) { 
      logger.log( MF_ERROR, 
        "chk_mngr: try to get a ST_READY_TO_DEL chunk with chunk_id: %u", 
         cid);
      return NULL;  
    } else if (chk_ret->getStatus().is(status_mask)) {
      logger.log(MF_DEBUG, 
        "chk_mngr: find chunk with id: %u and status %x", 
        cid, chk_ret->getStatus().get() );
      return chk_ret;
    } else {
      logger.log(MF_DEBUG,
        "chk_mngr: find chunk with id %u and status %x, "
        "but status doesn't match status_mask %x",
        cid, chk_ret->getStatus().get(), status_mask);
      return NULL;
    }
  } else {
    logger.log(MF_ERROR, 
      "chk_mngr: no chunk with id: %u in chunkIDTable", cid);
    return NULL;
  }
}

Chunk* MF_ChunkManager::get(uint32_t cid, 
                            const ChunkStatus &status_mask) {
  return get(cid, status_mask.get());  
}

uint32_t MF_ChunkManager::get(uint32_t dst_GUID, 
                              uint16_t status_mask, 
                              Vector<Chunk*> & vc) {
  HashTable<uint32_t, Vector<Chunk*> *>::iterator it = 
                                           _dstGUIDTable->find(dst_GUID);
  if (it != _dstGUIDTable->end()) {                          //if find entry
    for (Vector<Chunk*>::iterator it2 = it.value()->begin(); 
           it2 != it.value()->end(); ++it2) {
      if ((*it2)->getStatus().is(ChunkStatus::ST_READY_TO_DEL)) {
        logger.log( MF_DEBUG, 
          "chk_mngr: try to get a deleted chunk, skip, status %x, %x",
          status_mask, (*it2)->getStatus().get()); 
        continue;                    //skip chunks with status ST_READY_TO_DEL
      }
      if ((*it2)->getStatus().is(status_mask)) {
        vc.push_back(*it2);
      }
    }
    logger.log( MF_DEBUG, 
      "chk_mngr: find %u chunks with dst_GUID: %u, and status mask %x", 
       vc.size(), dst_GUID, status_mask);
    return vc.size();
  } else {   //no such entry
    logger.log( MF_DEBUG,  
      "chk_mngr: no chunk with dst_GUID %u In dstGUIDTable", dst_GUID);
    return 0;
  }
}

uint32_t MF_ChunkManager::get(uint32_t dst_GUID, 
                              const ChunkStatus &status, Vector<Chunk*> &vc) {
  return get(dst_GUID, status.get(), vc); 
}

void MF_ChunkManager::cleanup() {
  uint32_t cleanup_range = _cleanup_threshhold * _cleanup_percentage/100; 
  ChunkIDTable::iterator end_it = _chunkIDTable->end();
  ChunkIDTable::iterator it = _chunkIDTable->begin();
  //lock chunkIDTable before access
  logger.log(MF_TRACE, "chk_mngr: cleaning up..."); 
  pthread_mutex_lock(&_chunkIDTable_mtx);
  while (it != end_it) {
    Chunk *chunk_to_free = it.value();
    uint32_t cid = chunk_to_free->getChunkID();
    //if chunk status is ST_READY_TO_DEL, and cid is in the removable range
    if (chunk_to_free->getStatus().isReadyToDel() && 
           (cid > _cleanup_threshhold - cleanup_range) && 
           cid < _cleanup_chk_id && 
           (cid > _cleanup_chk_id - cleanup_range)) {
      delete chunk_to_free; 
      it = _chunkIDTable->erase(it);
      --_cleanup_chk_cnt;
      logger.log(MF_TRACE, 
                 "chk_mngr: CLEAN UP chunk id: %u, # of chunks to free: %u",
                 cid, _cleanup_chk_cnt); 
    } else {
      ++it; 
    }
  }
  //unlock table after cleanup
  pthread_mutex_unlock(&_chunkIDTable_mtx);
  logger.log(MF_TRACE, "chk_mngr: cleanup Done"); 
}

int MF_ChunkManager::removeData(Chunk *chunk) {
  uint32_t cid = chunk->getChunkID(); 
  uint32_t dst_GUID = chunk->getDstGUID().to_int();
  chunk->setStatus(ChunkStatus::ST_READY_TO_DEL); 
  DstGUIDTable::iterator it = _dstGUIDTable->find(dst_GUID);

  if (it != _dstGUIDTable->end()) {
    logger.log(MF_TRACE, "chk_mngr: Find chunk in dstGUIDTable");
    pthread_mutex_lock(&_dstGUIDTable_mtx);
    Vector<Chunk*>::iterator it2 = it.value()->begin();
    while (it2 != it.value()->end()) {
      if ((*it2)->getChunkID() == cid) {
        it2 = it.value()->erase(it2);
        break;
      } else {
        ++it2;
      }
    }
    if (it.value()->size() == 0) {     //all elements have been erased;
      _dstGUIDTable->erase(it);
    }
    pthread_mutex_unlock(&_dstGUIDTable_mtx);
  } else {
    logger.log(MF_ERROR,
               "chk_mngr: chunk is in chunkIDTable but not in dstGUIDTable");
    return 0;
  }

  //free momery
  Vector<Packet*> *vec_to_free = chunk->allPkts();
  logger.log(MF_DEBUG,
             "chk_mngr: deleting chunk %u , # of pkts %u",
             cid, vec_to_free->size());
  Vector<Packet*>::iterator vec_end = vec_to_free->end(); 
  for (Vector<Packet*>::iterator vec_it = vec_to_free->begin(); 
           vec_it != vec_end; ++vec_it) {
    Packet *pkt_to_free = *vec_it;
    assert(pkt_to_free);
    pkt_to_free->kill(); 
  }
  ++_cleanup_chk_cnt;
  //if _cleanup_chk_cnt reach the threshhold, cleanup 
  if (_cleanup_chk_cnt > _cleanup_threshhold && !_cleanup_running_flag) {
    logger.log(MF_INFO,
               "chk_mngr: # of ST_READY_TO_DEL chunks reaches THRESHHOLD: "
               "trigger cleanup method");
    _cleanup_chk_id = cid;
    _cleanup_running_flag = true; 
    cleanup();
    _cleanup_running_flag = false; 
  }
  
  return 1; 
}

/*
uint32_t MF_ChunkManager::remove(uint32_t cid) {
  HashTable<uint32_t, Chunk*>::iterator it = _chunkIDTable->find(cid);
  Chunk *chk_to_del = NULL; 
  if ( it != _chunkIDTable->end() ) {
    chk_to_del = it.value();
    assert(chk_to_del); 

    chk_to_del->setStatus(ChunkStatus::ST_READY_TO_DEL); //set to ST_READY_TO_DEL status
    logger.log(MF_DEBUG, 
               "chk_mngr: set target chunk_id: %u status: %x to ST_READY_TO_REMOVE status",
               cid, chk_to_del->getStatus().get());
    uint32_t dst_GUID = chk_to_del->getDstGUID().to_int();
    pthread_mutex_lock(&_chunkIDTable_mtx); 
    _chunkIDTable->erase(it);
    pthread_mutex_unlock(&_chunkIDTable_mtx); 
    
    HashTable<uint32_t, Vector<Chunk*> * >::iterator it2 = 
                                             _dstGUIDTable->find(dst_GUID);
    if (it2 != _dstGUIDTable->end()) {
      logger.log(MF_DEBUG, "chk_mngr: Find chunk in dstGUIDTable"); 
      pthread_mutex_lock(&_dstGUIDTable_mtx);
      Vector<Chunk*>::iterator it3 = it2.value()->begin();
      while (it3 != it2.value()->end()) {
        if ((*it3)->getChunkID() == cid) {
           it3 = it2.value()->erase(it3);
           break; 
        } else {
           ++it3; 
        }
      }
      if (it2.value()->size() == 0) {     //all elements have been erased;  
        _dstGUIDTable->erase(it2); 
      }
      pthread_mutex_unlock(&_dstGUIDTable_mtx);
    } else {
      logger.log( MF_ERROR, 
        "chk_mngr: chunk is in chunkIDTable but not in dstGUIDTable");
      return 0; 
    }
    //free momery
    Vector<Packet*> *vec_to_del = chk_to_del->allPkts();
    logger.log(MF_DEBUG, 
               "chk_mngr: deleting chunk %u , # of pkts %u",
               cid, vec_to_del->size()); 
    for (unsigned i = 0;i < vec_to_del->size(); ++i) {
      Packet *pkt_to_delete = vec_to_del->at(i);
      assert(pkt_to_delete);
      pkt_to_delete->kill();
    }
    delete chk_to_del; 
    chk_to_del = NULL; 
    logger.log(MF_DEBUG, 
      "chk_mngr: chunk %u is deleted successfully", cid); 
    return 1;              //delete successfully
  } else {
    logger.log(MF_ERROR,
      "chk_mngr: no chunk with id: %u in ChunkManager", cid); 
    return 0;            
  }
}
*/
/*
#if EXPLICIT_TEMPLATE_INSTANCES
template class Vector<Packet*>;
template class Vector<cache_entry*>;
#endif
*/
CLICK_ENDDECLS

EXPORT_ELEMENT(MF_ChunkManager)
