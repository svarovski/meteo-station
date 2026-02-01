#include "InfluxDBWrapper.h"

InfluxDBWrapper::InfluxDBWrapper() : client(nullptr), sensorPoint(nullptr), config(nullptr), initialized(false) {
}

InfluxDBWrapper::~InfluxDBWrapper() {
    if (sensorPoint) {
        delete sensorPoint;
    }
    if (client) {
        delete client;
    }
}

bool InfluxDBWrapper::begin(Config* cfg) {
    config = cfg;
    
    if (!config || !config->isValid()) {
        Serial.println("Invalid configuration for InfluxDB");
        return false;
    }
    
    // Create InfluxDB client instance
    String serverUrl = "http://" + String(config->influxServer) + ":" + String(config->influxPort);
    
    client = new InfluxDBClient(serverUrl.c_str(), config->influxDb);
    
    // Set authentication if provided
    if (strlen(config->influxUser) > 0) {
        client->setConnectionParams(serverUrl.c_str(), config->influxDb, 
                                    config->influxUser, config->influxPass);
    }
    
    // Create point for sensor data
    sensorPoint = new Point(config->influxMeasurement);
    
    initialized = true;
    
    Serial.printf("InfluxDB client initialized: %s\n", serverUrl.c_str());
    return true;
}

bool InfluxDBWrapper::validateConnection() {
    if (!initialized || !client) {
        return false;
    }
    
    if (client->validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client->getServerUrl());
        return true;
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client->getLastErrorMessage());
        return false;
    }
}

bool InfluxDBWrapper::writeSensorRecord(const SensorRecord& record, uint32_t timeOffset) {
    if (!initialized || !client || !sensorPoint) {
        return false;
    }
    
    // Clear previous data
    sensorPoint->clearFields();
    sensorPoint->clearTags();
    
    // Add fields
    sensorPoint->addField("temperature", record.getTemperature());
    sensorPoint->addField("humidity", record.getHumidity());
    
    // Set timestamp (in seconds, InfluxDB wants nanoseconds)
    uint32_t timestampSeconds = record.getTimestampSeconds(timeOffset);
    sensorPoint->setTime(timestampSeconds);
    
    // Write to InfluxDB
    if (!client->writePoint(*sensorPoint)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client->getLastErrorMessage());
        return false;
    }
    
    return true;
}

bool InfluxDBWrapper::writeBatteryVoltage(float voltage) {
    if (!initialized || !client || !sensorPoint) {
        return false;
    }
    
    sensorPoint->clearFields();
    sensorPoint->clearTags();
    
    sensorPoint->addField("battery_voltage", voltage);
    sensorPoint->setTime(WritePrecision::S);
    
    if (!client->writePoint(*sensorPoint)) {
        Serial.print("InfluxDB battery write failed: ");
        Serial.println(client->getLastErrorMessage());
        return false;
    }
    
    return true;
}

bool InfluxDBWrapper::flush() {
    if (!initialized || !client) {
        return false;
    }
    
    // The library handles buffering internally
    // This is a placeholder for future batch operations
    return true;
}

String InfluxDBWrapper::getLastError() const {
    if (!initialized || !client) {
        return "Client not initialized";
    }
    
    return client->getLastErrorMessage();
}
