#!/usr/bin/env python3
"""
Simulador ESP32 - Câmara Fria
Simula dados do controlador RKC REX-C100 e inversor WEG CFW500
enviando para a API PostgREST em tempo real.

Uso: python scripts/simulate_esp32.py [--host HOST] [--port PORT] [--interval SECONDS]
"""

import argparse
import json
import math
import os
import random
import sys
import time
from datetime import datetime, timezone
from urllib.request import Request, urlopen
from urllib.error import URLError, HTTPError

# Fix Windows console encoding for emojis
if sys.platform == "win32":
    os.environ.setdefault("PYTHONIOENCODING", "utf-8")
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

# ── Configurações padrão ──────────────────────────────────────────────
DEFAULT_HOST = "hnfdlpjkoxizrgukjcrw.supabase.co"
DEFAULT_PORT = 443
DEFAULT_PROTOCOL = "https"
DEFAULT_INTERVAL = 3  # segundos entre cada envio
DEVICE_ID = "esp32-camara-fria-01"

# ── Estado da simulação ───────────────────────────────────────────────
class SimulationState:
    def __init__(self):
        self.temperature = -18.0
        self.setpoint = -18.0
        self.heater_output = 45.0
        self.inverter_freq = 49.8
        self.inverter_voltage = 380.0
        self.inverter_current = 12.8
        self.inverter_dc_voltage = 540.0
        self.inverter_temp = 35.0
        self.inverter_running = True
        self.inverter_fault = 0
        self.door_open = False
        self.door_open_seconds = 0
        self.door_stage = 0
        self.cycle_count = 0
        self.scenario = "normal"  # normal, high_temp, door_open, fault

    def to_dict(self):
        return {
            "device_id": DEVICE_ID,
            "temperature": round(self.temperature, 2),
            "setpoint": round(self.setpoint, 2),
            "heater_output": round(self.heater_output, 2),
            "inverter_frequency": round(self.inverter_freq, 2),
            "inverter_voltage": round(self.inverter_voltage, 1),
            "inverter_current": round(self.inverter_current, 2),
            "inverter_dc_voltage": round(self.inverter_dc_voltage, 1),
            "inverter_temperature": round(self.inverter_temp, 1),
            "inverter_running": self.inverter_running,
            "inverter_fault_code": self.inverter_fault,
            "door_open": self.door_open,
            "door_open_seconds": self.door_open_seconds,
            "door_stage": self.door_stage,
        }


def simulate_normal(state: SimulationState, dt: float):
    """Cenário normal: temperatura oscila suavemente em torno do setpoint."""
    # Ruido na temperatura
    noise = random.gauss(0, 0.1)
    # Tendência suave para o setpoint
    drift = (state.setpoint - state.temperature) * 0.02
    state.temperature += drift + noise
    
    # Heater output varia com a temperatura
    if state.temperature > state.setpoint + 0.5:
        state.heater_output = max(0, state.heater_output - random.uniform(1, 3))
    elif state.temperature < state.setpoint - 0.5:
        state.heater_output = min(100, state.heater_output + random.uniform(1, 3))
    
    # Inversor opera normalmente
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.8 + random.gauss(0, 0.3)
    state.inverter_dc_voltage = 540 + random.gauss(0, 1)
    state.inverter_temp = 35 + random.gauss(0, 0.5)
    state.inverter_running = True
    state.inverter_fault = 0
    
    # Porta fechada
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0


def simulate_high_temp(state: SimulationState, dt: float):
    """Cenário de temperatura alta: sobe gradualmente acima do limit."""
    # Temperatura sobe (descongelamento, falha, etc)
    state.temperature += random.uniform(0.05, 0.15)
    state.heater_output = 0
    
    # Inversor continua normal
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.0 + random.gauss(0, 0.3)
    
    # Porta fechada
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0
    
    # Se temperatura chegou a -10°C, volta ao normal gradualmente
    if state.temperature >= -10.0:
        print(f"  *** Temperatura critica! -10C! Voltando ao normal... ***")
        state.scenario = "normal"
        state.temperature = -12.0  # Reseta um pouco acima do setpoint


