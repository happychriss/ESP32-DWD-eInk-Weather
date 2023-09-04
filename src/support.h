//
// Created by development on 27.08.23.
//

#ifndef LILYGO_WEATHER_SUPPORT_H
#define LILYGO_WEATHER_SUPPORT_H

#include <Arduino.h>
#include "Arduino.h"
#include "support.h"

#include <ctime>



#define MYDEBUG
#ifdef MYDEBUG
#define MYDEBUG_CORE
#define DP(x)     Serial.print (x)
#define DPD(x)     Serial.print (x, DEC)
#define DPL(x)  Serial.println (x)
#define DPF(...) Serial.printf (__VA_ARGS__)
#define DP2(x)     waitAndPrint (x)
#else
#define DP(x)
#define DPD(x)
#define DPL(x)
#define DPF(...)
#define DP2(x)
#endif

void waitAndPrint(const char* message);

void Serial_printTime(std::tm *time);
#endif //LILYGO_WEATHER_SUPPORT_H
