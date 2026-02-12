#include "WiFiManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <time.h>

#define NTP_SERVER "pool.ntp.org"
#define AP_SSID_PREFIX "sensor-"

WiFiManager::WiFiManager(Config* cfg, uint8_t led) 
    : config(cfg), ledPin(led), server(nullptr) {
}

WiFiManager::~WiFiManager() {
    if (server) {
        delete server;
    }
}

bool WiFiManager::connect() {
    unsigned long lastBlink = 0;
    bool ledState = false;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config->ssid, config->password);
    
    Serial.printf("Connecting to %s", config->ssid);
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(250);
        Serial.print(".");
        
        if (millis() - lastBlink > 500) {
            ledState = !ledState;
            digitalWrite(ledPin, ledState ? LOW : HIGH);
            lastBlink = millis();
        }
        
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        return false;
    }
    
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    return true;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::syncNTP() {
    Serial.println("Syncing time with NTP...");
    
    configTime(0, 0, NTP_SERVER);
    
    int attempts = 0;
    time_t now = time(nullptr);
    while (now < 1000000000 && attempts < 20) {
        delay(500);
        now = time(nullptr);
        attempts++;
    }
    
    if (now > 1000000000) {
        Serial.printf("Time synced: %s", ctime(&now));
        config->updateTimeOffset(now);
        config->save();
        return true;
    } else {
        Serial.println("NTP sync failed!");
        return false;
    }
}

uint32_t WiFiManager::getCurrentTime() const {
    time_t now = time(nullptr);
    return (now > 1000000000) ? now : (millis() / 1000);
}

void WiFiManager::startConfigMode() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    apSSID = String(AP_SSID_PREFIX) + 
             String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
    
    Serial.printf("Creating AP: %s\n", apSSID.c_str());
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    if (!server) {
        server = new ESP8266WebServer(80);
    }
    
    server->on("/", HTTP_GET, [this]() { this->handleRoot(); });
    server->on("/save", HTTP_POST, [this]() { this->handleSave(); });
    server->begin();
    
    Serial.println("Web server started");
}

void WiFiManager::handleClient() {
    if (server) {
        server->handleClient();
    }
}

String WiFiManager::loadHTMLFile(const char* filename) {
    if (!LittleFS.exists(filename)) {
        return "";
    }
    
    File file = LittleFS.open(filename, "r");
    String content = file.readString();
    file.close();
    
    return content;
}

String WiFiManager::replaceVariables(String html) {
    html.replace("%DEVICE_ID%", apSSID);
    html.replace("%SSID%", config->ssid);
    html.replace("%PASSWORD%", config->password);
    html.replace("%INTERVAL%", String(config->interval > 0 ? config->interval : 1800));
    html.replace("%SERVER%", config->influxServer);
    html.replace("%PORT%", String(config->influxPort > 0 ? config->influxPort : 8086));
    html.replace("%DATABASE%", config->influxDb);
    html.replace("%USER%", config->influxUser);
    html.replace("%DBPASS%", config->influxPass);
    html.replace("%MEASUREMENT%", 
                 strlen(config->influxMeasurement) > 0 ? config->influxMeasurement : "environment");
    
    return html;
}

void WiFiManager::handleRoot() {
    String html = loadHTMLFile("/config.html");
    
    if (html.length() == 0) {
        server->send(404, "text/plain", "config.html not found");
        return;
    }
    
    html = replaceVariables(html);
    server->send(200, "text/html", html);
}

void WiFiManager::handleSave() {
    Serial.println("Saving configuration...");
    
    strncpy(config->ssid, server->arg("ssid").c_str(), sizeof(config->ssid) - 1);
    strncpy(config->password, server->arg("password").c_str(), sizeof(config->password) - 1);
    config->interval = server->arg("interval").toInt();
    strncpy(config->influxServer, server->arg("server").c_str(), sizeof(config->influxServer) - 1);
    config->influxPort = server->arg("port").toInt();
    strncpy(config->influxDb, server->arg("database").c_str(), sizeof(config->influxDb) - 1);
    strncpy(config->influxUser, server->arg("user").c_str(), sizeof(config->influxUser) - 1);
    strncpy(config->influxPass, server->arg("dbpass").c_str(), sizeof(config->influxPass) - 1);
    strncpy(config->influxMeasurement, server->arg("measurement").c_str(), 
            sizeof(config->influxMeasurement) - 1);
    
    config->save();
    
    String html = loadHTMLFile("/success.html");
    if (html.length() == 0) {
        server->send(200, "text/plain", "Configuration saved! Restarting...");
    } else {
        server->send(200, "text/html", html);
    }
    
    delay(5000);
    ESP.restart();
}

void WiFiManager::blinkLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    if (millis() - lastBlink > 500) {
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? LOW : HIGH);
        lastBlink = millis();
    }
}
