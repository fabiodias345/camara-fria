// ============================================
// CÂMARA FRIA - Implementação Modbus RTU
// HF-006Enet: RS485 via UART0 + MAX485
// ============================================
#include "modbus_reader.h"
#include <HardwareSerial.h>

// UART0 no ESP32: TX=GPIO1, RX=GPIO3
// HF-006Enet: MAX485 conectado à UART0
HardwareSerial RS485Serial(0);

ModbusReader::ModbusReader() : _tempData{}, _invData{} {
    _tempData.communication_ok = false;
    _invData.communication_ok = false;
}

void ModbusReader::begin() {
    // Configura pin de direção RS485 (DE+RE do MAX485)
    pinMode(RS485_DE_RE_PIN, OUTPUT);
    digitalWrite(RS485_DE_RE_PIN, LOW);  // Começa em modo recepção

    // IMPORTANTE: Na HF-006Enet, RS485 e Serial TTL compartilham UART0
    // Se precisar de Serial para debug, desconecte o cabo RS485 primeiro!
    RS485Serial.begin(RS485_BAUD_RATE, RS485_SERIAL_CONFIG, RS485_RX_PIN, RS485_TX_PIN);

    // Configura nó do controlador (RKC REX-C100)
    _controllerNode.begin(CONTROLLER_SLAVE_ID, RS485Serial);
    _controllerNode.preTransmission([]() { 
        digitalWrite(RS485_DE_RE_PIN, HIGH);  // Habilita transmissão
    });
    _controllerNode.postTransmission([]() { 
        digitalWrite(RS485_DE_RE_PIN, LOW);   // Habilita recepção
    });

    // Configura nó do inversor (WEG CFW500)
    _inverterNode.begin(INVERTER_SLAVE_ID, RS485Serial);
    _inverterNode.preTransmission([]() { 
        digitalWrite(RS485_DE_RE_PIN, HIGH); 
    });
    _inverterNode.postTransmission([]() { 
        digitalWrite(RS485_DE_RE_PIN, LOW); 
    });

    #if DEBUG_SERIAL
    Serial.println("[MODBUS] HF-006Enet - UART0 + MAX485 inicializado");
    Serial.printf("[MODBUS] Baud: %d, Ctrl ID: %d, Inv ID: %d\n",
                  RS485_BAUD_RATE, CONTROLLER_SLAVE_ID, INVERTER_SLAVE_ID);
    Serial.printf("[MODBUS] GPIOs: TX=%d, RX=%d, DE/RE=%d\n",
                  RS485_TX_PIN, RS485_RX_PIN, RS485_DE_RE_PIN);
    #endif
}

float ModbusReader::readFloat(ModbusMaster &node, uint16_t reg) {
    uint16_t result = node.readHoldingRegisters(reg, 2);
    if (result == node.ku8MBSuccess) {
        uint16_t high = node.getResponseBuffer(0);
        uint16_t low = node.getResponseBuffer(1);
        uint32_t raw = ((uint32_t)high << 16) | (uint32_t)low;
        float value;
        memcpy(&value, &raw, sizeof(float));
        return value;
    }
    return 0.0f;
}

uint32_t ModbusReader::readUint32(ModbusMaster &node, uint16_t reg) {
    uint16_t result = node.readHoldingRegisters(reg, 2);
    if (result == node.ku8MBSuccess) {
        uint16_t high = node.getResponseBuffer(0);
        uint16_t low = node.getResponseBuffer(1);
        return ((uint32_t)high << 16) | (uint32_t)low;
    }
    return 0;
}