def simulate_door_open(state: SimulationState, dt: float):
    """Cenário de porta aberta: simula abertura gradual."""
    state.door_open = True
    state.door_open_seconds += int(dt)
    
    # Temperatura sobe缓慢 quando porta aberta
    state.temperature += random.uniform(0.01, 0.05)
    
    # Lógica de estágios (5min, 6min, 7min)
    if state.door_open_seconds >= 420:  # 7 minutos
        state.door_stage = 3
    elif state.door_open_seconds >= 360:  # 6 minutos
        state.door_stage = 2
    elif state.door_open_seconds >= 300:  # 5 minutos
        state.door_stage = 1
    else:
        state.door_stage = 0
    
    # Inversor normal
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.8 + random.gauss(0, 0.3)
    
    # Se ficou aberta tempo demais, fecha automaticamente
    if state.door_open_seconds >= 480:  # 8 minutos fecha
        print(f"  PORTA fechada automaticamente apos 8 minutos!")
        state.door_open = False
        state.door_open_seconds = 0
        state.door_stage = 0
        state.scenario = "normal"


def simulate_fault(state: SimulationState, dt: float):
    """Cenário de falha no inversor."""
    state.inverter_running = False
    state.inverter_fault = 7  # Código de falha genérico
    state.inverter_freq = 0
    state.inverter_current = 0
    state.inverter_temp += random.uniform(0.1, 0.3)  # Esquenta sem ventilação
    
    # Temperatura sobe sem o inversor trabalhando
    state.temperature += random.uniform(0.02, 0.08)
    
    # Porta fechada
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0
    
    # Depois de um tempo, "reinicia"
    if state.inverter_temp >= 80:
        print(f"  INVERSOR reiniciado apos superaquecimento!")
        state.inverter_running = True
        state.inverter_fault = 0
        state.inverter_temp = 35
        state.inverter_freq = 49.8
        state.inverter_current = 12.8
        state.scenario = "normal"


SCENARIOS = {
    "normal": simulate_normal,
    "high_temp": simulate_high_temp,
    "door_open": simulate_door_open,
    "fault": simulate_fault,
}


SUPABASE_KEY = "sb_publishable_DReGLMwglDMCB5DYoPcQwQ_3_gVWFui"

