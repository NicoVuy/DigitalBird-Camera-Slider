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
  Preferences preferences;
  String ssid;
  String password;
  const String default_ap_password = String("arduino1"); //needs to be minimum 8 chars, otherwise a default ESP_XXXX AP will be used without password

  Mode currentMode;
  bool serverActive;
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

  // Save settings to NVRAM
  void saveSettings() {
    logger.println("Saving Wifi settings to NVRAM...");
    preferences.begin("wifi-config", false);
    preferences.putInt("mode", static_cast<int>(currentMode));
    if (currentMode == Mode::Station) {
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
    }else{
      // Dont leak passwords in NVRAM
      preferences.putString("ssid", "");
      preferences.putString("password", "");  
    }
    preferences.end();
    logger.println("Wifi settings saved.");
  }

  void set_default_ap_settings(){
    ssid = getUniqueName();
    password = default_ap_password;
  }

  // Load settings from NVRAM
  void loadSettings() {
    logger.println("Loading Wifi settings from NVRAM...");
    preferences.begin("wifi-config", true);
    currentMode = static_cast<Mode>(preferences.getInt("mode", static_cast<int>(Mode::AP))); // Default to AP
    if (currentMode == Mode::Station) {
      ssid = preferences.getString("ssid", "");
      password = preferences.getString("password", "");
    }
    preferences.end();
    if (currentMode==Mode::Station){
      logger.printf("Settings loaded: Station mode connected to SSID %s\n", ssid.c_str());   
    }
    else if (currentMode==Mode::AP){
      set_default_ap_settings();
      logger.printf("Settings loaded: Hosting Access Point with SSID %s and password %s\n", ssid.c_str(), password.c_str()); 
    }
    else{
      logger.println("Settings loaded: ESPNOW only mode, settings reset needed to enable Wifi again");
    }
  }

  // Initialize ESP-NOW
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
    } else {
      logger.println("ESPNow Init Failed");
      delay(3000);
      ESP.restart();
    }
  }

  // Start the web server
  void startServer() {
    if (!serverActive) {
      register_mdns(); 
      logger.println("Starting web server...");
      server.begin();
      serverActive = true;
      logger.println("Web server started successfully.");
      coap_server.begin();
    }
  }

  // Stop the web server
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

  // Get status as a string
  String get_status() {
    String modeStr = currentMode == Mode::Station ? String("Station") : currentMode == Mode::AP ? String("AP") : String("ESPNOW Only");
    String ipStr = currentMode == Mode::AP ? WiFi.softAPIP().toString() : currentMode == Mode::Station && WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("Not connected");
    return String("Status: ") + modeStr + String(" | IP: ") + ipStr;
  }

  // Handle root page request
  void handleRoot() {
    logger.println(String("Handling root page request..."));
    String html;
    html.reserve(1200);
    html += "<!DOCTYPE html><html><head><title>ESP32 WiFi Config</title>";
    html += "<style>body {font-family: Arial, sans-serif; text-align: center; margin-top: 50px;}";
    html += "form {margin: 20px auto; width: 300px; padding: 20px; border: 1px solid #ccc;}";
    html += "input, button {margin: 10px; padding: 5px; width: 200px;}";
    html += "button {background-color: #4CAF50; color: white; border: none; cursor: pointer;}";
    html += "a {margin: 10px;}</style>";
    html += "</head><body><h1>ESP32 WiFi Configuration</h1>";
    html += "<h2>Current Mode: ";
    html += currentMode == Mode::Station ? String("Station + ESPNOW") : currentMode == Mode::AP ? String("Access Point + ESPNOW") : String("ESPNOW Only");
    html += "</h2>";
    html += "<p><a href='/logs'>View Logs</a> | <a href='/status'>View Status</a></p>";
    html += "<form action='/connect' method='POST'>";
    html += "<h3>Connect to WiFi</h3>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "<button type='submit'>Connect</button></form>";
    html += "<form action='/ap' method='POST'>";
    html += "<h3>Use as Access Point</h3>";
    html += "SSID: ";
    html += ssid;
    html += "<br>";
    html += "Password: ";
    html += default_ap_password;
    html += "<br>";
    html += "<button type='submit'>Set as AP</button></form>";
    html += "<form action='/espnow' method='POST'>";
    html += "<h3>Use ESPNOW Only</h3>";
    html += "<button type='submit'>Set as ESPNOW Only</button></form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    logger.printf("Default page sent to client. %d bytes\n", html.length());
  }

  // Handle logs page request
  void handleLogs() {
    logger.println(String("Handling logs page request..."));
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

  // Handle status page request
  void handleStatus() {
    logger.println(String("Handling status page request..."));
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

  // Handle WiFi connection request
  void handleConnect() {
    logger.println("Handling WiFi connection request...");
    if (server.hasArg("ssid") && server.hasArg("password")) {
      String newSSID = server.arg("ssid");
      String newPassword = server.arg("password");
      switchToStationMode(newSSID, newPassword);
      if (WiFi.status() == WL_CONNECTED) {
        server.send(200, "text/html", String("<h1>Connected to ") + newSSID + String("</h1><p>IP: ") + WiFi.localIP().toString() + String("</p><a href='/'>Back</a>"));
        logger.println("Connection success page sent to client.");
      } else {
        logger.println("Failed to connect to WiFi.");
        server.send(200, "text/html", String("<h1>Connection Failed</h1><p>Check credentials and try again.</p><a href='/'>Back</a>"));
        logger.println("Connection failure page sent to client.");
      }
    } else {
      logger.println("Bad request: Missing SSID or password.");
      server.send(400, "text/html", String("<h1>Bad Request</h1><p>Missing SSID or password.</p><a href='/'>Back</a>"));
      logger.println("Bad request page sent to client.");
    }
  }

  // Handle AP mode request
  void handleAP() {
    logger.println("Handling AP mode request...");
    switchToAPMode();
    server.send(200, "text/html", String("<h1>Access Point Started</h1><p>SSID: ") + ssid + String("<br>Password: ")+default_ap_password+String("<br>IP: ") + WiFi.softAPIP().toString() + String("<br>URL: http://") + ssid + String(".local</p><a href='/'>Back</a>"));
    logger.println("AP mode confirmation page sent to client.");
  }

  // Handle ESPNOW mode request
  void handleESPNOW() {
    logger.println("Handling ESPNOW only mode request...");
    switchToESPNOWMode();
    server.send(200, "text/html", String("<h1>ESPNOW Only Mode Activated</h1><p>Web server will stop. Reset device to access this page again.</p>"));
    logger.println("ESPNOW mode confirmation page sent to client.");
    stopServer(); // Stop server after sending response
  }

public:
  WiFiConfigManager(esp_now_recv_cb_t receiveCb, esp_now_send_cb_t sendCb, Logger& log)
    : server(80), ssid(""), password(""), currentMode(Mode::AP), serverActive(false),
      receiveCallback(receiveCb), sentCallback(sendCb), logger(log), coap_server(log) {
  
  }

  bool initiate_wifi(unsigned long timeoutMs = 20000){
    unsigned long startTime = millis();

    while (true) {
      int status = WiFi.status();
      switch (status) {
        case WL_CONNECTED:
          logger.printf("\nWiFi successfully connected");
          return true;

        case WL_NO_SSID_AVAIL:
          logger.printf("\nSSID not found");
          return false;

        case WL_CONNECT_FAILED:
          logger.printf("\nConnection failed (e.g., wrong password)");
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

      if (millis() - startTime >= timeoutMs) {
        logger.printf("\nWiFi connection timed out");
        return false;
      }

      delay(500);
      logger.print("...");
    }
  }

  void start_station_mode(){
    WiFi.mode(WIFI_STA);
    logger.printf("Attempting to connect to SSID: %s\n", ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    initiate_wifi();
    if (WiFi.status() == WL_CONNECTED) {
      logger.printf("Connected to WiFi. IP: {%s}\n", WiFi.localIP().toString().c_str());
      initESPNOW();
      startServer();
    } else {
      logger.println("Failed to connect to WiFi. Falling back to AP mode...");
      set_default_ap_settings();
      switchToAPMode();
    }
  }

  // Switch to Station mode programmatically
  void switchToStationMode(String newSSID, String newPassword) {
    logger.println("Switching to Station mode...");
    currentMode = Mode::Station;
    ssid = newSSID;
    password = newPassword;
    WiFi.disconnect();
    start_station_mode();
    saveSettings();
  }

  void register_mdns(){
    String hostname = ssid;
    logger.printf("Setting up mDNS with hostname: %s\n", hostname.c_str());
    if (MDNS.begin(hostname.c_str())) {
      logger.printf("mDNS started successfully: %s.local\n", hostname.c_str());
    } else {
      logger.println("Failed to start mDNS");
    }
  }

  void start_ap(){
    WiFi.mode(WIFI_AP_STA); // Use AP_STA for ESP-NOW compatibility
    logger.printf("Starting AP with SSID: %s PW:%s\n", ssid, password);
    String hostname = ssid;
    if (WiFi.softAP(ssid.c_str(), password.c_str()))
    {
      logger.printf("AP started with SSID %s\n",WiFi.softAPSSID().c_str());
      logger.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
      logger.printf("AP URL: http://%s.local\n", hostname.c_str());
    }
    else{
      logger.printf("AP failed to start, probable fallback to open network with SSID %s\n",WiFi.softAPSSID().c_str());
    }

    initESPNOW();

    startServer();
  }

  // Switch to AP mode programmatically
  void switchToAPMode() {
    logger.println("Switching to AP mode...");
    currentMode = Mode::AP;
    WiFi.disconnect();
    start_ap();      
    saveSettings();
  }

  void start_espnow_mode(){
    WiFi.mode(WIFI_STA);
    logger.println("WiFi mode set to Station (ESPNOW only, Wifi disconnected).");
    WiFi.disconnect();
    initESPNOW();
    stopServer();
    WiFi.disconnect();
  }

  // Switch to ESPNOW only mode programmatically
  void switchToESPNOWMode() {
    logger.println("Switching to ESPNOW only mode...");
    currentMode = Mode::ESPNOW;
    start_espnow_mode();
    saveSettings();
  }

  void setup() {
    logger.println("Starting ESP32 WiFi Configuration...");
    set_default_ap_settings();
    //loadSettings();

    String hostname = ssid;
    
    if (currentMode == Mode::AP) {
      start_ap();

    } else if (currentMode == Mode::Station) {
      start_station_mode();
    } else { // ESPNOW mode
      start_espnow_mode();
    }

    // Define server routes
    if (serverActive) {
      logger.println("Configuring server routes...");
      server.on("/", [this]() { handleRoot(); });
      server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
      server.on("/ap", HTTP_POST, [this]() { handleAP(); });
      server.on("/espnow", HTTP_POST, [this]() { handleESPNOW(); });
      server.on("/logs", [this]() { handleLogs(); });
      server.on("/status", [this]() { handleStatus(); });
    }
  }

  void loop() {
    if (serverActive) {
      server.handleClient();
      coap_server.loop();
    }
    
  }
};

#endif // WIFICONFIGMANAGER_H