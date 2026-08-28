# Hardware e compatibilidade

## Controlador da fase atual

- Hoffer PLC HF-006Enet.
- Alimentação: 12 Vcc ou 24 Vcc.
- Interface utilizada agora: RS-485 integrada para o TC300.
- Rede utilizada agora: Wi-Fi; Ethernet permanece disponível como alternativa.
- Porta: ED1 / GPIO36 para o interruptor.
- Alarme: Relé 1 / SD1 / GPIO4 para o buzzer externo, após confirmar a carga.

## Equipamento monitorado agora

- Controlador TC300: somente leitura de temperatura, umidade, setpoint, estado do sensor e alarmes via Modbus-RTU.
- O endereço, baud rate, paridade, stop bits e registradores continuam pendentes do modelo e manual do TC300.

## Fora do escopo desta fase

O inversor/CLP não será conectado nem configurado agora. O segundo equipamento será documentado e integrado posteriormente, após concluir os testes do TC300.

## Segurança

Montar somente com a placa desenergizada. Usar fonte adequada, fusível, aterramento e separação entre potência e sinais. RS-485 deve usar par trançado, topologia linear e terminação somente quando confirmada para o barramento.