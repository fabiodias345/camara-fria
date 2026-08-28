# Arquitetura

A Hoffer PLC HF-006Enet é o controlador único da fase atual. Ela lê o controlador TC300 por RS-485/Modbus-RTU, monitora o interruptor da porta pela ED1, aciona o alarme local e envia a telemetria pela rede para a API e o Supabase.

Fluxo: TC300 -> RS-485 da HF-006Enet -> Wi-Fi ou Ethernet -> API -> Supabase -> Realtime -> dashboard.

A primeira fase exibe temperatura, umidade, setpoint, estado do sensor, porta, alarmes e saúde da comunicação. O firmware é somente leitura para o TC300 e não altera seus parâmetros.

O inversor/CLP está fora do escopo atual e será integrado em uma fase futura, sem compartilhar ainda o barramento RS-485.