#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include <click/args.hh>

#include "mfassoctable.hh"

CLICK_DECLS
MF_AssocTable::MF_AssocTable():logger(MF_Logger::init()) {
  _assocTable = new AssocTable(); 
}

MF_AssocTable::~MF_AssocTable() {
}

int MF_AssocTable::configure (Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse (conf, this, errh, cpEnd) < 0) {
    return -1;
  }
  return 0; 
}

bool MF_AssocTable::insert (uint32_t guid, uint32_t assoc_guid) {
  assoc_table_record_t record;
  record.assoc_guid = assoc_guid;
  record.host_guid = guid;
  record.ts = Timestamp::now_steady();

  AssocTable::iterator it = _assocTable->find(assoc_guid);

  if (it == _assocTable->end()) {
    HashTable<uint32_t, assoc_table_record_t> *guids = 
        new HashTable<uint32_t, assoc_table_record_t>();
    guids->set(guid, record); 
    _assocTable->set(assoc_guid, guids);
    logger.log(MF_DEBUG, 
      "assoc_table: insert: insert guid: %u to a new assoc_guid: %u entry", 
      guid, assoc_guid); 
  } else { 
    HashTable<uint32_t, assoc_table_record_t>::iterator it2 = 
                                                       it.value()->find(guid);
    if (it2 == it.value()->end()) {
      logger.log(MF_DEBUG, 
        "assoc_table: insert: insert guid %u to an exist assoc_guid %u entry",
        guid, assoc_guid ); 
    } else {
      logger.log(MF_DEBUG,
        "assoc_table: insert: assoc_guid %u, guid %u exists, renew timestamp", 
        assoc_guid, guid);  
    }
    it.value()->set(guid, record);
  }
}

int32_t MF_AssocTable::get (uint32_t assoc_guid, Vector<uint32_t> &guid_vec) {
  AssocTable::iterator it = _assocTable->find(assoc_guid); 
  if (it == _assocTable->end()) {
    logger.log(MF_DEBUG, 
      "assoc_table: get: assoc_guid %u, no entry", assoc_guid);
    return 0; 
  } else {
    guid_vec.clear();
    HashTable<uint32_t, assoc_table_record_t>::iterator it2; 
    for (it2 = it.value()->begin(); it2 != it.value()->end(); ++it2) {
      if (MF_Functions::passed_sec(it2.value().ts) < ASSOC_TIMEOUT_SEC) {
        guid_vec.push_back(it2.key());
      } else {
        it.value()->erase(it2);             //remove timeout entry
      }
    }
    return guid_vec.size(); 
  }
}

bool MF_AssocTable::remove (uint32_t guid, uint32_t deassoc_guid) {
  AssocTable::iterator it = _assocTable->find(deassoc_guid);
  if (it == _assocTable->end()) {
    logger.log(MF_DEBUG,
      "assoc_table: remove: no entry guid: %u", deassoc_guid);
    return true; 
  } else {
    HashTable<uint32_t, assoc_table_record_t>::iterator it2 = 
        it.value()->find(guid); 
    if (it2 == it.value()->end()) {
      logger.log(MF_DEBUG,
        "assoc_table: remove: entry assoc_guid: %u exist, but no guid: %u",
        deassoc_guid, guid);
    } else {
      if (MF_Functions::passed_sec(it2.value().ts) < ASSOC_TIMEOUT_SEC) {
        logger.log(MF_DEBUG,
          "assoc_table: remove: erase guid: %u from assoc_guid: %u  entry", 
          guid, deassoc_guid); 
      }
      it.value()->erase(it2);
      if (it.value()->size() == 0) {
        _assocTable->erase(it);
        logger.log(MF_DEBUG,
          "assoc_table: remove: erase assoc_guid: %u entry",
          deassoc_guid); 
      }     
    }
  }
}

int MF_AssocTable::getAllClientGUIDs(Vector<uint32_t> &guids) {
  if (guids.size() !=0) {
    guids.clear();
  }
  AssocTable::iterator it = _assocTable->begin();
  while (it != _assocTable->end()) {
    uint32_t assoc_guid = it.key();
    if (it.value()->size() != 1) {
      continue; 
    } else {
      HashTable<uint32_t, assoc_table_record_t>::iterator it2 =
          it.value()->begin();
      if (it2.key() == it.key() && 
            MF_Functions::passed_sec(it2.value().ts) < ASSOC_TIMEOUT_SEC) {
        guids.push_back(assoc_guid); 
      } 
    }
  }
  return guids.size(); 
}

//for debug
void MF_AssocTable::print() {
  clean();
  logger.log(MF_DEBUG, "assoc_table: print"); 
  for (AssocTable::iterator it = _assocTable->begin(); it != _assocTable->end();
         ++it) {
    char buf[256];
    int n = sprintf(buf, "|\t%d\t|", it.key()); 
    for (HashTable<uint32_t, assoc_table_record_t>::iterator it2 = it.value()->begin();
           it2 != it.value()->end(); ++it2) {
      n += sprintf(buf + n, " %d,", it2.key());
    }
    buf[n] = '\0'; 
    logger.log(MF_DEBUG, "%s", buf); 
  }
}

void MF_AssocTable::clean() {
  for (AssocTable::iterator it = _assocTable->begin(); it != _assocTable->end();
         ++it) {
    for (HashTable<uint32_t, assoc_table_record_t>::iterator it2 = it.value()->begin();
           it2 != it.value()->end(); ++it2) {
      if (MF_Functions::passed_sec(it2.value().ts) > ASSOC_TIMEOUT_SEC) {
        it.value()->erase(it2);
      }
    }
    if (!it.value()->size()) {
      _assocTable->erase(it);
    }
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MF_AssocTable);
