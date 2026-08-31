// ============================================
// CÂMARA FRIA - Firmware ESP32 Principal
// HF-006Enet (Hoffer Automação)
// ============================================
#include <Arduino.h>
#include "config.h"
#include "modbus_reader.h"
#include "door_sensor.h"
#include "buzzer.h"
#include "network.h"
#include "api_client.h"

// ---- Objetos globais ----
ModbusReader modbusReader;
DoorSensor doorSensor;
Buzzer buzzer;
NetworkManager network;
ApiClient apiClient;

// ---- Timers ----
unsigned long lastModbusRead = 0;
unsigned long lastTelemetrySend = 0;
unsigned long lastStatusLog = 0;

// ---- Estado de alerta ----
bool tempAlertSent = false;
bool tempCriticalSent = false;

// ============================================
// SETUP
// ============================================
void setup() {
    #if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    delay(1000);
    Serial.println("========================================");
    Serial.println("  CÂMARA FRIA - HF-006Enet");
    Serial.println("  Sistema de Monitoramento v1.0.0");
    Serial.println("  Hoffer Automação Industrial");
    Serial.println("========================================");
    Serial.println();
    #endif

    network.begin();
    modbusReader.begin();
    doorSensor.begin();
    buzzer.begin();
    apiClient.setEndpoint(SUPABASE_HOST, SUPABASE_PORT, SUPABASE_API_PATH);
    apiClient.setApiKey(SUPABASE_ANON_KEY);

    #if DEBUG_SERIAL
    Serial.println("[INIT] ✅ Sistema inicializado!");
    Serial.printf("[INIT] IP: %s\n", network.getLocalIP().c_str());
    Serial.printf("[INIT] API: http://%s:%d%s\n", SUPABASE_HOST, SUPABASE_PORT, SUPABASE_API_PATH);
    Serial.println("========================================\n");
    #endif
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void loop() {
    unsigned long now = millis();

    network.maintain();

    // Leitura Modbus (a cada 5 segundos)
    if (now - lastModbusRead >= TEMP_POLL_INTERVAL_MS) {
        lastModbusRead = now;
        #if DEBUG_SERIAL
        Serial.println("\n[LOOP] --- Lendo dispositivos Modbus ---");
        #endif
        modbusReader.readAll();
        
        TemperatureData tempData = modbusReader.getTemperatureData();
        if (tempData.communication_ok) {
            // Alerta de temperatura
            if (tempData.current_temp >= TEMP_MAX_ALERT && !tempAlertSent) {
                tempAlertSent = true;
                #if DEBUG_SERIAL
                Serial.printf("[ALERT] ⚠️ Temperatura ALERTA: %.1f°C\n", tempData.current_temp);
                #endif
            }
            if (tempData.current_temp >= TEMP_CRITICAL && !tempCriticalSent) {
                tempCriticalSent = true;
                buzzer.setMode(BUZZER_TEMP_CRITICAL);
                #if DEBUG_SERIAL
                Serial.printf("[ALERT] 🔴 Temperatura CRÍTICA: %.1f°C\n", tempData.current_temp);
                #endif
            }
            if (tempData.current_temp < TEMP_MAX_ALERT) {
                tempAlertSent = false;
                tempCriticalSent = false;
            }
        }
    }

    // Sensor de porta
    doorSensor.update();

    // Controle do buzzer
    if (doorSensor.isOpen()) {
        switch (doorSensor.getStage()) {
            case DOOR_OPEN_WARNING: buzzer.setMode(BUZZER_PULSE_WARNING); break;
            case DOOR_OPEN_PULSE: buzzer.setMode(BUZZER_PULSE_INTERVAL); break;
            case DOOR_OPEN_CONTINUOUS: buzzer.setMode(BUZZER_CONTINUOUS); break;
            default: break;
        }
    } else {
        TemperatureData tempData = modbusReader.getTemperatureData();
        if (tempData.communication_ok && tempData.current_temp >= TEMP_CRITICAL) {
            buzzer.setMode(BUZZER_TEMP_CRITICAL);
        } else if (tempData.communication_ok && tempData.current_temp >= TEMP_MAX_ALERT) {
            buzzer.setMode(BUZZER_TEMP_ALERT);
        } else {
            buzzer.setMode(BUZZER_OFF);
        }
    }
    buzzer.update();

    // Enviar telemetria (a cada 10 segundos)
    if (now - lastTelemetrySend >= TELEMETRY_SEND_MS && network.isConnected()) {
        lastTelemetrySend = now;
        
        TemperatureData tempData = modbusReader.getTemperatureData();
        InverterData invData = modbusReader.getInverterData();
        DoorState doorState = doorSensor.getState();

        TelemetryPayload payload = {
            .temperature = tempData.current_temp,
            .setpoint = tempData.setpoint,
            .heater_output = tempData.heater_output,
            .inverter_frequency = invData.frequency,
            .inverter_voltage = invData.output_voltage,
            .inverter_current = invData.output_current,
            .inverter_dc_voltage = invData.dc_bus_voltage,
            .inverter_temperature = invData.temperature,
            .inverter_running = invData.running,
            .inverter_fault_code = invData.fault_code,
            .door_open = doorState.is_open,
            .door_open_seconds = doorState.open_seconds,
            .door_stage = doorState.stage
        };

        bool success = apiClient.sendTelemetry(payload);
        #if DEBUG_SERIAL
        Serial.printf("[API] %s\n", success ? "✅ Enviado" : "❌ Falha");
        #endif
    }

    // Log de status (a cada 30 segundos)
    if (now - lastStatusLog >= 30000) {
        lastStatusLog = now;
        #if DEBUG_SERIAL
        TemperatureData tempData = modbusReader.getTemperatureData();
        InverterData invData = modbusReader.getInverterData();
        DoorState doorState = doorSensor.getState();
        Serial.printf("\n[STATUS] T:%.1f°C SV:%.1f°C | Inv:%.1fHz %.1fV %.2fA | Porta:%s %ds | %s\n",
            tempData.current_temp, tempData.setpoint,
            invData.frequency, invData.output_voltage, invData.output_current,
            doorState.is_open ? "ABERTA" : "FECHADA", doorState.open_seconds,
            network.isConnected() ? "ONLINE" : "OFFLINE");
        #endif
    }

    delay(10);
}
