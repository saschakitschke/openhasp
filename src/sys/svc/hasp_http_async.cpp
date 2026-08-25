/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

//#include "webServer.h"
#include "hasplib.h"

#if HASP_USE_HTTP_ASYNC > 0

#include "ArduinoLog.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "Update.h"
#endif

#include "hasp_conf.h"
#include "dev/device.h"
#include "hal/hasp_hal.h"

#include "hasp_gui.h"
#include "hasp_debug.h"

#include "sys/net/hasp_network.h"

#if HASP_USE_WIFI > 0
#include "sys/svc/hasp_wifi_scan.h"
#endif

/* clang-format off */
//default theme
#ifndef D_HTTP_COLOR_TEXT
#define D_HTTP_COLOR_TEXT               "#000"       // Global text color - Black
#endif
#ifndef D_HTTP_COLOR_BACKGROUND
#define D_HTTP_COLOR_BACKGROUND         "#fff"       // Global background color - White
#endif
#ifndef D_HTTP_COLOR_INPUT_TEXT
#define D_HTTP_COLOR_INPUT_TEXT         "#000"       // Input text color - Black
#endif
#ifndef D_HTTP_COLOR_INPUT
#define D_HTTP_COLOR_INPUT              "#fff"       // Input background color - White
#endif
#ifndef D_HTTP_COLOR_INPUT_WARNING
#define D_HTTP_COLOR_INPUT_WARNING      "#f00"       // Input warning border color - Red
#endif
#ifndef D_HTTP_COLOR_BUTTON_TEXT
#define D_HTTP_COLOR_BUTTON_TEXT        "#fff"       // Button text color - White
#endif
#ifndef D_HTTP_COLOR_BUTTON
#define D_HTTP_COLOR_BUTTON             "#1fa3ec"    // Button color - Vivid blue
#endif
#ifndef D_HTTP_COLOR_BUTTON_RESET
#define D_HTTP_COLOR_BUTTON_RESET       "#f00"       // Restart/Reset button color - red
#endif
/* clang-format on */

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
File fsUploadFile;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
bool webServerStarted = false;

// bool httpEnable       = true;
// uint16_t httpPort     = 80;
// char httpUser[32]     = "";
// char httpPassword[MAX_PASSWORD_LENGTH] = "";
hasp_http_config_t http_config;

#define HTTP_PAGE_SIZE (6 * 256)

#if defined(STM32F4xx) && HASP_USE_ETHERNET > 0
#include <EthernetWebServer_STM32.h>
EthernetWebServer webServer(80);
#endif

#if defined(STM32F4xx) && HASP_USE_WIFI > 0
#include <EthernetWebServer_STM32.h>
// #include <WiFi.h>
EthernetWebServer webServer(80);
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include "StringStream.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <detail/mimetable.h>
AsyncWebServer webServer(80);
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <detail/mimetable.h>
AsyncWebServer webServer(80);
extern const uint8_t EDIT_HTM_GZ_START[] asm("_binary_data_edit_htm_gz_start");
extern const uint8_t EDIT_HTM_GZ_END[] asm("_binary_data_edit_htm_gz_end");
#endif // ESP32

AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