bool ModbusReader::readController(TemperatureData &data) {
    // Lê PV (Process Value / Temperatura Atual)
    uint16_t result = _controllerNode.readHoldingRegisters(REG_CTRL_PV_H, 2);
    if (result == _controllerNode.ku8MBSuccess) {
        int16_t rawPV = (int16_t)_controllerNode.getResponseBuffer(0);
        data.current_temp = rawPV / 10.0f;  // Divide por 10 (formato xx.x)
    } else {
        data.communication_ok = false;
        return false;
    }

    delay(50);  // Intervalo entre leituras

    // Lê SV (Setpoint)
    result = _controllerNode.readHoldingRegisters(REG_CTRL_SV_H, 2);
    if (result == _controllerNode.ku8MBSuccess) {
        int16_t rawSV = (int16_t)_controllerNode.getResponseBuffer(0);
        data.setpoint = rawSV / 10.0f;
    }
    delay(50);

    // Lê Output %
    result = _controllerNode.readHoldingRegisters(REG_CTRL_OUTPUT_H, 2);
    if (result == _controllerNode.ku8MBSuccess) {
        int16_t rawOut = (int16_t)_controllerNode.getResponseBuffer(0);
        data.heater_output = rawOut / 10.0f;
    }
    delay(50);

    // Lê Alarm High
    result = _controllerNode.readHoldingRegisters(REG_CTRL_ALARM_H, 2);
    if (result == _controllerNode.ku8MBSuccess) {
        int16_t rawAlarm = (int16_t)_controllerNode.getResponseBuffer(0);
        data.alarm_high = rawAlarm / 10.0f;
    }
    delay(50);

    // Lê Alarm Low
    result = _controllerNode.readHoldingRegisters(REG_CTRL_ALARML_H, 2);
    if (result == _controllerNode.ku8MBSuccess) {
        int16_t rawAlarmL = (int16_t)_controllerNode.getResponseBuffer(0);
        data.alarm_low = rawAlarmL / 10.0f;
    }

    data.communication_ok = true;
    data.last_read = millis();

    #if DEBUG_SERIAL
    Serial.printf("[CTRL] PV: %.1f°C | SV: %.1f°C | Out: %.1f%% | AlarmH: %.1f | AlarmL: %.1f\n",
                  data.current_temp, data.setpoint, data.heater_output,
                  data.alarm_high, data.alarm_low);
    #endif

    return true;
}

bool ModbusReader::readInverter(InverterData &data) {
    // Lê frequência de saída
    uint16_t result = _inverterNode.readHoldingRegisters(REG_INV_FREQ_H, 2);
    if (result == _inverterNode.ku8MBSuccess) {
        uint16_t rawFreq = _inverterNode.getResponseBuffer(0);
        data.frequency = rawFreq / 100.0f;
    } else {
        data.communication_ok = false;
        return false;
    }
    delay(50);

    // Lê tensão de saída
    result = _inverterNode.readHoldingRegisters(REG_INV_VOLT_H, 2);
    if (result == _inverterNode.ku8MBSuccess) {
        uint16_t rawVolt = _inverterNode.getResponseBuffer(0);
        data.output_voltage = rawVolt / 10.0f;
    }
    delay(50);

    // Lê corrente de saída
    result = _inverterNode.readHoldingRegisters(REG_INV_CURR_H, 2);
    if (result == _inverterNode.ku8MBSuccess) {
        uint16_t rawCurr = _inverterNode.getResponseBuffer(0);
        data.output_current = rawCurr / 100.0f;
    }
    delay(50);

    // Lê tensão DC
    result = _inverterNode.readHoldingRegisters(REG_INV_DC_H, 2);
    if (result == _inverterNode.ku8MBSuccess) {
        uint16_t rawDC = _inverterNode.getResponseBuffer(0);
        data.dc_bus_voltage = rawDC / 10.0f;
    }
    delay(50);

    // Lê temperatura do inversor
    result = _inverterNode.readHoldingRegisters(REG_INV_TEMP_H, 2);
    if (result == _inverterNode.ku8MBSuccess) {
        int16_t rawTemp = (int16_t)_inverterNode.getResponseBuffer(0);
        data.temperature = rawTemp / 10.0f;
    }
    delay(50);

    // Lê status word
    result = _inverterNode.readHoldingRegisters(REG_INV_STATUS, 1);
    if (result == _inverterNode.ku8MBSuccess) {
        data.status_word = _inverterNode.getResponseBuffer(0);
        data.running = (data.status_word & 0x0001) != 0;
    }
    delay(50);

    // Lê código de falha
    result = _inverterNode.readHoldingRegisters(REG_INV_FAULT, 1);
    if (result == _inverterNode.ku8MBSuccess) {
        data.fault_code = _inverterNode.getResponseBuffer(0);
    }

    data.communication_ok = true;
    data.last_read = millis();

    #if DEBUG_SERIAL
    Serial.printf("[INV] Freq: %.2f Hz | V: %.1fV | I: %.2fA | DC: %.1fV | T: %.1f°C | Status: 0x%04X | Fault: %d\n",
                  data.frequency, data.output_voltage, data.output_current,
                  data.dc_bus_voltage, data.temperature, data.status_word, data.fault_code);
    #endif

    return true;
}

void ModbusReader::readAll() {
    readController(_tempData);
    delay(100);  // Intervalo entre dispositivos
    readInverter(_invData);
}
