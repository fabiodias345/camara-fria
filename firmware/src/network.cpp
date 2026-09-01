// ============================================
// CÂMARA FRIA - Conexão de Rede (W5500 + Wi-Fi)
// HF-006Enet: W5500 via HSPI
// ============================================
#include "network.h"

#include <SPI.h>
#include <Ethernet_Generic.h>
#include <WiFi.h>

NetworkManager::NetworkManager() : _ethernet_initialized(false), _last_wifi_attempt(0) {
    uint64_t mac = ESP.getEfuseMac();
    _mac[0] = 0x02;
    _mac[1] = (mac >> 40) & 0xFF;
    _mac[2] = (mac >> 32) & 0xFF;
    _mac[3] = (mac >> 24) & 0xFF;
    _mac[4] = (mac >> 16) & 0xFF;
    _mac[5] = (mac >> 8) & 0xFF;
}

void NetworkManager::begin() {
    SPI.begin(ETH_SPI_SCK_PIN, ETH_SPI_MISO_PIN, ETH_SPI_MOSI_PIN, ETH_SPI_CS_PIN);
    IPAddress ip(ETH_IP_ADDR);
    IPAddress gateway(ETH_GATEWAY);
    IPAddress subnet(ETH_SUBNET);
    IPAddress dns1(ETH_DNS1);
    Ethernet.begin(_mac, ip, dns1, gateway, subnet);

    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
        unsigned long start = millis();
        while (Ethernet.linkStatus() == LinkOFF && millis() - start < 5000) delay(100);
        _ethernet_initialized = true;
    }

    if (WIFI_SSID[0] == '\0') return;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    _last_wifi_attempt = millis();

    const unsigned long timeout = millis() + 10000;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) delay(250);
}

bool NetworkManager::isConnected() {
    const bool ethernet_connected = _ethernet_initialized &&
        Ethernet.linkStatus() == LinkON && Ethernet.localIP() != IPAddress(0, 0, 0, 0);
    return ethernet_connected || WiFi.status() == WL_CONNECTED;
}

void NetworkManager::maintain() {
    if (_ethernet_initialized) {
        Ethernet.maintain();
        if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
            Ethernet.init(ETH_SPI_CS_PIN);
            begin();
        }
    }
    if (WIFI_SSID[0] != '\0' && WiFi.status() != WL_CONNECTED &&
        millis() - _last_wifi_attempt >= 30000) {
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        _last_wifi_attempt = millis();
    }
}

String NetworkManager::getLocalIP() const {
    if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
    if (_ethernet_initialized) return Ethernet.localIP().toString();
    return String("0.0.0.0");
}