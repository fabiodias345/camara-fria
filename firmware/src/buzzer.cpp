// ============================================
// CÂMARA FRIA - Implementação Buzzer
// HF-006Enet: EN5 (GPIO25) - Saída via optoacoplador
// ============================================
#include "buzzer.h"

Buzzer::Buzzer() : _mode(BUZZER_OFF), _last_pulse(0), _pulse_start(0),
                    _pulse_active(false), _buzzer_on(false) {}

void Buzzer::begin() {
    // HF-006Enet: EN5 (GPIO25) com optoacoplador
    // Configura canal PWM do LEDC para o buzzer
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ_HZ, 8);  // 8-bit resolution
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);  // Começa desligado
    
    #if DEBUG_SERIAL
    Serial.printf("[BUZZER] HF-006Enet - EN5 (GPIO%d) inicializado\n", BUZZER_PIN);
    #endif
}

void Buzzer::setMode(BuzzerMode mode) {
    if (_mode != mode) {
        _mode = mode;
        _last_pulse = millis();
        
        #if DEBUG_SERIAL
        const char* modeNames[] = {
            "OFF", "PULSE_WARN", "PULSE_INTERVAL", "CONTINUOUS",
            "TEMP_ALERT", "TEMP_CRITICAL"
        };
        Serial.printf("[BUZZER] Modo: %s\n", modeNames[mode]);
        #endif
    }
}

void Buzzer::update() {
    unsigned long now = millis();
    
    switch (_mode) {
        case BUZZER_OFF:
            setBuzzer(false);
            break;
            
        case BUZZER_PULSE_WARNING:
            // Um único pulso de 200ms quando detectado (5 min porta aberta)
            if (!_pulse_active) {
                setBuzzer(true);
                _pulse_start = now;
                _pulse_active = true;
            } else if (now - _pulse_start >= BUZZER_ALERT_PULSE_MS) {
                setBuzzer(false);
                _pulse_active = false;
                // Fica off até ser chamado novamente
            }
            break;
            
        case BUZZER_PULSE_INTERVAL:
            // Pulsos a cada 10 segundos (6 min porta aberta)
            if (now - _last_pulse >= 10000) {
                setBuzzer(true);
                _pulse_start = now;
                _last_pulse = now;
                _pulse_active = true;
            }
            if (_pulse_active && (now - _pulse_start >= BUZZER_ALERT_PULSE_MS)) {
                setBuzzer(false);
                _pulse_active = false;
            }
            break;
            
        case BUZZER_CONTINUOUS:
        case BUZZER_TEMP_CRITICAL:
            // Sempre ligado (7 min porta aberta ou temp crítica)
            setBuzzer(true);
            break;
            
        case BUZZER_TEMP_ALERT:
            // Pulsos alternados: 500ms on, 500ms off (alerta de temperatura)
            if (now - _last_pulse >= 500) {
                _buzzer_on = !_buzzer_on;
                setBuzzer(_buzzer_on);
                _last_pulse = now;
            }
            break;
    }
}

void Buzzer::triggerPulse() {
    if (_mode == BUZZER_OFF) {
        setMode(BUZZER_PULSE_WARNING);
    }
}

void Buzzer::stop() {
    setMode(BUZZER_OFF);
    setBuzzer(false);
}

void Buzzer::setBuzzer(bool on) {
    if (on) {
        ledcWriteTone(BUZZER_CHANNEL, BUZZER_FREQ_HZ);
    } else {
        ledcWriteTone(BUZZER_CHANNEL, 0);
    }
    _buzzer_on = on;
}
