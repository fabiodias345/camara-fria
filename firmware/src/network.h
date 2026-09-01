// ============================================
// CÂMARA FRIA - Conexão de Rede (W5500)
// HF-006Enet: W5500 via HSPI
// ============================================
#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include "config.h"

class NetworkManager {
public:
    NetworkManager();

    void begin();
    
    // Verifica se está conectado
    bool isConnected();
    
    // Reconecta se necessário
    void maintain();
    
    // Getters
    String getLocalIP() const;

private:
    byte _mac[6];
    bool _ethernet_initialized;
    unsigned long _last_wifi_attempt;
};

#endif // NETWORK_H
