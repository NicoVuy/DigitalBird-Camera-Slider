#ifndef WIFICONFIGMANAGER_H
#define WIFICONFIGMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_now.h>
#include "coap_server.h"

class WiFiConfigManager {
private:
  enum class Mode { Station, AP, ESPNOW };
  WebServer server;
  CoapServer coap_server;
  UDPViscaHandler& visca;
  Preferences preferences;
  String station_ssid;
  String station_password;
  const String ap_password = String("arduino1");
  String ap_ssid;
  bool stationEnabled;
  bool apEnabled;
  bool espnowEnabled;
  bool espnowActive;
  bool serverActive;
  bool config_applied;
  unsigned long loop_counter;
  esp_now_recv_cb_t receiveCallback;
  esp_now_send_cb_t sentCallback;
  Logger& logger;

  String getUniqueName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18] = {0};
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String("DB_") + String(macStr);
  }

  void saveSettings() {
    logger.println("Saving WiFi settings to NVRAM...");
    preferences.begin("wifi-config", false);
    preferences.putBool("stationEnabled", stationEnabled);
    preferences.putBool("apEnabled", apEnabled);
    preferences.putBool("espnowEnabled", espnowEnabled);
    if (stationEnabled) {
      preferences.putString("station_ssid", station_ssid);
      preferences.putString("station_password", station_password);
    } else {
      preferences.putString("station_ssid", "");
      preferences.putString("station_password", "");
    }
    preferences.end();
    logger.println("WiFi settings saved.");
  }

  void loadSettings() {
    logger.println("Loading WiFi settings from NVRAM...");
    preferences.begin("wifi-config", true);
    stationEnabled = preferences.getBool("stationEnabled", false);
    apEnabled = preferences.getBool("apEnabled", true); // Default to AP for fallback
    espnowEnabled = preferences.getBool("espnowEnabled", false);
    if (stationEnabled) {
      station_ssid = preferences.getString("station_ssid", "");
      station_password = preferences.getString("station_password", "");
    } 
    preferences.end();
    if (!stationEnabled && !apEnabled && !espnowEnabled) {
      logger.println("No modes enabled, falling back to AP mode");
      apEnabled = true;
      saveSettings();
    }
    String status = get_status();
    logger.printf("Settings loaded:  %s\n", status.c_str());
  }


  void initESPNOW() {
    logger.println("Initializing ESP-NOW...");
    if (esp_now_init() == ESP_OK) {
      logger.println("ESPNow Init Success");
      if (receiveCallback) {
        esp_now_register_recv_cb(receiveCallback);
      }
      if (sentCallback) {
        esp_now_register_send_cb(sentCallback);
      }
      espnowActive=true;
    } else {
      logger.println("ESPNow Init Failed, falling back to AP only mode...");
      apEnabled = true;
      stationEnabled = false;
      espnowEnabled = false;
    }
  }

  void startServer() {
    if (!serverActive) {
      register_mdns();
      logger.println("Starting web server...");
      server.begin();
      serverActive = true;
      logger.println("Web server started successfully.");
      coap_server.begin();
      visca.begin();
    }
  }

  void stopServer() {
    if (serverActive) {
      logger.println("Stopping web server...");
      MDNS.end();
      server.close();
      serverActive = false;
      logger.println("Web server stopped.");
      coap_server.end();
    }
  }

  String get_status() {
    String extra = " ";
    if (espnowEnabled) extra += "ESPNOW ";
    String stationIP = stationEnabled ? (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Not connected") : "disabled";
    String stationSSID = String(" SSID: ")+WiFi.SSID() + String(" ")+get_wifi_status();
    String apIP = apEnabled ? WiFi.softAPIP().toString() : "N/A";
    return String("Station IP: ") + stationIP  + stationSSID + String(" | AP IP: ") + apIP +extra;
  }

  void handleRoot() {
    logger.println("Handling root page request...");
    String html;
    html.reserve(1400);
    html += "<!DOCTYPE html><html><head><title>ESP32 WiFi Config</title>";
    html += "<style>body {font-family: Arial, sans-serif; text-align: center; margin-top: 50px;}";
    html += "form {margin: 20px auto; width: 300px; padding: 20px; border: 1px solid #ccc;}";
    html += "input, button {margin: 10px; padding: 5px;}";
    html += "button {background-color: #4CAF50; color: white; border: none; cursor: pointer; width: 200px;}";
    html += "a {margin: 10px;} label {display: block; text-align: left;}";
    html += "#stationFields {display: none;} .checkbox-container {text-align: left; margin-left: 50px;}";
    html += "</style><script>";
    html += "function toggleStationFields() {";
    html += "  var checkBox = document.getElementById('stationMode');";
    html += "  var fields = document.getElementById('stationFields');";
    html += "  fields.style.display = checkBox.checked ? 'block' : 'none';";
    html += "}</script></head>";
    html += "<body><h1>ESP32 WiFi Configuration</h1>";
    html += "<h2>";
    html += get_status();
    html += "</h2>";
    html += "<p><a href='/logs'>View Logs</a> | <a href='/status'>View Status</a></p>";
    html += "<form action='/configure' method='POST'>";
    html += "<h3>Configure WiFi Modes</h3>";
    html += "<div class='checkbox-container'>";
    html += "<label><input type='checkbox' name='station' id='stationMode' " + String(stationEnabled ? "checked" : "") + " onchange='toggleStationFields()'> Station Mode</label>";
    html += "<div id='stationFields' " + String(stationEnabled ? "style='display:block;'" : "") + ">";
    html += "SSID: <input type='text' name='station_ssid' value='" + station_ssid + "'><br>";
    html += "password: <input type='password' name='station_password' value='" + station_password + "'><br>";
    html += "</div>";
    html += "<label><input type='checkbox' name='ap' " + String(apEnabled ? "checked" : "") + "> Access Point Mode (SSID: " + ap_ssid + ", password: " + ap_password + ")</label>";
    html += "<label><input type='checkbox' name='espnow' " + String(espnowEnabled ? "checked" : "") + "> ESPNOW Mode</label>";
    html += "</div>";
    html += "<button type='submit'>Apply Configuration</button></form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    logger.printf("Default page sent to client. %d bytes\n", html.length());
  }

  void handleLogs() {
    logger.println("Handling logs page request...");
    String html;
    html.reserve(1024);
    html += "<!DOCTYPE html><html><head><title>ESP32 Logs</title>";
    html += "<style>body {font-family: Arial, sans-serif; text-align: center; margin-top: 50px;}";
    html += "pre {text-align: left; display: inline-block;}</style>";
    html += "</head><body><h1>ESP32 Logs</h1>";
    html += "<p><a href='/'>Back to Home</a></p>";
    html += "<pre>";
    html += logger.getLogBuffer();
    html += "</pre>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    logger.printf("Log page sent to client. %d bytes\n", html.length());
  }

  void handleStatus() {
    logger.println("Handling status page request...");
    String html;
    html.reserve(512);
    html += "<!DOCTYPE html><html><head><title>ESP32 Status</title>";
    html += "<style>body {font-family: Arial, sans-serif; text-align: center; margin-top: 50px;}</style>";
    html += "</head><body><h1>ESP32 Status</h1>";
    html += "<p><a href='/'>Back to Home</a></p>";
    html += "<p>";
    html += get_status();
    html += "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    logger.printf("Status page sent to client. %d bytes\n", html.length());
  }

  void handleConfigure() {
    logger.println("Handling configuration request...");
    bool newStationEnabled = server.hasArg("station");
    bool newAPEnabled = server.hasArg("ap");
    bool newESPNOWEnabled = server.hasArg("espnow");
    String newstation_ssid = server.hasArg("station_ssid") ? server.arg("station_ssid") : "";
    String newstation_password = server.hasArg("station_password") ? server.arg("station_password") : "";

    if (!newStationEnabled && !newAPEnabled && !newESPNOWEnabled) {
      logger.println("No modes selected, falling back to AP mode...");
      newAPEnabled = true;
    }

    stationEnabled = newStationEnabled;
    apEnabled = newAPEnabled;
    espnowEnabled = newESPNOWEnabled;
    if (stationEnabled) {
      station_ssid = newstation_ssid;
      station_password = newstation_password;
    } else {
      station_ssid = "";
      station_password = "";
    }

    stopServer();

    applyConfiguration();
    String response = "<h1>Configuration Applied</h1><p>";
    response += get_status();
    response += "</p><a href='/'>Back</a>";
    server.send(200, "text/html", response);
    logger.println("Configuration confirmation page sent to client.");
  }

  String get_wifi_status()
  {
    int status = WiFi.status();
    switch (status) {
      case WL_CONNECTED:
        return String("WiFi successfully connected");
      case WL_NO_SSID_AVAIL:
        return String("SSID not found");
      case WL_CONNECT_FAILED:
        return String("Connection failed (e.g., wrong station_password)");
      case WL_NO_SHIELD:
        return String("No WiFi hardware detected");
      case WL_IDLE_STATUS:
        return String("WiFi idle");
      case WL_DISCONNECTED:
        return String("WiFi disconnected");
      case WL_SCAN_COMPLETED:
        return String("WiFi scan completed");
      default:      
        break;
    }  
    return String("Unknown WiFi status");
  }

  bool initiate_station_wifi(unsigned long timeoutMs = 20000) {
    unsigned long startTime = millis();
    while (true) {
      int status = WiFi.status();
      switch (status) {
        case WL_CONNECTED:
          logger.printf("\nWiFi successfully connected");
          return true;
        case WL_NO_SSID_AVAIL:
          logger.printf("\nstation_ssid not found");
          return false;
        case WL_CONNECT_FAILED:
          logger.printf("\nConnection failed (e.g., wrong station_password)");
          return false;
        case WL_NO_SHIELD:
          logger.printf("\nNo WiFi hardware detected");
          return false;
        case WL_IDLE_STATUS:
          logger.printf("\nWiFi idle");
          break;
        case WL_DISCONNECTED:
          logger.printf("\nWiFi disconnected");
          break;
        case WL_SCAN_COMPLETED:
          logger.printf("\nWiFi scan completed");
          break;
        default:
          logger.printf("\nUnknown WiFi status: %d ", status);
          break;
      }
      unsigned long wait_time=millis() - startTime;
      if ( wait_time>= timeoutMs) {
        logger.printf("\nWiFi connection timed out after %ld millis", wait_time);
        return false;
      }
      delay(500);
      logger.print("...");
    }
  }

  


  void register_mdns() {
    String hostname =  getUniqueName();
    logger.printf("Setting up mDNS with hostname: %s\n", hostname.c_str());
    if (MDNS.begin(hostname.c_str())) {
      logger.printf("mDNS started successfully: %s.local\n", hostname.c_str());
    } else {
      logger.println("Failed to start mDNS");
    }
  }




  wifi_mode_t get_mode(){
    wifi_mode_t mode=WIFI_STA;
    if (espnowEnabled){
      // needs AP mode to keep radios powered.
      if (stationEnabled){
        mode = WIFI_AP_STA;
      }
      else{
        mode = WIFI_AP; 
      }
    }
    else {
      if (stationEnabled && apEnabled){
        mode = WIFI_AP_STA;
      }
      else if (stationEnabled){
        mode = WIFI_STA;
      }
      else {
        mode = WIFI_AP;
      }
    }
    return mode;
  }
  void applyConfiguration() {
    logger.println("Applying configuration...");
    espnowActive=false;
    WiFi.disconnect();

    // Reconfigure WiFi
    WiFi.mode(get_mode());
    if (stationEnabled){
      logger.printf("Attempting to connect to access point %s\n", station_ssid.c_str());
      WiFi.begin(station_ssid.c_str(), station_password.c_str());
      if (initiate_station_wifi()) {
        logger.printf("Connected to WiFi access point %s with IP: %s\n", station_ssid.c_str(), WiFi.localIP().toString().c_str());
      } else {
        logger.println("Failed to connect to WiFi, falling back to AP mode...");
        stationEnabled=false;
        apEnabled=true; //make sure we can always reconfigure via AP mode
        WiFi.disconnect();
        WiFi.mode(get_mode());
      }
    }

    if (apEnabled){
      if (WiFi.softAP(ap_ssid.c_str(), ap_password.c_str())) {
        logger.printf("\nAP started with SSID %s and password %s", WiFi.softAPSSID().c_str(),ap_password.c_str());
        logger.printf("\nAP IP: %s", WiFi.softAPIP().toString().c_str());
      } 
      else 
      {
        logger.printf("\nAP failed to start, probable fallback to open network with SSID %s\n", WiFi.softAPSSID().c_str());
      }
    }

    if (espnowEnabled){
      initESPNOW();
    }

    saveSettings();
    startServer();
  }

public:
  WiFiConfigManager(esp_now_recv_cb_t receiveCb, esp_now_send_cb_t sendCb, Logger& log, UDPViscaHandler& visca_handler)
    : server(80), station_ssid(""), station_password(""), stationEnabled(false), apEnabled(true), espnowEnabled(false),espnowActive(false),
      serverActive(false), receiveCallback(receiveCb), sentCallback(sendCb), logger(log),
      coap_server(log), visca(visca_handler), config_applied(false), loop_counter(0) {
        ap_ssid=getUniqueName();
  }

  bool is_espnow_active(){
    return espnowActive;
  }

  void setup() {
    loadSettings();
    applyConfiguration();
  }

  bool loop() {
    if (loop_counter%10000==0){
      logger.println(get_status().c_str());  
      WiFi.printDiag(Serial);
    }
    loop_counter+=1;

    if (!config_applied){
      logger.println("Starting ESP32 WiFi Configuration...");
      applyConfiguration();

      if (serverActive) {
        logger.println("Configuring server routes...");
        server.on("/", [this]() { handleRoot(); });
        server.on("/configure", HTTP_POST, [this]() { handleConfigure(); });
        server.on("/logs", [this]() { handleLogs(); });
        server.on("/status", [this]() { handleStatus(); });
      }
      config_applied=true;
    }
    if (serverActive) {
      server.handleClient();
      coap_server.loop();
      return visca.processPackets();
    }
    return false;
  }
};

#endif // WIFICONFIGMANAGER_H