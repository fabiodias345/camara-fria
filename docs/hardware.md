# Hardware e compatibilidade

## Controlador do projeto

- Modelo: Hoffer PLC HF-006Enet.
- Fonte técnica: Manual de Utilização HF-006Enet, revisão Julho/2026, fornecido para este projeto.
- Alimentação: 12 Vcc ou 24 Vcc, com proteção contra inversão de polaridade.
- Entradas: oito digitais optoacopladas, compatíveis com sensores industriais de 12 Vcc ou 24 Vcc.
- Saídas: seis relés com contatos C, NA e NF; acionados internamente por ULN2003.
- Rede: Ethernet W5500 10/100 Mbps e Wi-Fi integrado.
- Comunicação: RS-485 integrada, half-duplex, compatível com Modbus RTU; a serial TTL usa a mesma UART e não pode operar junto ao RS-485.

## Uso no projeto

- Porta da câmara: entrada digital 1 (ED1 / GPIO36), reservada para o interruptor após teste de polaridade em bancada.
- Alarme sonoro: Relé 1 (SD1 / GPIO4), reservado para acionar o buzzer externo após definir tensão e corrente da carga. O buzzer visível na placa não possui função documentada no manual; não será usado pelo firmware sem confirmação do fabricante.
- CLP/inversor e TC300: barramento RS-485 A/B da placa. Não conectar até confirmar os bornes e a configuração Modbus dos dois equipamentos.

## Segurança

Montar somente com a placa desenergizada. Usar fonte 12/24 Vcc apropriada, fusível, aterramento do painel e separação entre cabos de potência e sinais. Para RS-485, usar par trançado em topologia linear; evitar estrela e usar terminação de 120 ohms apenas nas extremidades quando necessária.