def send_telemetry(base_url: str, data: dict) -> bool:
    """Envia dados de telemetria para a API."""
    url = f"{base_url}/rest/v1/telemetry"
    payload = json.dumps(data).encode("utf-8")
    
    req = Request(url, data=payload, method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("apikey", SUPABASE_KEY)
    req.add_header("Prefer", "return=minimal")
    
    try:
        with urlopen(req, timeout=5) as resp:
            return resp.status in (200, 201, 204)
    except HTTPError as e:
        print(f"  ❌ HTTP {e.code}: {e.read().decode()[:200]}")
        return False
    except URLError as e:
        print(f"  ❌ Conexão falhou: {e.reason}")
        return False


def print_status(state: SimulationState, sent_ok: bool):
    """Imprime status formatado."""
    status = "OK" if sent_ok else "ERR"
    temp_str = f"{state.temperature:+.1f}C"
    if state.temperature > -15:
        temp_str = f"** {temp_str} **"
    
    door_str = f"PORTA ABERTA {state.door_open_seconds}s (Nivel {state.door_stage})" if state.door_open else "PORTA Fechada"
    inv_str = f"{state.inverter_freq:.1f}Hz / {state.inverter_voltage:.0f}V / {state.inverter_current:.1f}A" if state.inverter_running else f"FALHA #{state.inverter_fault}"
    
    print(f"  [{status}] {temp_str} | SP: {state.setpoint}C | Heater: {state.heater_output:.0f}%")
    print(f"     INV: {inv_str}")
    print(f"     {door_str}")


def main():
    parser = argparse.ArgumentParser(description="Simulador ESP32 - Câmara Fria")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Host da API")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Porta da API")
    parser.add_argument("--protocol", default=DEFAULT_PROTOCOL, help="Protocolo (http/https)")
    parser.add_argument("--local", action="store_true", help="Usar API local (localhost:3010)")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL, help="Intervalo em segundos (default: 3)")
    parser.add_argument("--scenario", choices=["auto", "normal", "high_temp", "door_open", "fault"], 
                       default="auto", help="Cenário de simulação (default: auto)")
    parser.add_argument("--count", type=int, default=0, help="Número de envios (0 = infinito)")
    args = parser.parse_args()
    
    if args.local:
        args.host = "localhost"
        args.port = 3010
    
    if args.port == 443:
        base_url = f"https://{args.host}"
    else:
        base_url = f"http://{args.host}:{args.port}"
    state = SimulationState()
    
    print("=" * 60)
    print("SIMULADOR ESP32 - CAMARA FRIGORIFICA")
    print("=" * 60)
    print(f"  API: {base_url}")
    print(f"  Intervalo: {args.interval}s")
    print(f"  Cenario: {args.scenario}")
    print(f"  Dispositivo: {DEVICE_ID}")
    print("=" * 60)
    print()
    
    # Testa conexão
    print("Testando conexao com a API...")
    test_data = state.to_dict()
    if not send_telemetry(base_url, test_data):
        print("NAO foi possivel conectar a API. Verifique se o docker compose esta rodando.")
        print("   Execute: docker compose up -d")
        sys.exit(1)
    print("Conexao OK!\n")
    
    # Sequência de cenários (modo auto)
    auto_sequence = [
        ("normal", 10, "[NORMAL] Operacao normal - dados estaveis"),
        ("door_open", 20, "[PORTA]  Simulando abertura de porta..."),
        ("normal", 8, "[NORMAL] Retomando operacao normal"),
        ("high_temp", 15, "[TEMP]   Simulando subida de temperatura!"),
        ("normal", 10, "[NORMAL] Resfriando - operacao normal"),
        ("fault", 12, "[FAULT]  Simulando falha no inversor!"),
        ("normal", 10, "[NORMAL] Inversor recuperado - normalizando"),
    ]
    
    seq_index = 0
    if args.scenario != "auto":
        state.scenario = args.scenario
    else:
        scenario_name, duration, description = auto_sequence[0]
        state.scenario = scenario_name
        seq_remaining = duration
        seq_description = description
    seq_remaining = auto_sequence[0][1] if args.scenario == "auto" else 999999
    seq_description = auto_sequence[0][2] if args.scenario == "auto" else ""
    
    try:
        while True:
            # Auto-scenario management
            if args.scenario == "auto":
                if seq_remaining <= 0:
                    seq_index = (seq_index + 1) % len(auto_sequence)
                    scenario_name, duration, description = auto_sequence[seq_index]
                    state.scenario = scenario_name
                    seq_remaining = duration
                    seq_description = description
                    print(f"\n{'-' * 50}")
                    print(f"  {description}")
                    print(f"{'-' * 50}\n")
                seq_remaining -= 1
            
            # Simula dados
            sim_func = SCENARIOS[state.scenario]
            sim_func(state, args.interval)
            
            # Envia para API
            data = state.to_dict()
            sent = send_telemetry(base_url, data)
            
            # Mostra status
            timestamp = datetime.now(timezone.utc).strftime("%H:%M:%S")
            print(f"[{timestamp}] #{state.cycle_count:04d}")
            print_status(state, sent)
            print()
            
            state.cycle_count += 1
            if args.count > 0 and state.cycle_count >= args.count:
                print(f"\n{args.count} envios concluidos.")
                break
            time.sleep(args.interval)
            
    except KeyboardInterrupt:
        print("\n\nSimulador encerrado.")
        print(f"   Total de envios: {state.cycle_count}")
        print(f"   Temperatura final: {state.temperature:.1f}C")
        sys.exit(0)


if __name__ == "__main__":
    main()
