# Arquitetura

A Hoffer PLC HF-006Enet é o controlador único do projeto. Ela recebe dados do CLP/inversor e do TC300 por RS-485/Modbus-RTU, monitora o interruptor da porta pela ED1 e disponibiliza o Relé 1 para o alarme sonoro externo. A telemetria sai pela Ethernet para a API de ingestão e o Supabase.

Fluxo: CLP/inversor + TC300 -> RS-485 da HF-006Enet -> Ethernet -> API -> Supabase -> Realtime -> dashboard.

A primeira versão é somente leitura: nenhum comando de partida, parada ou alteração de frequência será enviado aos equipamentos. O alarme de porta e os limites de temperatura devem continuar locais mesmo durante perda de rede.

O uso do RS-485 impede o uso simultâneo da serial TTL, pois ambos compartilham a UART0 da placa.