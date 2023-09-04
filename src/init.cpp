//
// Created by development on 01.09.23.
//

#include "init.h"
#include "credentials.h"
uint8_t InitWifi() {
    Serial.println("\r\nConnecting to: " + String(ssid));
    IPAddress dns(8, 8, 8, 8); // Use Google DNS
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // switch off AP
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("STA: Failed!\n");
        WiFi.disconnect(false);
        delay(500);
        WiFi.begin(ssid, password);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected at: " + WiFi.localIP().toString());
    }
    else Serial.println("WiFi connection *** FAILED ***");
    return WiFi.status();
}


void InitSpiff() {
    static const char *TAG = "SPIFFS";

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_err_t ret;

#undef SPIFF_FORMAT
#ifdef SPIFF_FORMAT
    ret= esp_spiffs_format(NULL);
        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            }
        } else {
            ESP_LOGI(TAG, "SPIFFS_format() successful");
        }
#endif
    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

// Use settings defined above to initialize and mount SPIFFS filesystem.
// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }


    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void InitSntp() {
    DP("SNT Server Setup:");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *) "pool.ntp.org");
    sntp_init();

    sntp_sync_status_t st;
    int count = 0;
    do {
        st = sntp_get_sync_status();
        count++;
        DP(".");
        delay(500);
    } while (st != SNTP_SYNC_STATUS_COMPLETED && count < 20);

    if (st != SNTP_SYNC_STATUS_COMPLETED) {
        DP("ERROR TIME SYNC:");
        DP(st);
        delay(5000);
        ESP.restart();
    }

    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    delay(500); //needed until time is available
    DPL("SNTP DONE");
}