// ============================================
// CÂMARA FRIA - Implementação Rede Ethernet
// HF-006Enet: W5500 via HSPI
// ============================================
#include "network.h"

// Ethernet_Generic must only be included in .cpp files
#include <SPI.h>
#include <Ethernet_Generic.h>

NetworkManager::NetworkManager() : _initialized(false) {
    // MAC address pseudo-aleatório baseado no chip ID do ESP32
    uint64_t mac = ESP.getEfuseMac();
    _mac[0] = 0x02;  // Local admin bit set
    _mac[1] = (mac >> 40) & 0xFF;
    _mac[2] = (mac >> 32) & 0xFF;
    _mac[3] = (mac >> 24) & 0xFF;
    _mac[4] = (mac >> 16) & 0xFF;
    _mac[5] = (mac >> 8) & 0xFF;
}

void NetworkManager::begin() {
    #if DEBUG_SERIAL
    Serial.println("[NET] HF-006Enet - Inicializando W5500 via HSPI...");
    Serial.printf("[NET] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
    Serial.printf("[NET] GPIOs HSPI: MOSI=%d, MISO=%d, SCK=%d, CS=%d\n",
                  ETH_SPI_MOSI_PIN, ETH_SPI_MISO_PIN, ETH_SPI_SCK_PIN, ETH_SPI_CS_PIN);
    #endif

    // IMPORTANTE: HF-006Enet usa HSPI para W5500
    // Isso preserva VSPI para futuras expansões (SD card, displays, etc.)
    SPI.begin(ETH_SPI_SCK_PIN, ETH_SPI_MISO_PIN, ETH_SPI_MOSI_PIN, ETH_SPI_CS_PIN);
    
    // Configura IP estático
    IPAddress ip(ETH_IP_ADDR);
    IPAddress gateway(ETH_GATEWAY);
    IPAddress subnet(ETH_SUBNET);
    IPAddress dns1(ETH_DNS1);
    
    Ethernet.begin(_mac, ip, dns1, gateway, subnet);

    // Aguarda link físico
    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
        #if DEBUG_SERIAL
        Serial.println("[NET] W5500 detectado via HSPI");
        #endif
        
        // Aguarda link
        unsigned long start = millis();
        while (Ethernet.linkStatus() == LinkOFF && millis() - start < 5000) {
            delay(100);
        }
        
        if (Ethernet.linkStatus() == LinkON) {
            _initialized = true;
            #if DEBUG_SERIAL
            Serial.printf("[NET] Conectado! IP: %s\n", Ethernet.localIP().toString().c_str());
            #endif
        } else {
            #if DEBUG_SERIAL
            Serial.println("[NET] AVISO: Link não detectado, tentando mesmo assim...");
            #endif
            _initialized = true;
        }
    } else {
        #if DEBUG_SERIAL
        Serial.println("[NET] ERRO: W5500 não detectado via HSPI!");
        #endif
    }
}

bool NetworkManager::isConnected() {
    if (!_initialized) return false;
    return Ethernet.linkStatus() == LinkON && Ethernet.localIP() != IPAddress(0,0,0,0);
}

void NetworkManager::maintain() {
    if (!_initialized) return;
    
    Ethernet.maintain();
    
    // Verifica se o IP ainda é válido
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
        #if DEBUG_SERIAL
        Serial.println("[NET] IP perdido, reinicializando W5500...");
        #endif
        Ethernet.init(ETH_SPI_CS_PIN);
        begin();
    }
}

String NetworkManager::getLocalIP() const {
    if (!_initialized) return String("0.0.0.0");
    return Ethernet.localIP().toString();
}
