// ============================================
// CÂMARA FRIA - Controle do Buzzer
// ============================================
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "config.h"

enum BuzzerMode {
    BUZZER_OFF = 0,
    BUZZER_PULSE_WARNING,   // Pulso único (5 min porta)
    BUZZER_PULSE_INTERVAL,  // Pulsos a cada 10s (6 min porta)
    BUZZER_CONTINUOUS,      // Contínuo (7 min porta ou temp crítica)
    BUZZER_TEMP_ALERT,      // Alerta de temperatura
    BUZZER_TEMP_CRITICAL    // Crítico de temperatura
};

class Buzzer {
public:
    Buzzer();

    void begin();
    
    // Define o modo do buzzer
    void setMode(BuzzerMode mode);
    
    // Deve ser chamada a cada loop()
    void update();
    
    // Ativa um pulso curto de alerta
    void triggerPulse();
    
    // Para o buzzer completamente
    void stop();
    
    // Getters
    BuzzerMode getMode() const { return _mode; }
    bool isActive() const { return _mode != BUZZER_OFF; }

private:
    BuzzerMode _mode;
    unsigned long _last_pulse;
    unsigned long _pulse_start;
    bool _pulse_active;
    bool _buzzer_on;
    
    void setBuzzer(bool on);
};

#endif // BUZZER_H
