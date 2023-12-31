// source: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/tree/master/examples/demo
#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include "epd_driver.h"
#include "font/firasans.h"
#include "esp_adc_cal.h"
#include <FS.h>
#include <SPI.h>
#include <SD.h>

#include "image/output.h"
#include "pins.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_spiffs.h>
#include <cstdint>
#include <esp_wifi.h>
#include "weather.h"
#include "sntp.h"
#include "support.h"
#include "init.h"



#if defined(T5_47_PLUS)
#include "pcf8563.h"
#include <Wire.h>
#endif


#if defined(T5_47_PLUS)
PCF8563_Class rtc;
#endif

// global variables
struct_Weather WF;


void epd_print_time(int x, int y, std::tm *time) {

    char str_time[20];
    strftime(str_time, sizeof(str_time), "%H:%M %d.%m.%y", time);
    int cursor_x = x;
    int cursor_y = y;

    writeln((GFXfont *) &FiraSans, str_time, &cursor_x, &cursor_y, NULL);
    epd_poweroff();
}

void PaintWeatherIcon(int x, int y, int icon) {

    DP("PaintWeatherIcon: start");
    //get address of icon from memrry (flash)
    auto  icon_data_pointer= (const uint8_t*)pgm_read_ptr(&(weather_icons[icon]));

    Rect_t area = {
            .x = x,
            .y = y,
            .width = 128,
            .height = 128,
    };

    DP("draw image: ");
    epd_draw_image(area, (uint8_t * ) icon_data_pointer, BLACK_ON_WHITE);
    DP("PaintWeatherIcon: done");
}


void setup() {
    Serial.begin(115200);
    DPL("***** Setup *****");

    DPL("***** Setup: init epd *****");
    epd_init();
    epd_poweron();
    epd_clear();

    int cursor_x = 200;
    int cursor_y = 250;
    const char *string1 = "➸ getting fresh weather  😀 \n";
    writeln((GFXfont *)&FiraSans, string1, &cursor_x, &cursor_y, NULL);
    epd_poweroff();

/*    InitSpiff();*/
    InitWifi();
    InitSntp();


    epd_poweron();

    std::time_t nowt = std::time(0);   // get time now
    std::tm *now = std::localtime(&nowt);
    epd_print_time(200, 250 + 50, now);

    epd_poweroff();

    DPL("***** Setup done *****");
}

void PaintWeather(struct_Weather *ptr_myW) {

    epd_poweron();
    epd_clear();

#define X_START 10

#define Y_START_TIME 60
#define Y_START (Y_START_TIME+100)
#define Y_LINE_HEIGHT_ICONS 140
#define Y_LINE_HEIGHT_TEXT 50

    int x_cursor = X_START;
    int y_cursor = Y_START;
    int y_position = 0;

    std::time_t nowt = std::time(0);   // get time now
    std::tm *now = std::localtime(&nowt);

    epd_print_time(X_START, Y_START_TIME, now);
    epd_print_time(X_START+300, Y_START_TIME, &ptr_myW->publish_time);

    std::vector<int> forecasts = {0, 3, 6};

    for (const auto& value : forecasts) {
        DPL("*********************** Painting forecast *********************");
        auto fc = ptr_myW->HourlyWeather[value];
        printHourlyWeather(fc);

        int weather_icon= determineWeatherIcon(fc);
        DPF("Weather icon: %d\n", weather_icon);

        String summary;
        String forecast;
        determineWeatherString(fc, summary, forecast);

        DPF("Weather text-1: %s\n", summary.c_str());
        DPF("Weather text-2: %s\n", forecast.c_str());

        DP("Painting weather icon...");
        x_cursor = X_START;
        y_cursor = Y_START+y_position-Y_LINE_HEIGHT_TEXT;
        PaintWeatherIcon(x_cursor, y_cursor, weather_icon);

        DP("Painting weather text...");
        x_cursor = X_START+140;
        y_cursor = Y_START+y_position;
        writeln((GFXfont *)&FiraSans, summary.c_str(), &x_cursor, &y_cursor, NULL);

        x_cursor = X_START+140;
        y_cursor = Y_START+Y_LINE_HEIGHT_TEXT+y_position;
        writeln((GFXfont *)&FiraSans, forecast.c_str(), &x_cursor, &y_cursor, NULL);
        DP("Painting weather text done.");

        y_position += Y_LINE_HEIGHT_ICONS;

    }


    DPL("*********************** Painting forecast done *********************");

}

// this task is needed to have stack sized increased to 40k, seems to be no other way to do this

void loop() {
    DPL("***** Loop *****");
    struct_Weather Weather;

    GetWeather(&Weather);
    PaintWeather(&Weather);

#define DEEP_SLEEP_TIME_SEC (60*60*3)
    uint64_t deep_sleep_time =  DEEP_SLEEP_TIME_SEC;
    DPF("Good Night for %d...", deep_sleep_time);
    esp_wifi_stop();
    epd_poweroff();

    esp_sleep_enable_timer_wakeup(1000000ULL * DEEP_SLEEP_TIME_SEC);
    esp_deep_sleep_start();

    while (true) {
        delay(1000);
    }
}
