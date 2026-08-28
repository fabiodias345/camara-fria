# Referência técnica — HF-006Enet

Fonte: arquivo `hf006net.zip` fornecido pelo usuário, Manual de Utilização HF-006Enet, Hoffer Automação Industrial e Comércio Ltda., revisão Julho/2026.

## Fatos confirmados

- Alimentação de 12 Vcc ou 24 Vcc.
- Oito entradas digitais optoacopladas.
- Seis saídas a relé, contatos C/NA/NF.
- Ethernet W5500 10/100 Mbps, Wi-Fi integrado e RS-485 integrada.
- RS-485 half-duplex para Modbus RTU.
- RS-485 e serial TTL compartilham UART0; usar somente uma por vez.
- Entradas: GPIO36, 39, 32, 33, 25, 26, 27 e 14; GPIO14 é compartilhado com HSPI/Ethernet.
- Relés: GPIO4, 16, 17, 18, 19 e 23.
- RS-485: UART0 TX GPIO1, RX GPIO3 e DE/RE GPIO5.

## Decisões deste projeto

ED1/GPIO36 foi reservada para a porta e SD1/GPIO4 para o alarme externo. O buzzer físico visível na placa não aparece no mapa funcional do manual; sua utilização fica bloqueada até confirmação do fabricante.