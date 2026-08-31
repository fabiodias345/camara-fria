// ============================================
// CÂMARA FRIA - Leitura Modbus RTU via RS485
// ============================================
#ifndef MODBUS_READER_H
#define MODBUS_READER_H

#include <ModbusMaster.h>
#include "config.h"

// Estrutura de dados do controlador de temperatura
struct TemperatureData {
    float current_temp;     // Temperatura atual (PV)
    float setpoint;         // Setpoint (SV)
    float heater_output;    // Saída do aquecedor (%)
    float alarm_high;       // Limite alarme alto
    float alarm_low;        // Limite alarme baixo
    bool  communication_ok; // Status da comunicação
    unsigned long last_read; // Timestamp da última leitura
};

// Estrutura de dados do inversor de frequência
struct InverterData {
    float frequency;        // Frequência de saída (Hz)
    float output_voltage;   // Tensão de saída (V)
    float output_current;   // Corrente de saída (A)
    float dc_bus_voltage;   // Tensão do barramento DC (V)
    float temperature;      // Temperatura do inversor (°C)
    uint16_t status_word;   // Word de status
    uint16_t fault_code;    // Código de falha
    bool  running;          // Em operação
    bool  communication_ok; // Status da comunicação
    unsigned long last_read;
};

// Registradores Modbus comuns
// Controlador (ex: RKC REX-C100 - ajuste conforme seu modelo)
#define REG_CTRL_PV_H       0x0000  // PV (Process Value) high word
#define REG_CTRL_PV_L       0x0001  // PV low word
#define REG_CTRL_SV_H       0x0002  // SV (Setpoint Value) high word
#define REG_CTRL_SV_L       0x0003  // SV low word
#define REG_CTRL_OUTPUT_H   0x0004  // Output %
#define REG_CTRL_OUTPUT_L   0x0005
#define REG_CTRL_ALARM_H    0x0008  // Alarm high
#define REG_CTRL_ALARM_L    0x0009
#define REG_CTRL_ALARML_H   0x000A  // Alarm low
#define REG_CTRL_ALARML_L   0x000B

// Inversor (ex: WEG CFW500 - ajuste conforme seu modelo)
#define REG_INV_FREQ_H      0x0000  // Frequência de saída
#define REG_INV_FREQ_L      0x0001
#define REG_INV_VOLT_H      0x0002  // Tensão de saída
#define REG_INV_VOLT_L      0x0003
#define REG_INV_CURR_H      0x0004  // Corrente de saída
#define REG_INV_CURR_L      0x0005
#define REG_INV_DC_H        0x0006  // Tensão DC bus
#define REG_INV_DC_L        0x0007
#define REG_INV_TEMP_H      0x0008  // Temperatura
#define REG_INV_TEMP_L      0x0009
#define REG_INV_STATUS      0x000A  // Status word
#define REG_INV_FAULT       0x000B  // Fault code

class ModbusReader {
public:
    ModbusReader();

    void begin();
    
    // Lê dados do controlador de temperatura
    bool readController(TemperatureData &data);
    
    // Lê dados do inversor de frequência
    bool readInverter(InverterData &data);
    
    // Lê todos os dispositivos
    void readAll();

    // Getters
    TemperatureData getTemperatureData() const { return _tempData; }
    InverterData getInverterData() const { return _invData; }

private:
    ModbusMaster _controllerNode;
    ModbusMaster _inverterNode;
    TemperatureData _tempData;
    InverterData _invData;

    void preTransmission();
    void postTransmission();
    
    // Helper para ler float de dois registros
    float readFloat(ModbusMaster &node, uint16_t reg);
    uint32_t readUint32(ModbusMaster &node, uint16_t reg);
};

// Callbacks estáticos para direção RS485
static void _preTx() {
    digitalWrite(RS485_DE_RE_PIN, HIGH);  // Habilita transmissão
}

static void _postTx() {
    digitalWrite(RS485_DE_RE_PIN, LOW);   // Habilita recepção
}

#endif // MODBUS_READER_H
