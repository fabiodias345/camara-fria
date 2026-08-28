# Testes de entrada e saída — fase TC300

| ID | Tipo | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| IN-01 | RS-485 TC300 | Ler temperatura, umidade e setpoint conhecidos | Valores e escalas corretos | Pendente |
| IN-02 | Alarmes TC300 | Simular ou registrar estado de alarme | Código e estado preservados | Pendente |
| IN-03 | Entrada ED1 | Abrir/fechar a porta | Transição sem falsos eventos | Pendente |
| IN-04 | Falha serial | Remover cabo ou alterar endereço | TC300 offline e evento registrado | Pendente |
| OUT-01 | Relé 1 / buzzer | Simular 5, 6 e 7 minutos | Aviso, intervalo de 10 s e contínuo | Pendente |
| OUT-02 | Wi-Fi | Desconectar e reconectar a rede | Fila local e reconexão | Pendente |
| OUT-03 | Supabase | Inserir telemetria do TC300 | Dashboard recebe via Realtime | Pendente |
| OUT-04 | WhatsApp | Acionar temperatura acima do limite | Mensagem enviada ou erro auditado | Pendente |

O inversor/CLP terá checklist próprio quando entrar no escopo.