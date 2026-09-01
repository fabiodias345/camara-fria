// ============================================
// CÂMARA FRIA - Implementação Cliente API
// HF-006Enet: Comunicação via Ethernet W5500
// ============================================
#include "api_client.h"

ApiClient::ApiClient() : _port(SUPABASE_PORT), _retry_count(0) {
    strncpy(_host, SUPABASE_HOST, sizeof(_host));
    strncpy(_path, SUPABASE_API_PATH, sizeof(_path));
    strncpy(_api_key, SUPABASE_ANON_KEY, sizeof(_api_key));
}

void ApiClient::setEndpoint(const char* host, uint16_t port, const char* path) {
    strncpy(_host, host, sizeof(_host));
    _port = port;
    strncpy(_path, path, sizeof(_path));
}

void ApiClient::setApiKey(const char* key) {
    strncpy(_api_key, key, sizeof(_api_key));
}

String ApiClient::buildJson(const TelemetryPayload &payload) {
    JsonDocument doc;
    
    doc["temperature"] = round(payload.temperature * 10.0f) / 10.0f;
    doc["setpoint"] = round(payload.setpoint * 10.0f) / 10.0f;
    doc["heater_output"] = round(payload.heater_output * 10.0f) / 10.0f;
    doc["inverter_frequency"] = round(payload.inverter_frequency * 100.0f) / 100.0f;
    doc["inverter_voltage"] = round(payload.inverter_voltage * 10.0f) / 10.0f;
    doc["inverter_current"] = round(payload.inverter_current * 100.0f) / 100.0f;
    doc["inverter_dc_voltage"] = round(payload.inverter_dc_voltage * 10.0f) / 10.0f;
    doc["inverter_temperature"] = round(payload.inverter_temperature * 10.0f) / 10.0f;
    doc["inverter_running"] = payload.inverter_running;
    doc["inverter_fault_code"] = payload.inverter_fault_code;
    doc["door_open"] = payload.door_open;
    doc["door_open_seconds"] = payload.door_open_seconds;
    doc["door_stage"] = (int)payload.door_stage;
    doc["timestamp"] = millis();
    doc["device_id"] = DEVICE_ID;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ApiClient::sendTelemetry(const TelemetryPayload &payload) {
    // Verifica se há conexão de rede (chamado externamente pelo main)
    // A verificação de link é feita no main.cpp antes de chamar esta função
    
    HTTPClient http;
    String url = String("https://") + _host + _path;
    
    #if DEBUG_SERIAL
    Serial.printf("[API] POST %s\n", url.c_str());
    #endif
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", _api_key);
    http.addHeader("Authorization", String("Bearer ") + _api_key);
    http.setTimeout(10000);  // 10s timeout
    
    String body = buildJson(payload);
    
    int httpCode = http.POST(body);
    
    if (httpCode > 0) {
        #if DEBUG_SERIAL
        Serial.printf("[API] Resposta: %d\n", httpCode);
        if (httpCode == 200 || httpCode == 201) {
            String response = http.getString();
            Serial.printf("[API] Body: %s\n", response.c_str());
        }
        #endif
        
        http.end();
        _retry_count = 0;
        return (httpCode >= 200 && httpCode < 300);
    } else {
        #if DEBUG_SERIAL
        Serial.printf("[API] ERRO HTTP: %s\n", http.errorToString(httpCode).c_str());
        #endif
        http.end();
        _retry_count++;
        return false;
    }
}
