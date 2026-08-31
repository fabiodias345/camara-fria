// ============================================
// CÂMARA FRIA - Cliente API (Envio ao Supabase)
// ============================================
#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "modbus_reader.h"
#include "door_sensor.h"

struct TelemetryPayload {
    float temperature;
    float setpoint;
    float heater_output;
    float inverter_frequency;
    float inverter_voltage;
    float inverter_current;
    float inverter_dc_voltage;
    float inverter_temperature;
    bool  inverter_running;
    uint16_t inverter_fault_code;
    bool  door_open;
    uint32_t door_open_seconds;
    DoorAlarmStage door_stage;
};

class ApiClient {
public:
    ApiClient();

    bool sendTelemetry(const TelemetryPayload &payload);
    
    // Configura URL e chave da API
    void setEndpoint(const char* host, uint16_t port, const char* path);
    void setApiKey(const char* key);

private:
    char _host[64];
    uint16_t _port;
    char _path[128];
    char _api_key[256];
    int _retry_count;
    
    String buildJson(const TelemetryPayload &payload);
};

#endif // API_CLIENT_H
