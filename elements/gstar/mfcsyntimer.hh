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
#ifndef MF_CSYN_TIMER_HH_
#define MF_CSYN_TIMER_HH_

#include <click/timer.hh>
#include <click/element.hh>

#include "mfhopmsg.hh"

class MF_Segmentor; 

typedef struct CSYNTimerData {
  MF_Segmentor *elem;
  uint32_t hop_ID;
  uint32_t chk_pkt_count;
  uint32_t chk_id;
} csyn_timer_data_t;

class MF_CSYNTimer {
 public:
  MF_CSYNTimer() : isTimerSet(false), csyn_timer(NULL), timer_data(NULL) {
  }

  ~MF_CSYNTimer() {
    if (isTimerSet) {
      delete csyn_timer;
      delete timer_data; 
      //isTimerSet = false; 
    }
  }
  
  bool init(TimerCallback f, MF_Segmentor *seg, uint32_t hopID, 
            uint32_t pktCnt, uint32_t chunkID) {
    isTimerSet = true;
    timer_data = new csyn_timer_data_t();
    timer_data->elem = seg;
    timer_data->hop_ID = hopID;
    timer_data->chk_pkt_count = pktCnt;
    timer_data->chk_id = chunkID;

    csyn_timer = new Timer(f, timer_data);
    //cast to base class 
    csyn_timer->initialize((Element*)seg);
    csyn_timer->schedule_after_msec(CSYN_TIMEOUT);
  }
  
  bool kill() {
    isTimerSet = false;
    delete csyn_timer;
    delete timer_data;
    csyn_timer = NULL; 
    timer_data = NULL; 
  }
  
  bool isAlive() {
    return isTimerSet; 
  }
  inline void reschedule() {
    csyn_timer->reschedule_after_msec(CSYN_TIMEOUT); 
  }
  
  inline void unschedule() {
    assert(csyn_timer);
    assert(timer_data); 
    csyn_timer->unschedule(); 
  }
   
 private:
  Timer *csyn_timer;
  csyn_timer_data_t *timer_data; 
  bool isTimerSet; 
};


#endif /*MF_CSYN_TIMER_HH_*/
