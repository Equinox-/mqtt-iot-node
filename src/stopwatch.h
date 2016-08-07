#ifndef __STOPWATCH_H
#define __STOPWATCH_H

extern "C" unsigned long millis();

#define STOPWATCH_INIT(varname) static unsigned long varname = -1
#define STOPWATCH_HAS_BEEN(varname, ttl) ((varname) > millis() || (varname) + (ttl) < millis())
#define STOPWATCH_BEGIN(varname) varname = millis()

#endif