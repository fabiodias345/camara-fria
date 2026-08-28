# Lógica de alarmes

## Porta

- ED1 monitora o interruptor da porta.
- Porta fechada: zera temporizadores e desliga o Relé 1.
- Porta aberta por 5 minutos: aviso sonoro inicial pelo Relé 1.
- Porta aberta por 6 minutos: aviso sonoro a cada 10 segundos.
- Porta aberta após 7 minutos: alarme contínuo até fechar a porta.

A polaridade elétrica da ED1 e o estado lógico do Relé 1 serão testados em bancada antes da ativação da lógica.

## Temperatura e comunicação

Temperatura acima do limite cria alarme local, registro persistente e evento para WhatsApp. Falha de comunicação RS-485 deixa o equipamento offline e preserva o último alarme com horário. Histerese e atraso serão configuráveis para evitar oscilações.