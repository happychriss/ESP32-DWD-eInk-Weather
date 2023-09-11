// check out https://rudo.info/mosmix.php
// https://github.com/jolichter/dwdWeather/blob/master/dwdWeather.php
#include "weather.h"
#include "pugixml.hpp"
#include <iostream>
#include <string>
#include <cmath>
#include <HTTPClient.h>
#include "miniz.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "support.h"

#define DEST_FS_USES_SPIFFS

#include <ESP32-targz.h>
#include "support.h"

void printHourlyWeather(struct_HourlyWeather hw)  {
    Serial_printTime(&hw.time);
    DPF("(%d)\t", hw.hour);
    DPF("Forecast(%d): %s\t", hw.forecast_id,hw.forecast.c_str());
    DPF(" %f C\t", hw.temperature);
    DPF(" %f Bft\t", hw.wind);
    DPF("Rain: %f mm\t", hw.rain);
    DPF("Sun: %f min\t", hw.sun);
    DPF("Clouds: %f %\n", hw.clouds);
}

String getWeatherString(int number) {
    auto it = Weather::weatherData.find(number);
    if (it != Weather::weatherData.end()) {
        return it->second;
    } else {
        return "Unknown weather code";
    }
}



//  pio run -t downloadfs
void *downloadKML(const String fetch_url, size_t *buffer_len) {

//    Serial.printf("Fetching URL: %s\n", url.c_str());
    DPL("Creating HTTP client...");
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    HTTPClient http;
    http.begin(*client, DWD_URL);
    int httpCode = http.GET();
    DP("HTTP client created.");

    if (httpCode != HTTP_CODE_OK) {
        DPL("Failed to fetch the URL.");
        return nullptr;
    }

    // get length of document (is -1 when Server sends no Content-Length header)
    int len = http.getSize();
    int file_len= len;


    if (len == -1) {
        DPL("Server did not send Content-Length header.");
        return nullptr;
    }

    DPF("Content-Length: %d\n", len);

    // Allocate buffer dynamically
    char *buffer = (char *)calloc(file_len + 1,sizeof(char));  // +1 for null-terminator
    if (buffer == nullptr) {
        DPL("Failed to allocate memory for the file.");
        return nullptr;
    }

    FILE *file=fmemopen(buffer, file_len, "w");

    //     FILE *file = fopen(MOSMIX_ZIP_FILE, FILE_WRITE);

    if (!file) {
        DPL("Failed to create the file.");
        return nullptr;
    }

    DP("Opened downloaded zip file...read file");
    // create buffer for read
    uint8_t buff[128] = {0};

    // get tcp stream
    WiFiClient *stream = http.getStreamPtr();

    // read all data from server
    while (http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if (size) {
            // read up to 128 byte
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            DP(".");
            fwrite(buff, 1, static_cast<size_t>(c), file);
            if (len > 0) {
                len -= c;
            }
        }
        delay(1);
    }
    DPL("Done reading file.");

    fclose(file);

    http.end();
    DPL("Done saving the file");
    DPL("Printing first 10 bytes of the file: ");

    for(int i = 0; i < 50; i++) {
        char c = *(buffer+ i);
        DPF("%d: ",i);
        Serial.println(c,HEX);
    }

    DPL("Printing last 10 bytes of the file:");
    for(int i = 0; i <50; i++) {
        char c = *(buffer+file_len-(50 - i));
        DPF("%d: ",i);
        Serial.print(c,HEX);
        Serial.print("-");
        Serial.println(c);
    }
    DPL("Done printing file.");
    FILE *file2=fmemopen(buffer, file_len, "r");
    // Unzipping using miniz
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    DPL("Initializing miniz...");
    // https://www.esp32.com/viewtopic.php?f=13&t=1076

    if (!mz_zip_reader_init_file_v3(&zip_archive, file2, 0, 0, 0)) {
        DPL("Error initializing miniz.");
        mz_zip_error mzerr = mz_zip_get_last_error(&zip_archive);
        DP(mz_zip_get_error_string(mzerr));
        return nullptr;
    }

    DPL("Initialized miniz.");
    mz_uint num_files = mz_zip_reader_get_num_files(&zip_archive);
    if (num_files <= 0) {
        DPL("The zip file does not contain any files.");
        mz_zip_reader_end(&zip_archive);
        return nullptr;
    }

    DPF("The zip file contains %d files.\n", num_files);
    // Extract the first file
    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, 0, &file_stat)) {
        DPL("Error retrieving the file information from ZIP archive.");

        mz_zip_reader_end(&zip_archive);
        return nullptr;
    }
    DPF("The first file name in the zip is: %s\n", file_stat.m_filename);
    mz_zip_error mzZipError;
    size_t heapSize = esp_get_free_heap_size();
    DPF("Heap size: %d\n", heapSize);

    // Allocate buffer dynamically
    char *unzip_buffer = (char *)calloc(file_stat.m_uncomp_size + 1,sizeof(char));  // +1 for null-terminator
    if (unzip_buffer == nullptr) {
        DPL("Failed to allocate memory for the unzip buffer.");
        return nullptr;
    }

    if (!mz_zip_reader_extract_to_mem_no_alloc(
            &zip_archive,
            0,
            unzip_buffer,
            file_stat.m_uncomp_size,
            0,
            nullptr,
            0)) {
        DPL("Error extracting the file: ");
        DPL(mz_zip_get_error_string(mzZipError));
        mz_zip_error mzerr = mz_zip_get_last_error(&zip_archive);
        DPL(mz_zip_get_error_string(mzerr));
        mz_zip_reader_end(&zip_archive);
        return nullptr;
    }

    *buffer_len = file_stat.m_uncomp_size;

    DPL("Done");

    return unzip_buffer;
}


