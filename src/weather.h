#ifndef weather_h
#define weather_h

#include <Arduino.h>
#include "pugixml.hpp"
#include <iostream>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <map>
#include <algorithm>
#include <utility>

#include "image/output.h"


#define HOURS_FORECAST 24

#define DWD_URL "https://opendata.dwd.de/weather/local_forecasts/mos/MOSMIX_L/single_stations/H635/kml/MOSMIX_L_LATEST_H635.kmz"
//#define DWD_URL "https://example.com/"

#define MOSMIX_ZIP_FILE  "/spiffs/MOSMIX_L_LATEST.kmz"
#define MOSMIX_FILE  "/spiffs/MOSMIX_L_LATEST.kml"


#define NO_ICON_FOUND 999

const uint8_t * const weather_icons[] PROGMEM= {
        chanceflurries_data,  // index 0  // Flurries, ID: 0
        chancerain_data,      // index 1  // Chance Rain, ID: 61
        chancesleet_data,     // index 2  // Chance Sleet, ID: 56
        chancesnow_data,      // index 3  // Chance Snow, ID: 85
        chancetstorms_data,   // index 4  // Chance Thunderstorms, ID: 99
        clear_data,           // index 5  // Clear, ID: 1
        cloudy_data,          // index 6  // Cloudy, ID: 3
        flurries_data,        // index 7  // Flurries, ID: 83
        fog_data,             // index 8  // Fog, ID: 49
        hazy_data,            // index 9  // Hazy, ID: 99
        mostlycloudy_data,    // index 10 // Mostly Cloudy, ID: 99
        mostlysunny_data,     // index 11 // Mostly Sunny, ID: 99
        partlycloudy_data,    // index 12 // Partly Cloudy, ID: 99
        partlysunny_data,     // index 13 // Partly Sunny, ID: 99
        rain_data,            // index 14 // Rain, ID: 65
        sleet_data,           // index 15 // Sleet, ID: 57
        snow_data,            // index 16 // Snow, ID: 75
        sunny_data,           // index 17 // Sunny, ID: 0
        tstorms_data,         // index 18 // Thunderstorms, ID: 95
        unknown_data          // index 19 // Unknown, ID: 0
};

enum WeatherIcon {
    wi_ChanceFlurries,  // 0
    wi_ChanceRain,      // 1
    wi_ChanceSleet,     // 2
    wi_ChanceSnow,      // 3
    wi_ChanceTStorms,   // 4
    wi_Clear,           // 5
    wi_Cloudy,          // 6
    wi_Flurries,        // 7
    wi_Fog,             // 8
    wi_Hazy,            // 9
    wi_MostlyCloudy,    // 10
    wi_MostlySunny,     // 11
    wi_PartlyCloudy,    // 12
    wi_PartlySunny,     // 13
    wi_Rain,            // 14
    wi_Sleet,           // 15
    wi_Snow,            // 16
    wi_Sunny,           // 17
    wi_TStorms,         // 18
    wi_Unknown          // 19
};


struct WeatherIDMap {
    uint8_t dwd_id;
    int icon_id;
};



const WeatherIDMap weather_id_maps[] = {
        { 0, NO_ICON_FOUND }, // unknown
        { 61, 1 }, // chance rain
        { 56, 2 }, // chance sleet
        { 66, 2 }, // chance sleet
        { 68, 2 }, // chance sleet
        { 53, 2 }, // chance sleet
        { 51, 2 }, // chance sleet
        { 85, 3 }, // chance snow
        { 99, 4 }, // chance thunderstorms
        { 1, 5 }, // clear
        { 3, 6 }, // cloudy
        { 2, 6 }, // cloudy
        { 83, 7 }, // flurries
        { 49, 8 }, // fog
        { 99, 9 }, // hazy
        { 99, 10 }, // mostly cloudy
        { 99, 11 }, // mostly sunny
        { 99, 12 }, // partly cloudy
        { 99, 13 }, // partly sunny
        { 65, 14 }, // rain
        { 63, 14 }, // rain
        { 61, 14 }, // rain
        { 57, 15 }, // sleet
        { 67, 15 }, // sleet
        { 55, 15 }, // sleet
        { 75, 16 }, // snow
        { 73, 16 }, // snow
        { 71, 16 }, // snow
        { 69, 16 }, // snow
        { 0, 17 }, // sunny
        { 95, 18 }, // thunderstorms
        { 0, 19 }  // unknown
};




#pragma once
#include <map>
#include <string>

namespace Weather {
    // This std::map holds the mapping between weather codes and their descriptions
    static std::map<int, String> weatherData = {
            {95, "leichtes oder mäßiges Gewitter mit Regen oder Schnee"},
            {57, "mäßiger oder starker gefrierender Sprühregen"},
            {56, "leichter gefrierender Sprühregen"},
            {67, "mäßiger bis starker gefrierender Regen"},
            {66, "leichter gefrierender Regen"},
            {86, "mäßiger bis starker Schneeschauer"},
            {85, "leichter Schneeschauer"},
            {84, "mäßiger oder starker Schneeregenschauer"},
            {83, "leichter Schneeregenschauer"},
            {82, "äußerst heftiger Regenschauer"},
            {81, "mäßiger oder starker Regenschauer"},
            {80, "leichter Regenschauer"},
            {75, "durchgehend starker Schneefall"},
            {73, "durchgehend mäßiger Schneefall"},
            {71, "durchgehend leichter Schneefall"},
            {69, "mäßger oder starker Schneeregen"},
            {68, "leichter Schneeregen"},
            {55, "durchgehend starker Sprühregen"},
            {53, "durchgehend mäßiger Sprühregen"},
            {51, "durchgehend leichter Sprühregen"},
            {65, "durchgehend starker Regen"},
            {63, "durchgehend mäßiger Regen"},
            {61, "durchgehend leichter Regen"},
            {49, "Nebel mit Reifansatz, Himmel nicht erkennbar, unverändert"},
            {45, "Nebel, Himmel nicht erkennbar"},
            {3,  "Bewölkung zunehmend"},
            {2,  "Bewölkung unverändert"},
            {1,  "Bewölkung abnehmend"},
            {0,  "keine Bewölkungsentwicklung"}
    };
}


//struct std::tm TimeSteps[HOURS_FORECAST];

struct struct_HourlyWeather {
    tm time;
    int hour;
    double temperature;
    double wind;
    double rain;
    double clouds;
    double sun;
    String forecast;
    int forecast_id;
};
extern struct_HourlyWeather HourlyWeather[HOURS_FORECAST];
void getWeather();
void coreTask( void * pvParameters );
int determineWeatherIcon(const struct_HourlyWeather &hw);
void determineWeatherString(const struct_HourlyWeather &weather, String &line_1, String &line_2);
void printHourlyWeather(struct_HourlyWeather hw);
#endif



