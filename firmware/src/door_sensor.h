// ============================================
// CÂMARA FRIA - Sensor de Porta
// ============================================
#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <Arduino.h>
#include "config.h"

enum DoorAlarmStage {
    DOOR_CLOSED = 0,        // Porta fechada
    DOOR_OPEN_NORMAL,       // Aberta < 5 min (sem alarme)
    DOOR_OPEN_WARNING,      // Aberta 5 min (1 pulso)
    DOOR_OPEN_PULSE,        // Aberta 6 min (pulsos a cada 10s)
    DOOR_OPEN_CONTINUOUS    // Aberta > 7 min (contínuo)
};

struct DoorState {
    bool is_open;               // Estado atual da porta
    DoorAlarmStage stage;       // Estágio do alarme
    unsigned long open_since;   // Timestamp quando abriu
    unsigned long close_since;  // Timestamp quando fechou
    unsigned long last_pulse;   // Timestamp do último pulso
    uint32_t open_seconds;      // Tempo total aberta em segundos
    uint32_t open_count;        // Contagem de aberturas
};

class DoorSensor {
public:
    DoorSensor();

    void begin();
    
    // Deve ser chamada a cada loop() - atualiza estado
    void update();
    
    // Reseta timers quando a porta fecha
    void reset();
    
    // Getters
    bool isOpen() const { return _state.is_open; }
    DoorAlarmStage getStage() const { return _state.stage; }
    DoorState getState() const { return _state; }
    bool shouldBuzzerAlarm() const;   // Deve ativar buzzer
    bool shouldSendPulse() const;     // Deve enviar pulso (10s intervalo)

private:
    DoorState _state;
    bool _last_raw_state;
    unsigned long _last_debounce;
    
    void updateStage();
};

#endif // DOOR_SENSOR_H