// DWD data has index of hours from last forecast time - identify index of hours to use for accessing forecast data
// also
std::pair<int, int> get_hour_index_for_time(std::tm *forecast_time, pugi::xml_node dt_root) {

    int hours_index = 0;
    int hours_index_result = 0;
    int update_index = 0;
    bool b_found = false;
    struct std::tm TimeStep{};

    for (pugi::xml_node dt: dt_root.children("dwd:TimeStep")) {

        std::istringstream w_time(dt.first_child().value());

        w_time >> std::get_time(&TimeStep, "%Y-%m-%dT%H:%M:%S"); // or just %T in this case

        if (w_time.fail()) {
            DPL("Parse failed");
            return std::make_pair(0, 0);
        }

        if (!b_found) {
            int seconds = difftime(mktime(&TimeStep), mktime(forecast_time));
            std::cout << std::put_time(&TimeStep, "%c") << ' ' << seconds << '\n';
            if (seconds > 0) {
                DP(seconds);
                DP(":");
                DP(dt.first_child().value());
                DP(":");
                Serial_printTime(&TimeStep);
                DP(" hour:");
                DPL(TimeStep.tm_hour);
                b_found = true;
                hours_index_result = hours_index;
            }

        }
        if (b_found) {
            WF.HourlyWeather[update_index].time = TimeStep;
            WF.HourlyWeather[update_index].hour = hours_index;
            update_index++;
        }

        if (update_index == HOURS_FORECAST) {
            break;
        }

        hours_index++;

    }

    return std::make_pair(hours_index_result, TimeStep.tm_hour);
}


std::vector<double> extractNumbersFromString(const std::string &inputString) {
    std::vector<double> numbers;
    int i = 0;
    int len = inputString.length();

    while (i < len) {
        // Skip non-digit characters and notations like '+', '-', '.', and 'e'
        while (i < len && !std::isdigit(inputString[i]) && inputString[i] != '.' && inputString[i] != '-' &&
               inputString[i] != '+')
            ++i;

        // Extract the number
        std::string numStr;
        while (i < len && (std::isdigit(inputString[i]) || inputString[i] == '.' || inputString[i] == '-' ||
                           inputString[i] == '+')) {
            numStr += inputString[i];
            ++i;
        }

        // Convert the extracted number to double and store in the vector
        if (!numStr.empty()) {
            try {
                double number = std::stod(numStr);
                numbers.push_back(number);
            } catch (const std::invalid_argument &e) {
                // Ignore invalid numbers
            }
        }
    }

    return numbers;
}


