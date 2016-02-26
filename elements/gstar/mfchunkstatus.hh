#ifndef MF_CHUNKSTATUS_HH
#define MF_CHUNKSTATUS_HH


CLICK_DECLS

class ChunkStatus {
public:
  ChunkStatus() : status(0) {
  }
  ChunkStatus(const uint16_t chk_stat) : status(chk_stat){
  }
  ChunkStatus(uint32_t chk_stat) {
    chk_stat &= 0xffff; 
    this->status = chk_stat; 
  }
  ChunkStatus( const ChunkStatus &chk_stat ) {
    this->status = chk_stat.status; 
  }
  ~ChunkStatus() {
  }
  
  inline uint16_t get() const { 
    return this->status;
  }
  
  //check whether flags are set or not
  inline bool is(uint16_t chk_stat) {
    return (this->status & chk_stat) == chk_stat? true : false; 
  }
  
  inline bool isComplete() {
    return (this->status & ST_COMPLETE) == ST_COMPLETE? true : false; 
  }
  inline bool isBuffered() {
    return (this->status & ST_BUFFERED) == ST_BUFFERED? true : false; 
  }
  inline bool isExclusive() {
    return (this->status & ST_EXCLUSIVE) == ST_EXCLUSIVE? true : false; 
  }
  inline bool isResolving() {
    return (this->status & ST_RESOLVING) == ST_RESOLVING? true : false; 
  } 
  inline bool isInitialized() {
    return (this->status & ST_INITIALIZED) == ST_INITIALIZED? true : false; 
  }
  //inline bool isAcked() {
  //  return (this->status & ST_ACKED) == ST_ACKED? true : false;
  //}
  inline bool isReadyToDel() {
    return (this->status & ST_READY_TO_DEL) == ST_READY_TO_DEL?  true : false; 
  }
  
  //set functions
  inline ChunkStatus set(uint16_t chk_stat) {
    this->status |= chk_stat; 
    return *this; 
  } 
  
  inline ChunkStatus setComplete() {
    this->status |= ST_INITIALIZED;     //A ST_COMPLETE chunk must be a ST_INITIALIZED one
    this->status |= ST_COMPLETE; 
    return *this;
  }
  inline ChunkStatus setBuffered() {
    this->status |= ST_BUFFERED; 
    return *this; 
  }
  inline ChunkStatus setExclusive() {
    this->status |= ST_EXCLUSIVE; 
    return *this; 
  }
  inline ChunkStatus setResolving() {
    this->status |= ST_RESOLVING; 
    return *this; 
  } 
  inline ChunkStatus setInitialized() {
    this->status |= ST_INITIALIZED;
    return *this;  
  }
  //inline ChunkStatus setAcked() {
  //  this->status |= ST_ACKED;
  //  return *this; 
  //}
  inline ChunkStatus setReadyToDel() {
    this->status |= ST_READY_TO_DEL; 
    return *this; 
  }
  
  //reset functions
  inline ChunkStatus reset( uint16_t chk_stat) {
    this->status &= (~chk_stat); 
    return *this; 
  }

  inline ChunkStatus resetComplete() {
    this->status &= (~ST_COMPLETE);
    return *this; 
  }
  inline ChunkStatus resetBuffered() {
    this->status &= (~ST_BUFFERED);
    return *this; 
  }
  inline ChunkStatus resetExclusive() {
    this->status &= (~ST_EXCLUSIVE); 
    return *this; 
  }
  inline ChunkStatus resetResolving() {
    this->status &= (~ST_RESOLVING);
    return *this; 
  }
  inline ChunkStatus resetInitialized() {
    this->status &=( ~ST_INITIALIZED);
    return *this; 
  }
  inline ChunkStatus resetReadyToRemove() {
    this->status &= (~ST_READY_TO_DEL); 
    return *this; 
  }

  //assign
  inline bool operator=(const ChunkStatus & chk_stat) {
    this->status = chk_stat.status;
    return true; 
  }
  inline bool operator=(const uint16_t chk_stat) {
    this->status= chk_stat;
    return true; 
  }
  
  //compare
  friend inline bool operator==(const ChunkStatus &lhs, const ChunkStatus &rhs) {
    return lhs.status == rhs.status ? true : false; 
  }
  friend inline bool operator==(const ChunkStatus &lhs, const uint16_t rhs) {
    return lhs.status == rhs ? true : false; 
  }
  
  friend inline bool operator!=(const ChunkStatus &lhs, const ChunkStatus &rhs) {
    return !(lhs == rhs); 
  }
  friend inline bool operator!=(const ChunkStatus &lhs, const uint16_t rhs) {
    return !(lhs == rhs); 
  }
  
  static const uint16_t ST_COMPLETE = 0x0080;          //0000 0000 1000 0000B  128
  static const uint16_t ST_BUFFERED = 0x0040;          //0000 0000 0100 0000B   68
  static const uint16_t ST_RESOLVING = 0x0020;         //0000 0000 0010 0000B   32
  static const uint16_t ST_CACHED = 0x0010;            //0000 0000 0001 0000B   16
  static const uint16_t ST_READY_TO_DEL = 0x0008;      //0000 0000 0000 1000B    8
  static const uint16_t ST_INITIALIZED = 0x0004;       //0000 0000 0000 0100B    4
  //static const uint16_t ST_ACKED = 0x0002;             //0000 0000 0000 0010B    2
  static const uint16_t ST_EXCLUSIVE = 0x0001;         //0000 0000 0000 0001B    1
  
private:
  uint16_t status;
};

CLICK_ENDDECLS
#endif
