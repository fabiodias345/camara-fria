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
    // EN1 é optoacoplada: ligada = GPIO LOW (ON); desligada = GPIO HIGH (OFF).
    pinMode(DOOR_SENSOR_PIN, INPUT);

    _last_raw_state = digitalRead(DOOR_SENSOR_PIN);
    _last_debounce = millis();
    setDoorState(isDoorOpen(_last_raw_state));

    #if DEBUG_SERIAL
    Serial.printf("[DOOR] HF-006Enet - EN1 (GPIO%d) inicializado\n", DOOR_SENSOR_PIN);
    Serial.printf("[DOOR] Estado inicial: %s\n", _state.is_open ? "ABERTA" : "FECHADA");
    #endif
}

void DoorSensor::update() {
    const bool raw_state = digitalRead(DOOR_SENSOR_PIN);
    const unsigned long now = millis();

    if (raw_state != _last_raw_state) {
        _last_raw_state = raw_state;
        _last_debounce = now;
    }

    if (now - _last_debounce >= DOOR_DEBOUNCE_MS) {
        setDoorState(isDoorOpen(raw_state));
    }

    if (_state.is_open) {
        _state.open_seconds = (now - _state.open_since) / 1000;
        updateStage();
    }
}

bool DoorSensor::isDoorOpen(bool raw_state) const {
    return DOOR_INPUT_ACTIVE_LOW ? !raw_state : raw_state;
}

void DoorSensor::setDoorState(bool is_open) {
    if (is_open == _state.is_open) return;

    if (is_open) {
        _state.is_open = true;
        _state.open_since = millis();
        _state.open_seconds = 0;
        _state.open_count++;
        _state.stage = DOOR_OPEN_NORMAL;
        #if DEBUG_SERIAL
        Serial.printf("[DOOR] Porta ABERTA (aberturas: %d)\n", _state.open_count);
        #endif
        return;
    }

    const unsigned long open_seconds = (millis() - _state.open_since) / 1000;
    _state.is_open = false;
    _state.close_since = millis();
    _state.open_seconds = 0;
    _state.stage = DOOR_CLOSED;
    _state.last_pulse = 0;
    #if DEBUG_SERIAL
    Serial.printf("[DOOR] Porta FECHADA (tempo aberta: %lu s)\n", open_seconds);
    #endif
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

    if (_state.stage != old_stage) {
        #if DEBUG_SERIAL
        const char* stageNames[] = {"FECHADA", "NORMAL", "AVISO", "PULSO", "CONTINUO"};
        Serial.printf("[DOOR] Estágio alterado: %s (%d segundos aberta)\n", stageNames[_state.stage], seconds);
        #endif
    }
}

bool DoorSensor::shouldBuzzerAlarm() const {
    return _state.is_open && _state.stage >= DOOR_OPEN_WARNING;
}

bool DoorSensor::shouldSendPulse() const {
    if (_state.stage != DOOR_OPEN_PULSE) return false;

    unsigned long now = millis();
    return now - _state.last_pulse >= 10000;
}

void DoorSensor::reset() {
    _state.is_open = false;
    _state.stage = DOOR_CLOSED;
    _state.open_since = 0;
    _state.open_seconds = 0;
    _state.last_pulse = 0;
}