std::vector<double>
getForcast(pugi::xml_node dforecast_root, int hours_index, const std::string &search_elementName) {

    std::vector<double> forecast_value;

    for (pugi::xml_node dt_fc: dforecast_root.children("dwd:Forecast")) {

        std::string elementName = dt_fc.attribute("dwd:elementName").value();
//        DP("ElementName: ");
//        DPL(elementName.c_str());
        if (elementName == search_elementName) {

            std::string data = dt_fc.child("dwd:value").text().get();
            std::vector<double> forecast = extractNumbersFromString(data);
            for (int i = hours_index; i < hours_index + HOURS_FORECAST && i < forecast.size(); ++i) {
                forecast_value.push_back(forecast[i]);
            }

            break;
        }

    }

    return forecast_value;
}


const uint8_t *findIconByAlternativeIndex(uint16_t altIndex) {
    for (auto weather_icon: weather_icons) { // Assuming the array size is 20
        auto alternative_index = (uint16_t) pgm_read_word(&(weather_icon[1]));
        DPF("findIconByAlternativeIndex: alternative_index=%d\n", alternative_index);

        if (alternative_index == altIndex) {
            DPF("findIconByAlternativeIndex: found icon for alternative index %d\n", altIndex);
            auto res = (const uint8_t *) pgm_read_ptr(&(weather_icon[0]));
            DPL("findIconByAlternativeIndex-3");
            return res;
        }
    }
    return nullptr; // Not found
}


void determineWeatherString(const struct_HourlyWeather &weather, String &line_1, String &line_2) {

    String summary_1 = String(weather.time.tm_hour) + " Uhr: ";
    String summary_2 = "";


    summary_1 += String(weather.temperature) + "°C, ";

    if (weather.rain > 0.0) {
        summary_1 += String(weather.rain) + " mm, ";
    }

    summary_1 += String(weather.wind) + " Bft";

    if (weather.forecast_id >3) {
        summary_2 = weather.forecast;
    }
    line_1 = summary_1;
    line_2 = summary_2;

}


int determineWeatherIcon(const struct_HourlyWeather &hw) {
//    struct struct_HourlyWeather {
//        tm time;
//        int hour;
//        double temperature;
//        double wind;
//        double rain;
//        double clouds;
//        double sun;
//        std::string forecast;
//        int forecast_id;
//    };




    int fc_icon = NO_ICON_FOUND;

    // check in DWD weather mapping if we find a fitting icon for DWD weather - if yes-take this
    for (auto weather_id_map: weather_id_maps) {
        if (weather_id_map.dwd_id == hw.forecast_id) {
            DPF("Checking Icon from DWD:  %d with forecast %d\n", weather_id_map.icon_id, hw.forecast_id);
            fc_icon = weather_id_map.icon_id;
            break;
        }
    }

    if (fc_icon != NO_ICON_FOUND) {
        DPF("Found Icon from DWD:  %d\n", fc_icon);
        return fc_icon;
    }

    DPL("No Icon found from DWD - checking alternative icons");

    if (hw.rain == 0) {

        // Check for the sun - measured in minutes
        if ((hw.sun > 55) || (hw.clouds < 5)) {
            return wi_Sunny;
        }

        if ((hw.sun > 45) || (hw.clouds < 10)) {
            return wi_MostlySunny;
        }

        if ((hw.sun > 30) || (hw.clouds < 20)) {
            return wi_PartlySunny;
        }

        if ((hw.sun > 10) || (hw.clouds < 40)) {
            return wi_PartlyCloudy;
        }

        if (hw.clouds < 60) {
            return wi_PartlyCloudy;
        }

        return wi_Cloudy;
    }

    if (hw.rain < 0.2) {
        return wi_ChanceRain;
    }

    if (hw.rain < 0.5) {
        return wi_Rain;
    }

    return wi_Cloudy;

}


