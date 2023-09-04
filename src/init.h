//
// Created by development on 01.09.23.
//

#ifndef LILYGO_WEATHER_INIT_H
#define LILYGO_WEATHER_INIT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include "epd_driver.h"
#include "font/firasans.h"
#include "esp_adc_cal.h"
#include <FS.h>
#include <SPI.h>
#include <SD.h>

#include "pins.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_spiffs.h>
#include <cstdint>
#include "weather.h"
#include "sntp.h"
#include "support.h"



void InitSpiff();
uint8_t InitWifi();
void InitSntp();

#endif //LILYGO_WEATHER_INIT_H
