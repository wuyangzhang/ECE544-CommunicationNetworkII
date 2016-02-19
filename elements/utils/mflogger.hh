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

#ifndef MF_LOGGER_HH_
#define MF_LOGGER_HH_

#include <click/timestamp.hh>

#include <cstdarg>
#include <cstdlib>

#define LOG_BUF_SIZE 1024

/* log message types */

enum log_level_t { 
    MF_TRACE = 0,
    MF_DEBUG = 1,
    MF_INFO = 2,
    MF_TIME = 3, /* same as INFO, but adds a current timestamp */
    MF_WARN = 4,
    MF_ERROR = 5,
    MF_CRITICAL = 6,
    MF_FATAL = 7
};

CLICK_DECLS

class MF_Logger {

    public:
        
        /* retreives and sets log_level value from env (if present) */
        static inline MF_Logger init() {

            int log_level = MF_ERROR;
            char * mf_ll;
            mf_ll = getenv("MF_CLICK_LOG_LEVEL");
            if (mf_ll != NULL) {
                log_level = get_log_level(atoi(mf_ll));
            } else {
                fprintf(stderr, 
                "WARN: MF_CLICK_LOG_LEVEL not set, "
                "setting to default %d == ERROR\n", log_level);
            }
            MF_Logger m(log_level);
            return m;
        }

        MF_Logger(int level):log_level(level) {

        }


        inline void log(log_level_t msg_type, const char* fmt, ...) {

            if(msg_type < log_level)return;
            
            va_list val;
            va_start(val, fmt);
    
            //truncate after about 3 lines (each of 80 chars) 
            char buf[LOG_BUF_SIZE];
            set_log_level_str(msg_type, buf);
            int start_index = strlen(buf);
            /* write message in remaining, leave room for '\n'*/
            int max = LOG_BUF_SIZE - start_index - 1;
            int req; //no of bytes vsnprint would write if space allows
            int last_index = LOG_BUF_SIZE - 3; //position of last char of log message
            if ((req = vsnprintf(buf+start_index, max, fmt, val)) < max) {
                last_index = start_index - 1 + req;
            }
            buf[last_index + 1] = '\n'; buf[last_index + 2] ='\0';

            fputs((const char*) buf, stderr);
            va_end(val);
        }

    private:

        int log_level;

        /* 
         * checks integer value lies within defined range, and returns
         * corresponding enum type
         */
        static inline log_level_t get_log_level(int lvl) {

            switch (lvl) {

                case MF_TRACE: return MF_TRACE;
                case MF_DEBUG: return MF_DEBUG;
                case MF_INFO: return MF_INFO;
                case MF_TIME: return MF_TIME;
                case MF_WARN: return MF_WARN;
                case MF_ERROR: return MF_ERROR;
                case MF_CRITICAL: return MF_CRITICAL;
                case MF_FATAL: return MF_FATAL;
                /* if out of range, default to ERROR */
                default: return MF_ERROR;
            }
        }

        static inline void set_log_level_str(log_level_t lvl, char* str) {

            switch (lvl) {

                case MF_TRACE: sprintf(str, "TRACE: ");break;
                case MF_DEBUG: sprintf(str, "DEBUG: ");break;
                case MF_INFO: sprintf(str, "INFO : ");break;
                case MF_TIME: 
                                struct timeval ts;
                                gettimeofday(&ts, NULL); 
                                sprintf(str, "INFO : [%llu]: ", 
                                (unsigned long long)ts.tv_sec*1000000 + 
                                (unsigned long long)ts.tv_usec);
                                break;
                case MF_WARN: sprintf(str, "WARN : ");break;
                case MF_ERROR: sprintf(str, "ERROR: ");break;
                case MF_CRITICAL: sprintf(str, "CRITICAL: ");break;
                case MF_FATAL: sprintf(str, "FATAL: ");break;
                //default: //won't happen
            }

        }

};


CLICK_ENDDECLS

#endif /* MF_LOGGER_HH_ */