void getWeather() {

    DPL("Downloading KML file...");
    String localFilePath;
    void *p = nullptr;
    size_t p_len;
    p = downloadKML(DWD_URL, &p_len);
//    char *str = (char *)p;
//    DPL(str);

    if (p) {
        DPL("Parsing KML file...");
        pugi::xml_document dwd_xml;

        // Assuming the data in the buffer is null-terminated, otherwise you'd also need to pass the size
        pugi::xml_parse_result result = dwd_xml.load_buffer(p, p_len);
        if (result) {
            DPL("KML file parsed successfully.");
            std::time_t nowt = std::time(0);   // get time now
            std::tm *now = std::localtime(&nowt);

            // Read Issue Time
            pugi::xml_node dt_time = dwd_xml.child("kml:kml").
                    child("kml:Document").
                    child("kml:ExtendedData").
                    child("dwd:ProductDefinition").
                    child("dwd:IssueTime");

            DPF("dt_time: %s\n", dt_time.first_child().value() );

            std::istringstream w_time(dt_time.first_child().value());

            tm IssueTime{};
            w_time >> std::get_time(&IssueTime, "%Y-%m-%dT%H:%M:%S"); // or just %T in this case
            DP("IssueTime: ");
            Serial_printTime(&IssueTime);
            WF.publish_time = IssueTime;

            // Extract hours into hours_index

            DPL("Parsing Hours!");

            pugi::xml_node dt_root = dwd_xml.child("kml:kml").
                    child("kml:Document").
                    child("kml:ExtendedData").
                    child("dwd:ProductDefinition").
                    child("dwd:ForecastTimeSteps");

            std::time_t t = std::time(0);   // get time now
            now = std::localtime(&t);


            Serial_printTime(now);
            DPL("<-Current time");

            int hours_index, hours_offset;
            std::tie(hours_index, hours_offset) = get_hour_index_for_time(now, dt_root);
            DPF("Hours Index: %d\n", hours_index);

            // Extract weather data - jump to the right hour in

            pugi::xml_node dforecast_root = dwd_xml.
                    child("kml:kml").
                    child("kml:Document").
                    child("kml:Placemark").
                    child("kml:ExtendedData");
//            DPF( "dforecast_root: %s\n", dforecast_root.name());

            // Get all the weather data for the next 24 hours and store in HourlyWeather

            // ************** Get Weather Symbol ************** by using the forecast value for ww
            std::vector<double> forecast_value = getForcast(dforecast_root, hours_index, "ww");


            for (int i = 0; i < forecast_value.size(); ++i) {

                // DPF( "Hour %d: %s\n", i, getWeatherString((int) forecast_value[i]).c_str());

                WF.HourlyWeather[i].forecast = getWeatherString((int) forecast_value[i]);
                WF.HourlyWeather[i].forecast_id = (int) forecast_value[i];

            }

            // ************** Get Temperature ************** by using the forecast value for TTT
            forecast_value = getForcast(dforecast_root, hours_index, "TTT");

            for (int i = 0; i < forecast_value.size(); ++i) {
                // DPF( "Hour %d: %f C\n", i, forecast_value[i] - 273.15);
                WF.HourlyWeather[i].temperature = forecast_value[i] - 273.15;
            }

            // ************** Get Rain **************10 kg/m² Niederschlag entsprechen ungefähr 10 mm Niederschlag

            forecast_value = getForcast(dforecast_root, hours_index, "RR1c");
            for (int i = 0; i < forecast_value.size(); ++i) {
                // DPF( "Hour %d: %f mm\n", i, forecast_value[i]);
                WF.HourlyWeather[i].rain = forecast_value[i];
            }

            // ************** Get Wind **************

            forecast_value = getForcast(dforecast_root, hours_index, "FF");

            for (int i = 0; i < forecast_value.size(); ++i) {
                auto wind_beaufort = static_cast<double >(std::round(forecast_value[i] * forecast_value[i] / 3.01));
                // DPF( "Hour %d: %f Bft\n", i, wind_beaufort);
                WF.HourlyWeather[i].wind = wind_beaufort;
            }

            // ************** Get Sun **************

            forecast_value = getForcast(dforecast_root, hours_index, "SunD1");

            for (int i = 0; i < forecast_value.size(); ++i) {
                WF.HourlyWeather[i].sun = static_cast<double >(std::round(forecast_value[i] / 60));
            }

            // ************** Get Cloud **************

            forecast_value = getForcast(dforecast_root, hours_index, "Neff");

            for (int i = 0; i < forecast_value.size(); ++i) {
                WF.HourlyWeather[i].clouds = forecast_value[i];
            }

            for (auto &hw: WF.HourlyWeather) {
                printHourlyWeather(hw);
            }


        } else {
            DPL("ERROR: Failed to load and parse the KML file:");
            DPL(result.description());
        }

        std::cout << "Done!\n";

    } else {
        std::cout << "Failed to load and parse the KML file." << std::endl;
    }
}

void paint_weather() {

}