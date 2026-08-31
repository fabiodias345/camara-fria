#!/usr/bin/env python3
"""
Simulador ESP32 - Câmara Fria
Simula dados do controlador RKC REX-C100 e inversor WEG CFW500
enviando para as Edge Functions do Supabase Cloud.

Uso: python scripts/simulate_esp32.py [--interval SECONDS] [--scenario SCENARIO]
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

# Fix Windows console encoding
if sys.platform == "win32":
    os.environ.setdefault("PYTHONIOENCODING", "utf-8")
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

# ── Configurações ──────────────────────────────────────────────────────
DEFAULT_HOST = "hnfdlpjkoxizrgukjcrw.supabase.co"
DEFAULT_INTERVAL = 3
DEVICE_ID = "esp32-camara-fria-01"
SUPABASE_KEY = "sb_publishable_DReGLMwglDMCB5DYoPcQwQ_3_gVWFui"

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
        self.humidity = 65.0
        self.buzzer_active = False
        self.cycle_count = 0
        self.scenario = "normal"

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
            "humidity": round(self.humidity, 1),
            "door_open": self.door_open,
            "door_open_seconds": self.door_open_seconds,
            "door_stage": self.door_stage,
            "buzzer_active": self.buzzer_active,
        }


def simulate_normal(state, dt):
    noise = random.gauss(0, 0.1)
    drift = (state.setpoint - state.temperature) * 0.02
    state.temperature += drift + noise
    if state.temperature > state.setpoint + 0.5:
        state.heater_output = max(0, state.heater_output - random.uniform(1, 3))
    elif state.temperature < state.setpoint - 0.5:
        state.heater_output = min(100, state.heater_output + random.uniform(1, 3))
    state.humidity = 65 + random.gauss(0, 2)
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.8 + random.gauss(0, 0.3)
    state.inverter_dc_voltage = 540 + random.gauss(0, 1)
    state.inverter_temp = 35 + random.gauss(0, 0.5)
    state.inverter_running = True
    state.inverter_fault = 0
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0
    state.buzzer_active = False


def simulate_high_temp(state, dt):
    state.temperature += random.uniform(0.05, 0.15)
    state.heater_output = 0
    state.humidity = 75 + random.gauss(0, 3)
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.0 + random.gauss(0, 0.3)
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0
    state.buzzer_active = state.temperature > -8.0
    if state.temperature >= -10.0:
        print(f"  *** Temperatura critica! -10C! Voltando ao normal... ***")
        state.scenario = "normal"
        state.temperature = -12.0


def simulate_door_open(state, dt):
    state.door_open = True
    state.door_open_seconds += int(dt)
    state.temperature += random.uniform(0.01, 0.05)
    state.humidity = min(95, 65 + state.door_open_seconds * 0.1)
    if state.door_open_seconds >= 420:
        state.door_stage = 3
    elif state.door_open_seconds >= 360:
        state.door_stage = 2
    elif state.door_open_seconds >= 300:
        state.door_stage = 1
    else:
        state.door_stage = 0
    state.inverter_freq = 49.8 + random.gauss(0, 0.1)
    state.inverter_voltage = 380 + random.gauss(0, 2)
    state.inverter_current = 12.8 + random.gauss(0, 0.3)
    state.buzzer_active = state.door_open_seconds >= 300
    if state.door_open_seconds >= 480:
        print(f"  PORTA fechada automaticamente apos 8 minutos!")
        state.door_open = False
        state.door_open_seconds = 0
        state.door_stage = 0
        state.scenario = "normal"


def simulate_fault(state, dt):
    state.inverter_running = False
    state.inverter_fault = 7
    state.inverter_freq = 0
    state.inverter_current = 0
    state.inverter_temp += random.uniform(0.1, 0.3)
    state.temperature += random.uniform(0.02, 0.08)
    state.humidity = 70 + random.gauss(0, 2)
    state.door_open = False
    state.door_open_seconds = 0
    state.door_stage = 0
    state.buzzer_active = state.temperature > -15.0
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


def send_telemetry(base_url, data):
    """Envia dados via Edge Function receive-telemetry."""
    url = f"{base_url}/functions/v1/receive-telemetry"
    payload = json.dumps(data).encode("utf-8")
    
    req = Request(url, data=payload, method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("apikey", SUPABASE_KEY)
    
    try:
        with urlopen(req, timeout=10) as resp:
            body = json.loads(resp.read().decode())
            return resp.status in (200, 201), body
    except HTTPError as e:
        error_body = e.read().decode()[:200]
        print(f"  [ERR] HTTP {e.code}: {error_body}")
        return False, None
    except URLError as e:
        print(f"  [ERR] Conexao falhou: {e.reason}")
        return False, None


def print_status(state, sent_ok, response):
    status = "OK" if sent_ok else "ERR"
    temp_str = f"{state.temperature:+.1f}C"
    if state.temperature > -15:
        temp_str = f"** {temp_str} **"
    
    door_str = f"PORTA ABERTA {state.door_open_seconds}s (Nivel {state.door_stage})" if state.door_open else "PORTA Fechada"
    inv_str = f"{state.inverter_freq:.1f}Hz / {state.inverter_voltage:.0f}V / {state.inverter_current:.1f}A" if state.inverter_running else f"FALHA #{state.inverter_fault}"
    
    alerts = response.get("alerts_created", 0) if response else 0
    alert_str = f" [{alerts} alertas]" if alerts > 0 else ""
    
    print(f"  [{status}]{alert_str} {temp_str} | SP: {state.setpoint}C | Heater: {state.heater_output:.0f}%")
    print(f"     INV: {inv_str}")
    print(f"     {door_str}")


def main():
    parser = argparse.ArgumentParser(description="Simulador ESP32 - Camara Fria (Edge Functions)")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Host do Supabase")
    parser.add_argument("--interval", type=float, default=DEFAULT_INTERVAL, help="Intervalo em segundos")
    parser.add_argument("--scenario", choices=["auto", "normal", "high_temp", "door_open", "fault"], 
                       default="auto", help="Cenario de simulacao")
    parser.add_argument("--count", type=int, default=0, help="Numero de envios (0 = infinito)")
    args = parser.parse_args()
    
    base_url = f"https://{args.host}"
    state = SimulationState()
    
    print("=" * 60)
    print("SIMULADOR ESP32 - CAMARA FRIGORIFICA")
    print("Usando Supabase Edge Functions")
    print("=" * 60)
    print(f"  API: {base_url}/functions/v1/receive-telemetry")
    print(f"  Intervalo: {args.interval}s")
    print(f"  Cenario: {args.scenario}")
    print(f"  Dispositivo: {DEVICE_ID}")
    print("=" * 60)
    print()
    
    # Testa conexao
    print("Testando conexao com a Edge Function...")
    test_data = state.to_dict()
    ok, resp = send_telemetry(base_url, test_data)
    if not ok:
        print("NAO foi possivel conectar. Verifique a configuracao.")
        sys.exit(1)
    print("Conexao OK!\n")
    
    # Sequencia de cenarios (modo auto)
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
        seq_remaining = 999999
    else:
        scenario_name, duration, description = auto_sequence[0]
        state.scenario = scenario_name
        seq_remaining = duration
        seq_description = description
    
    try:
        while True:
            if args.scenario == "auto":
                if seq_remaining <= 0:
                    seq_index = (seq_index + 1) % len(auto_sequence)
                    scenario_name, duration, description = auto_sequence[seq_index]
                    state.scenario = scenario_name
                    seq_remaining = duration
                    print(f"\n{'-' * 50}")
                    print(f"  {description}")
                    print(f"{'-' * 50}\n")
                seq_remaining -= 1
            
            sim_func = SCENARIOS[state.scenario]
            sim_func(state, args.interval)
            
            data = state.to_dict()
            sent, resp = send_telemetry(base_url, data)
            
            timestamp = datetime.now(timezone.utc).strftime("%H:%M:%S")
            print(f"[{timestamp}] #{state.cycle_count:04d}")
            print_status(state, sent, resp)
            print()
            
            state.cycle_count += 1
            if args.count > 0 and state.cycle_count >= args.count:
                print(f"\n{args.count} envios concluidos.")
                break
            time.sleep(args.interval)
            
    except KeyboardInterrupt:
        print(f"\n\nSimulador encerrado.")
        print(f"   Total de envios: {state.cycle_count}")
        print(f"   Temperatura final: {state.temperature:.1f}C")
        sys.exit(0)


if __name__ == "__main__":
    main()
