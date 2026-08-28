# Pinagem da Hoffer PLC HF-006Enet

Fonte: Manual de Utilização HF-006Enet, revisão Julho/2026. Os nomes ED e SD serão usados na instalação; os GPIOs servem somente ao firmware.

## Entradas digitais

| Borne | GPIO | Uso reservado |
|---|---:|---|
| ED1 | 36 | Interruptor da porta |
| ED2 | 39 | Reserva |
| ED3 | 32 | Reserva |
| ED4 | 33 | Reserva |
| ED5 | 25 | Reserva |
| ED6 | 26 | Reserva |
| ED7 | 27 | Reserva |
| ED8 | 14 | Não usar: compartilhado com HSPI/Ethernet |

## Saídas a relé

| Borne | GPIO | Uso reservado |
|---|---:|---|
| SD1 | 4 | Alarme sonoro externo |
| SD2 | 16 | Reserva |
| SD3 | 17 | Reserva |
| SD4 | 18 | Reserva |
| SD5 | 19 | Reserva |
| SD6 | 23 | Reserva |

Cada saída disponibiliza C, NA e NF. A carga deve ser ligada pelo contato do relé, respeitando a corrente e tensão confirmadas no manual e na etiqueta do relé.

## Comunicação interna

| Função | GPIO | Observação |
|---|---:|---|
| RS-485 / UART0 TX | 1 | Compartilhado com serial TTL |
| RS-485 / UART0 RX | 3 | Compartilhado com serial TTL |
| RS-485 DE/RE | 5 | Controle de direção half-duplex |
| Ethernet MOSI | 13 | W5500 / HSPI |
| Ethernet MISO | 12 | W5500 / HSPI |
| Ethernet SCK | 14 | W5500 / HSPI; não usar ED8 |
| Ethernet CS | 15 | W5500 / HSPI |
| Ethernet INT | 2 | W5500 |
| Ethernet RST | 0 | W5500 |

Não utilizar a serial TTL simultaneamente ao RS-485.