// ============================================
// CÂMARA FRIA - Implementação Sensor de Porta
// HF-006Enet: EN7 (GPIO27) - Entrada optoacoplada
// ============================================
#include "door_sensor.h"

DoorSensor::DoorSensor() : _state{}, _last_raw_state(false), _last_debounce(0) {
    _state.is_open = false;
    _state.stage = DOOR_CLOSED;
    _state.open_since = 0;
    _state.close_since = 0;
    _state.last_pulse = 0;
    _state.open_seconds = 0;
    _state.open_count = 0;
}

void DoorSensor::begin() {
    // HF-006Enet: EN7 (GPIO27) com optoacoplador PC817
    // Entrada optoacoplada: HIGH = sensor ativo (porta aberta)
    // Reed switch: normalmente fechado quando porta está fechada
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);  // PULLUP interno
    
    _last_raw_state = digitalRead(DOOR_SENSOR_PIN);
    _state.is_open = _last_raw_state;  // HIGH = porta aberta (reed switch acionado)
    
    #if DEBUG_SERIAL
    Serial.printf("[DOOR] HF-006Enet - EN7 (GPIO%d) inicializado\n", DOOR_SENSOR_PIN);
    Serial.printf("[DOOR] Estado inicial: %s\n", _state.is_open ? "ABERTA" : "FECHADA");
    #endif
}

void DoorSensor::update() {
    // Leitura com debounce
    bool raw_state = digitalRead(DOOR_SENSOR_PIN);
    
    if (raw_state != _last_raw_state) {
        if (millis() - _last_debounce > DOOR_DEBOUNCE_MS) {
            _last_debounce = millis();
            _last_raw_state = raw_state;
            
            bool new_is_open = raw_state;  // HIGH = porta aberta
            
            // Transição: Fechada → Aberta
            if (new_is_open && !_state.is_open) {
                _state.is_open = true;
                _state.open_since = millis();
                _state.open_count++;
                _state.stage = DOOR_OPEN_NORMAL;
                
                #if DEBUG_SERIAL
                Serial.printf("[DOOR] Porta ABERTA (aberturas: %d)\n", _state.open_count);
                #endif
            }
            // Transição: Aberta → Fechada
            else if (!new_is_open && _state.is_open) {
                _state.is_open = false;
                _state.close_since = millis();
                _state.stage = DOOR_CLOSED;
                _state.last_pulse = 0;
                
                #if DEBUG_SERIAL
                Serial.printf("[DOOR] Porta FECHADA (tempo aberta: %lu s)\n",
                             (millis() - _state.open_since) / 1000);
                #endif
            }
        }
    }
    
    // Atualiza estágio do alarme se porta estiver aberta
    if (_state.is_open) {
        _state.open_seconds = (millis() - _state.open_since) / 1000;
        updateStage();
    }
}

void DoorSensor::updateStage() {
    uint32_t seconds = _state.open_seconds;
    DoorAlarmStage old_stage = _state.stage;

    if (seconds >= DOOR_OPEN_CONT_S) {
        _state.stage = DOOR_OPEN_CONTINUOUS;
    } else if (seconds >= DOOR_OPEN_PULSE_S) {
        _state.stage = DOOR_OPEN_PULSE;
    } else if (seconds >= DOOR_OPEN_WARN_S) {
        _state.stage = DOOR_OPEN_WARNING;
    } else {
        _state.stage = DOOR_OPEN_NORMAL;
    }
    
    // Log quando muda de estágio
    if (_state.stage != old_stage) {
        #if DEBUG_SERIAL
        const char* stageNames[] = {"FECHADA", "NORMAL", "AVISO", "PULSO", "CONTINUO"};
        Serial.printf("[DOOR] Estágio alterado: %s (%d segundos aberta)\n",
                     stageNames[_state.stage], seconds);
        #endif
    }
}

bool DoorSensor::shouldBuzzerAlarm() const {
    // Buzzer deve tocar no estágio WARNING (pulso), PULSE e CONTINUOUS
    return _state.is_open && _state.stage >= DOOR_OPEN_WARNING;
}

bool DoorSensor::shouldSendPulse() const {
    // No estágio PULSE, envia pulso a cada 10 segundos
    if (_state.stage != DOOR_OPEN_PULSE) return false;
    
    unsigned long now = millis();
    if (now - _state.last_pulse >= 10000) {  // 10 segundos
        return true;
    }
    return false;
}

void DoorSensor::reset() {
    _state.is_open = false;
    _state.stage = DOOR_CLOSED;
    _state.open_since = 0;
    _state.open_seconds = 0;
    _state.last_pulse = 0;
}
