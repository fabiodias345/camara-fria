# 🧊 Câmara Fria - Sistema de Monitoramento Inteligente

Sistema completo de monitoramento de câmara fria com leitura via Modbus RTU (RS485),
dashboard web em tempo real, alarmes via buzzer e alertas automáticos via WhatsApp.

## 🏗️ Arquitetura

```
┌─────────────────┐    RS485/Modbus    ┌──────────────┐
│  Controlador     │◄─────────────────►│              │
│  de Temperatura  │    (Slave ID: 1)  │    ESP32     │
│  (RKC REX-C100)  │                   │  + Ethernet  │
└─────────────────┘                    │  + RS485     │
                                       │  + Buzzer    │
┌─────────────────┐    RS485/Modbus    │  + Sensor    │
│  Inversor de     │◄─────────────────►│    Porta     │
│  Frequência      │    (Slave ID: 2)  │              │
│  (WEG CFW500)    │                   └──────┬───────┘
└─────────────────┘                          │
                                             │ HTTP/WebSocket
                                             ▼
┌─────────────────────────────────────────────────────────┐
│                    Supabase (Docker)                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │PostgreSQL │  │ Realtime  │  │  Edge Functions      │  │
│  │  (dados)  │  │ (WS push) │  │  - Receber dados     │  │
│  └──────────┘  └──────────┘  │  - Alertas AI         │  │
│                               │  - WhatsApp sender    │  │
│                               └──────────────────────┘  │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│              Dashboard (Next.js + Tailwind)              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │
│  │Temperatura│ │Inversor  │ │Status    │ │Histórico  │  │
│  │ Atual     │ │Frequência│ │ Porta    │ │ & Gráficos│  │
│  └──────────┘ └──────────┘ └──────────┘ └───────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│              Evolution API (WhatsApp)                     │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Alertas automáticos via WhatsApp quando:         │   │
│  │  • Temperatura > MAX_ALERT                        │   │
│  │  • Temperatura > CRITICAL                         │   │
│  │  • Porta aberta > 5 min                           │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## 📁 Estrutura do Projeto

```
camara-fria/
├── docker-compose.yml          # Stack completa: Supabase + Evolution + Dashboard
├── .env                        # Variáveis de ambiente
├── README.md                   # Este arquivo
│
├── firmware/                   # Código do ESP32
│   ├── platformio.ini          # Config PlatformIO
│   ├── src/
│   │   ├── main.cpp            # Loop principal
│   │   ├── config.h            # Configurações de rede e pinos
│   │   ├── modbus_reader.h/cpp # Leitura Modbus RTU
│   │   ├── network.h/cpp       # Conexão Ethernet
│   │   ├── door_sensor.h/cpp   # Lógica da porta
│   │   ├── buzzer.h/cpp        # Controle do buzzer
│   │   └── api_client.h/cpp    # Envio HTTP ao Supabase
│   └── lib/
│       └── readme.md
│
├── supabase/                   # Configuração Supabase
│   ├── migrations/
│   │   └── 001_initial.sql     # Schema do banco
│   └── functions/
│       ├── receive-telemetry/   # Edge Function: recebe dados do ESP32
│       │   └── index.ts
│       ├── alert-checker/       # Edge Function: verifica alertas e envia WhatsApp
│       │   └── index.ts
│       └── send-whatsapp/       # Edge Function: envia mensagem via Evolution API
│           └── index.ts
│
└── dashboard/                  # Frontend web
    ├── package.json
    ├── next.config.js
    ├── tailwind.config.js
    ├── tsconfig.json
    ├── postcss.config.js
    ├── public/
    ├── src/
    │   ├── app/
    │   │   ├── layout.tsx
    │   │   ├── page.tsx         # Página principal (redirect)
    │   │   ├── globals.css
    │   │   └── dashboard/
    │   │       └── page.tsx     # Dashboard principal
    │   ├── components/
    │   │   ├── TemperatureCard.tsx
    │   │   ├── InverterCard.tsx
    │   │   ├── DoorStatus.tsx
    │   │   ├── AlertHistory.tsx
    │   │   ├── SystemStatus.tsx
    │   │   └── Header.tsx
    │   ├── lib/
    │   │   └── supabase.ts      # Cliente Supabase
    │   └── types/
    │       └── index.ts
    └── Dockerfile
```

## 🚀 Setup Rápido

### Pré-requisitos
- Docker Desktop (Windows/Mac) ou Docker Engine (Linux)
- PlatformIO CLI (para programar o ESP32)
- Git

### 1. Subir o ambiente completo
```bash
# Clonar e configurar
cd camara-fria
cp .env.example .env

# Subir tudo
docker compose up -d

# Verificar status
docker compose ps
```

### 2. Acessar os serviços
| Serviço | URL |
|---------|-----|
| **Dashboard** | http://localhost:3000 |
| **Supabase Studio** | http://localhost:8000 |
| **Evolution API** | http://localhost:8080 |

### 3. Programar o ESP32
```bash
cd firmware
# Configurar WiFi e IP estático em src/config.h
pio run --target upload
```

### 4. Configurar WhatsApp
1. Acesse Evolution API em http://localhost:8080
2. Crie uma instância "cold-room-alerts"
3. Escaneie o QR Code com seu WhatsApp
4. Configure o número de notificação em `.env`

## 🔧 Hardware

### Componentes
- **ESP32** com módulo Ethernet (W5500 ou similar)
- **Módulo RS485** (MAX485/MAX3485) - conversor TTL↔RS485
- **Controlador de Temperatura** - ex: RKC REX-C100 (Modbus RTU Slave ID: 1)
- **Inversor de Frequência** - ex: WEG CFW500 (Modbus RTU Slave ID: 2)
- **Buzzer** ativo 5V - alarme sonoro
- **Sensor magnético de porta** - reed switch Normally Open
- **Alimentação** 12V/24V DC para os equipamentos

### Conexões ESP32
| Pino ESP32 | Conexão |
|------------|---------|
| GPIO16 (RX2) | MAX485 RO (Recv Out) |
| GPIO17 (TX2) | MAX485 DI (Data In) |
| GPIO4 | MAX485 DE/RE (Direction) |
| GPIO15 | Buzzer (+) |
| GPIO13 | Sensor Porta (+) |
| GPIO5 (SS) | W5500 CS |
| GPIO18 (SCK) | W5500 SCK |
| GPIO19 (MISO) | W5500 MISO |
| GPIO23 (MOSI) | W5500 MOSI |

### Fios RS485
| Dispositivo | RS485 A (+) | RS485 B (-) | GND |
|-------------|-------------|-------------|-----|
| Controlador | A+ | B- | GND |
| Inversor | A+ | B- | GND |
| Módulo MAX485 | A | B | GND |

## 📊 Lógica de Alarmes

### Porta Aberta
```
Tempo aberta  →  Ação
─────────────────────────────
< 5 minutos   →  Nenhuma
5 minutos     →  1 pulso no buzzer
6 minutos     →  Pulsos a cada 10 segundos
> 7 minutos   →  Buzzer contínuo
Porta fecha   →  Timer reseta, buzzer desliga
```

### Temperatura
```
Temperatura    →  Ação
─────────────────────────────
< 8°C         →  Normal
8°C - 12°C    →  Alerta WhatsApp
> 12°C        →  Crítico WhatsApp + Buzzer
```

## 📝 Licença

Projeto privado - Câmara Fria Monitoramento
