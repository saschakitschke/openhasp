/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#include "hasplib.h"

#if HASP_USE_WIFI > 0 && HASP_USE_HTTP_ASYNC > 0

#include "hasp_conf.h"
#include <ArduinoJson.h>
#include "ArduinoLog.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Scan Results Cache
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char ssid[33];
    int32_t rssi;
    uint8_t channel;
    bool secure;
    char bssid[18];
    uint8_t encryptionType;
} hasp_wifi_network_t;

static hasp_wifi_network_t* wifi_scan_results = NULL;
static uint16_t wifi_scan_count = 0;
static bool wifi_scan_in_progress = false;
static unsigned long wifi_last_scan = 0;
static const uint32_t WIFI_SCAN_CACHE_TIME = 30000; // Cache results for 30 seconds

////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////
extern bool httpIsAuthenticated(AsyncWebServerRequest* request, const __FlashStringHelper* notused);

////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Scan Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

void wifiScanSetup()
{
    LOG_INFO(TAG_WIFI, F("WiFi Scan API initialized"));
}

bool wifiScanInProgress()
{
    return wifi_scan_in_progress;
}

void wifiStartAsyncScan()
{
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if(!wifi_scan_in_progress) {
        wifi_scan_in_progress = true;
        LOG_INFO(TAG_WIFI, F("Starting asynchronous WiFi network scan..."));

#if defined(ARDUINO_ARCH_ESP32)
        WiFi.scanNetworks(true); // async scan
#elif defined(ARDUINO_ARCH_ESP8266)
        WiFi.scanNetworks(true); // async scan
#endif
    }
#endif
}

void wifiProcessScanResults()
{
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    int16_t scan_status = WiFi.scanComplete();

    if(scan_status == WIFI_SCAN_RUNNING) {
        // Scan still in progress
        return;
    }

    if(scan_status > 0) {
        // Scan completed successfully
        int16_t n = scan_status;

        // Free previous results
        if(wifi_scan_results != NULL) {
            free(wifi_scan_results);
            wifi_scan_results = NULL;
        }

        // Allocate memory for new results
        wifi_scan_results = (hasp_wifi_network_t*)malloc(n * sizeof(hasp_wifi_network_t));
        if(wifi_scan_results == NULL) {
            LOG_ERROR(TAG_WIFI, F("Failed to allocate memory for WiFi scan results"));
            wifi_scan_count = 0;
            wifi_scan_in_progress = false;
            return;
        }

        wifi_scan_count = n;

        // Copy scan results
        for(int16_t i = 0; i < n; i++) {
            // SSID
            String ssid = WiFi.SSID(i);
            strncpy(wifi_scan_results[i].ssid, ssid.c_str(), sizeof(wifi_scan_results[i].ssid) - 1);
            wifi_scan_results[i].ssid[sizeof(wifi_scan_results[i].ssid) - 1] = '\0';

            // Signal strength
            wifi_scan_results[i].rssi = WiFi.RSSI(i);

            // Channel
            wifi_scan_results[i].channel = WiFi.channel(i);

            // Encryption/Security
#if defined(ARDUINO_ARCH_ESP32)
            wifi_scan_results[i].encryptionType = WiFi.encryptionType(i);
            wifi_scan_results[i].secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            String bssid = WiFi.BSSIDstr(i);
            strncpy(wifi_scan_results[i].bssid, bssid.c_str(), sizeof(wifi_scan_results[i].bssid) - 1);
#elif defined(ARDUINO_ARCH_ESP8266)
            wifi_scan_results[i].secure = !WiFi.isHidden(i);
            wifi_scan_results[i].encryptionType = 0;
            strncpy(wifi_scan_results[i].bssid, "", sizeof(wifi_scan_results[i].bssid) - 1);
#endif
            wifi_scan_results[i].bssid[sizeof(wifi_scan_results[i].bssid) - 1] = '\0';
        }

        wifi_last_scan = millis();
        wifi_scan_in_progress = false;
        LOG_INFO(TAG_WIFI, F("WiFi scan completed: %d networks found"), wifi_scan_count);

        // Clean up
#if defined(ARDUINO_ARCH_ESP32)
        WiFi.scanDelete();
#endif
    } else if(scan_status == WIFI_SCAN_FAILED) {
        LOG_ERROR(TAG_WIFI, F("WiFi scan failed"));
        wifi_scan_in_progress = false;
    }
#endif
}

void wifiGetScanResultsJson(JsonDocument& doc)
{
    JsonObject root = doc.to<JsonObject>();
    root[F("ok")] = true;
    root[F("scanning")] = wifi_scan_in_progress;

    JsonArray networks = root.createNestedArray(F("networks"));

    if(!wifi_scan_in_progress && wifi_scan_results != NULL) {
        for(uint16_t i = 0; i < wifi_scan_count; i++) {
            JsonObject network = networks.createNestedObject();
            network[F("ssid")] = wifi_scan_results[i].ssid;
            network[F("rssi")] = wifi_scan_results[i].rssi;
            network[F("channel")] = wifi_scan_results[i].channel;
            network[F("secure")] = wifi_scan_results[i].secure;
            network[F("bssid")] = wifi_scan_results[i].bssid;
            network[F("encryption")] = wifi_scan_results[i].encryptionType;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP Handlers
////////////////////////////////////////////////////////////////////////////////////////////////////

void webHandleHaspStudioWifiScan(AsyncWebServerRequest* request)
{
    if(!httpIsAuthenticated(request, F("hasp-studio/wifi/scan"))) {
        request->send(401, F("application/json"), F("{\"ok\":false,\"error\":\"Unauthorized\"}"));
        return;
    }

    // Process any completed scans first
    wifiProcessScanResults();

    // Check if we should start a new scan
    bool force_scan = request->hasArg(F("force"));
    unsigned long time_since_scan = millis() - wifi_last_scan;

    if(force_scan || time_since_scan > WIFI_SCAN_CACHE_TIME || wifi_scan_count == 0) {
        if(!wifi_scan_in_progress) {
            wifiStartAsyncScan();
        }
    }

    // Build JSON response
    DynamicJsonDocument doc(3072);
    wifiGetScanResultsJson(doc);

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, F("application/json"), response);

    LOG_TRACE(TAG_HTTP, F("WiFi scan results sent to client"));
}

void webHandleHaspStudioStatus(AsyncWebServerRequest* request)
{
    if(!httpIsAuthenticated(request, F("hasp-studio/status"))) {
        request->send(401, F("application/json"), F("{\"ok\":false,\"error\":\"Unauthorized\"}"));
        return;
    }

    DynamicJsonDocument doc(512);
    JsonObject root = doc.to<JsonObject>();

    root[F("ok")] = true;
    root[F("hostname")] = haspDevice.get_hostname();

#if HASP_USE_WIFI > 0
    if(WiFi.status() == WL_CONNECTED) {
        root[F("ip")] = WiFi.localIP().toString();
        root[F("ssid")] = WiFi.SSID();
        root[F("rssi")] = WiFi.RSSI();
    } else {
        root[F("ip")] = "0.0.0.0";
        root[F("ssid")] = "";
        root[F("rssi")] = 0;
    }
#endif

    String response;
    serializeJson(doc, response);
    request->send(200, F("application/json"), response);

    LOG_TRACE(TAG_HTTP, F("HASP Studio status sent to client"));
}

#endif // HASP_USE_WIFI && HASP_USE_HTTP_ASYNC
