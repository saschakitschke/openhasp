/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#ifndef HASP_WIFI_SCAN_H
#define HASP_WIFI_SCAN_H

#include "ArduinoJson.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi scan module
 */
void wifiScanSetup();

/**
 * @brief Check if WiFi scan is in progress
 * @return true if scanning, false otherwise
 */
bool wifiScanInProgress();

/**
 * @brief Start an asynchronous WiFi network scan
 */
void wifiStartAsyncScan();

/**
 * @brief Get scan results as JSON
 * @param doc JSON document to populate
 */
void wifiGetScanResultsJson(JsonDocument& doc);

#ifdef __cplusplus
}
#endif

#endif // HASP_WIFI_SCAN_H
