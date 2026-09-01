// ============================================
// CÂMARA FRIA - Configurações HF-006Enet
// ============================================
#ifndef CONFIG_H
#define CONFIG_H

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#else
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

// =============================================
// PLACA: HF-006Enet (Hoffer Automação)
// ESP32 Dual-Core + W5500 Ethernet (HSPI)
// + RS485 (UART0) + 8 EN + 6 Relés
// =============================================

// --- Rede Ethernet W5500 (via HSPI) ---
// IMPORTANTE: W5500 usa HSPI, preservando VSPI para expansão
#define ETH_SPI_CS_PIN      15      // GPIO15 - W5500 CS
#define ETH_SPI_SCK_PIN     14      // GPIO14 - HSPI SCK (W5500)
#define ETH_SPI_MISO_PIN    12      // GPIO12 - HSPI MISO (W5500)
#define ETH_SPI_MOSI_PIN    13      // GPIO13 - HSPI MOSI (W5500)
#define ETH_INTERRUPT_PIN   2       // GPIO2  - W5500 INT
#define ETH_RESET_PIN       0       // GPIO0  - W5500 RESET

// IP Estático do ESP32 na rede local
#define ETH_IP_ADDR         {192, 168, 1, 100}
#define ETH_GATEWAY         {192, 168, 1, 1}
#define ETH_SUBNET          {255, 255, 255, 0}
#define ETH_DNS1            {8, 8, 8, 8}
#define ETH_DNS2            {8, 8, 4, 4}

// --- RS485 / Modbus RTU (UART0 + MAX485) ---
// IMPORTANTE: UART0 compartilhada entre RS485 e Serial TTL
// Não usar Serial durante operação normal!
#define RS485_RX_PIN        3       // GPIO3  - UART0 RX (RO do MAX485)
#define RS485_TX_PIN        1       // GPIO1  - UART0 TX (DI do MAX485)
#define RS485_DE_RE_PIN     5       // GPIO5  - DE+RE do MAX485 (direção Half-Duplex)
#define RS485_BAUD_RATE     9600    // Baud rate padrão dos dispositivos
#define RS485_SERIAL_CONFIG SERIAL_8N1

// Slave IDs dos dispositivos Modbus
#define CONTROLLER_SLAVE_ID     1   // Controlador de Temperatura (RKC REX-C100)
#define INVERTER_SLAVE_ID       2   // Inversor de Frequência (WEG CFW500)

// --- Entradas Digitais (optoacopladas 12/24V) ---
#define DEVICE_ID           "camara-fria-01"
#define DOOR_SENSOR_PIN     36      // EN1 (GPIO36) - Sensor da porta
#define DOOR_INPUT_ACTIVE_LOW true   // EN1 ligada = ON/porta aberta; desligada = OFF/fechada
#define DOOR_DEBOUNCE_MS    500     // Debounce 500ms
#define DOOR_OPEN_WARN_S    300     // 5 minutos = aviso (1 pulso)
#define DOOR_OPEN_PULSE_S   360     // 6 minutos = pulsos a cada 10s
#define DOOR_OPEN_CONT_S    420     // 7 minutos = contínuo até fechar

// Outras entradas disponíveis:
// EN1→GPIO36, EN2→GPIO39, EN3→GPIO32, EN4→GPIO33
// EN5→GPIO25, EN6→GPIO26
// EN8→GPIO14 (compartilhado com HSPI SCK - NÃO USAR)

// --- Saída Buzzer (via EN5 - PWM) ---
#define BUZZER_PIN          25      // EN5 (GPIO25) - Buzzer ativo via optoacoplador
#define BUZZER_CHANNEL      0       // Canal PWM do LEDC
#define BUZZER_FREQ_HZ      2000    // Frequência 2kHz
#define BUZZER_DUTY_CYCLE   128     // 50% duty cycle
#define BUZZER_ALERT_PULSE_MS  200  // Duração do pulso de alerta

// --- Saídas Relé disponíveis ---
// R1→GPIO4, R2→GPIO16, R3→GPIO17
// R4→GPIO18, R5→GPIO19, R6→GPIO23
// Capacidade: 10A resistiva / 2A indutiva

// --- Temperatura ---
#define TEMP_MAX_ALERT      8.0f    // Alerta WhatsApp (acima de -8°C para câmara fria)
#define TEMP_CRITICAL       12.0f   // Crítico (buzzer + WhatsApp)
#define TEMP_POLL_INTERVAL_MS 5000  // Lê a cada 5 segundos

// --- Supabase Cloud (Edge Functions) ---
#define SUPABASE_HOST       "hnfdlpjkoxizrgukjcrw.supabase.co"
#define SUPABASE_PORT       443
#define SUPABASE_API_PATH   "/functions/v1/receive-telemetry"
#define SUPABASE_ANON_KEY   "sb_publishable_DReGLMwglDMCB5DYoPcQwQ_3_gVWFui"
#define SUPABASE_SSL        true

// --- Intervalos ---
#define TELEMETRY_SEND_MS   10000   // Envia dados a cada 10 segundos
#define WATCHDOG_TIMEOUT_MS 30000   // Watchdog 30 segundos

// --- Debug ---
// NOTA: Serial compartilhada com RS485!
// Usar apenas durante desenvolvimento (com RS485 desconectado)
#define DEBUG_SERIAL        true    // Habilitar logs via Serial
#define DEBUG_BAUD_RATE     115200

#endif // CONFIG_H
