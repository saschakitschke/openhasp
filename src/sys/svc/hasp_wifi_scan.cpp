/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#include "hasplib.h"

#if HASP_USE_WIFI > 0 && HASP_USE_HTTP_ASYNC > 0

#include "hasp_conf.h"
#include <ArduinoJson.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Scan Results Cache
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char ssid[33];
    int32_t rssi;
    uint8_t encryptionType;
    uint8_t channel;
} hasp_wifi_network_t;

static hasp_wifi_network_t* wifi_scan_results = NULL;
static uint16_t wifi_scan_count = 0;
static unsigned long wifi_last_scan = 0;
static const uint32_t WIFI_SCAN_CACHE_TIME = 10000; // Cache results for 10 seconds

////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////

void webHandleWifiScan(AsyncWebServerRequest* request);

////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Scan Handler Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

void wifiPerformScan()
{
    // Free previous scan results
    if(wifi_scan_results != NULL) {
        free(wifi_scan_results);
        wifi_scan_results = NULL;
        wifi_scan_count = 0;
    }

    LOG_TRACE(TAG_WIFI, F("Starting WiFi network scan..."));

    // Perform the WiFi scan (blocking, ~2-5 seconds typically)
    int16_t n = WiFi.scanNetworks(false, true); // async=false, show_hidden=true

    if(n <= 0) {
        LOG_WARNING(TAG_WIFI, F("WiFi scan found no networks"));
        wifi_scan_count = 0;
        wifi_last_scan = millis();
        return;
    }

    // Allocate memory for results
    wifi_scan_results = (hasp_wifi_network_t*)malloc(n * sizeof(hasp_wifi_network_t));
    if(wifi_scan_results == NULL) {
        LOG_ERROR(TAG_WIFI, F("Failed to allocate memory for WiFi scan results"));
        wifi_scan_count = 0;
        return;
    }

    // Copy scan results
    wifi_scan_count = n;
    for(int16_t i = 0; i < n; i++) {
        // SSID
        String ssid = WiFi.SSID(i);
        strncpy(wifi_scan_results[i].ssid, ssid.c_str(), sizeof(wifi_scan_results[i].ssid) - 1);
        wifi_scan_results[i].ssid[sizeof(wifi_scan_results[i].ssid) - 1] = '\0';

        // Signal strength
        wifi_scan_results[i].rssi = WiFi.RSSI(i);

        // Encryption type
        wifi_scan_results[i].encryptionType = WiFi.encryptionType(i);

        // Channel
        wifi_scan_results[i].channel = WiFi.channel(i);
    }

    wifi_last_scan = millis();
    LOG_INFO(TAG_WIFI, F("WiFi scan completed: %d networks found"), wifi_scan_count);

    // Clean up WiFi library's scan results
    WiFi.scanDelete();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Web Handler: GET /api/wifiscan
////////////////////////////////////////////////////////////////////////////////////////////////////

void webHandleWifiScan(AsyncWebServerRequest* request)
{
    if(!httpIsAuthenticated(request, F("wifiscan"))) {
        request->send(401, F("text/plain"), F("Unauthorized"));
        return;
    }

    // Check if we should perform a new scan
    bool force_scan = request->hasArg(F("force"));
    unsigned long time_since_scan = millis() - wifi_last_scan;

    if(force_scan || time_since_scan > WIFI_SCAN_CACHE_TIME) {
        wifiPerformScan();
    }

    // Build JSON response
    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray(F("networks"));

    for(uint16_t i = 0; i < wifi_scan_count; i++) {
        JsonObject network = networks.createNestedObject();
        network[F("ssid")] = wifi_scan_results[i].ssid;
        network[F("rssi")] = wifi_scan_results[i].rssi;
        network[F("channel")] = wifi_scan_results[i].channel;
        network[F("encryptionType")] = wifi_scan_results[i].encryptionType;

        // Add signal quality indicator
        int8_t rssi = wifi_scan_results[i].rssi;
        if(rssi >= -50) {
            network[F("quality")] = F("Excellent");
        } else if(rssi >= -60) {
            network[F("quality")] = F("Good");
        } else if(rssi >= -70) {
            network[F("quality")] = F("Fair");
        } else if(rssi >= -80) {
            network[F("quality")] = F("Weak");
        } else {
            network[F("quality")] = F("Very Weak");
        }
    }

    doc[F("count")] = wifi_scan_count;
    doc[F("timestamp")] = wifi_last_scan;

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, F("application/json"), response);

    LOG_TRACE(TAG_WIFI, F("WiFi scan results sent to client"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Web Handler: GET /api/wifiscan/encryption
// Returns encryption type name mapping
////////////////////////////////////////////////////////////////////////////////////////////////////

void webHandleWifiEncryptionTypes(AsyncWebServerRequest* request)
{
    if(!httpIsAuthenticated(request, F("wifiscan/encryption"))) {
        request->send(401, F("text/plain"), F("Unauthorized"));
        return;
    }

    DynamicJsonDocument doc(512);

    doc[F("OPEN")] = 0;
    doc[F("WEP")] = 1;
    doc[F("WPA_PSK")] = 2;
    doc[F("WPA2_PSK")] = 3;
    doc[F("WPA_WPA2_PSK")] = 4;
    doc[F("WPA2_ENTERPRISE")] = 5;

#ifdef ARDUINO_ARCH_ESP32
    doc[F("WPA3_PSK")] = 6;
    doc[F("WPA2_WPA3_PSK")] = 7;
#endif

    String response;
    serializeJson(doc, response);
    request->send(200, F("application/json"), response);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup and Integration
////////////////////////////////////////////////////////////////////////////////////////////////////

void wifiScanSetup()
{
    LOG_INFO(TAG_WIFI, F("WiFi Scan API initialized"));
}

void wifiScanLoop()
{
    // Optional: Perform periodic background scans
    // This is optional and can be triggered on-demand via API
}

#endif // HASP_USE_WIFI && HASP_USE_HTTP_ASYNC
