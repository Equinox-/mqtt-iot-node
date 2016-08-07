#ifndef __LOGGING_H
#define __LOGGING_H
#include <stdio.h>

#define LOGGING
#define VERBOSE
extern void log_message(char *buffer);

#ifdef LOGGING
	static char _pf_buffer_[256];
	#define LOG_BEGIN SerialUSB.begin(115200)
	#define LOG_F(format, ...) { snprintf(_pf_buffer_, sizeof(_pf_buffer_), format, ##__VA_ARGS__); log_message(_pf_buffer_);}
#else
#define LOG_BEGIN 
#define LOG_F(format, ...) 
#endif

#ifdef VERBOSE
#define LOG_F_VERB(format, ...) LOG_F(format, ##__VA_ARGS__)
#else
#define LOG_F_VERB(format, ...) 
#endif

#endif