
#ifndef INC_DEBUG_H
#define INC_DEBUG_H

#define DEBUG // ENABLE SERIAL DEBUG MESSAGES

#ifdef DEBUG
  #define DPRINT(...) Serial.print(__VA_ARGS__)
  #define DPRINTLN(...) Serial.println(__VA_ARGS__)
  #define DPRINTP(...) Serial.print(F(__VA_ARGS__))
  #define DPRINTPLN(...) Serial.println(F(__VA_ARGS__))
  #define DPRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DPRINT(...)    //blank line
  #define DPRINTLN(...)  //blank line
  #define DPRINTP(...)   //blank line
  #define DPRINTPLN(...) //blank line
  #define DPRINTF(...)   //blank line
#endif

#endif
