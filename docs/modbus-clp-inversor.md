# Mapa Modbus — CLP/inversor

Fonte inicial: manual do inversor série 9600. O manual informa Modbus-RTU, parâmetros de baud rate e endereço local, mas não autoriza inventar registradores para o modelo instalado.

| Dado | Função/endereço | Escala | Unidade | Fonte/status |
|---|---|---|---|---|
| Tensão | Pendente | Pendente | V | Confirmar modelo |
| Corrente | Pendente | Pendente | A | Confirmar modelo |
| Frequência | Pendente | Pendente | Hz | Confirmar modelo |
| Estado | Pendente | Pendente | enum | Confirmar modelo |
| Alarmes/erros | Pendente | Pendente | código | Confirmar modelo |

Configuração inicial de teste: Modbus-RTU, 9600 baud, 8N1, endereço configurável. Confirmar PD-00/PD-01/PD-02 no equipamento.
