# Testes de entrada e saida

| ID | Tipo | Procedimento | Resultado esperado | Status |
|---|---|---|---|---|
| IN-01 | RS-485 inversor | Ler registro conhecido | Valor e escala corretos | Pendente |
| IN-02 | RS-485 TC300 | Ler temperatura/umidade | Valores coerentes | Pendente |
| IN-03 | Entrada digital da HF-006Enet | Abrir/fechar a porta | Transicao sem falsos eventos | Pendente |
| IN-04 | Falha serial | Remover cabo ou mudar endereco | Offline e alarme registrados | Pendente |
| OUT-01 | Alarme da HF-006Enet | Simular 5, 6 e 7 minutos | Aviso, intervalo e continuo | Pendente |
| OUT-02 | Supabase | Inserir telemetria | Dashboard recebe via Realtime | Pendente |
| OUT-03 | WhatsApp | Acionar alarme de temperatura | Mensagem enviada ou erro auditado | Pendente |

Cada execucao deve registrar data, operador, equipamento, configuracao, foto da ligacao e evidencia do resultado.