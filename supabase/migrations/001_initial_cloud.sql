-- ============================================
-- CAMARA FRIGORIFICA - Schema para Supabase Cloud
-- ============================================
-- Adaptado para Supabase Cloud (roles ja existem: anon, service_role)

-- ============================================
-- TABELA: devices (Dispositivos ESP32)
-- ============================================
CREATE TABLE IF NOT EXISTS devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(64) UNIQUE NOT NULL,
    name VARCHAR(128) NOT NULL DEFAULT 'Camara Fria',
    location VARCHAR(256),
    firmware_version VARCHAR(32) DEFAULT '1.0.0',
    is_online BOOLEAN DEFAULT false,
    last_seen TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now()
);

-- Dispositivo padrao
INSERT INTO devices (device_id, name, location, is_online)
VALUES ('esp32-camara-fria-01', 'Camara Fria Principal', 'Cozinha Industrial', true)
ON CONFLICT (device_id) DO NOTHING;

-- ============================================
-- TABELA: telemetry (Dados de telemetria)
-- ============================================
CREATE TABLE IF NOT EXISTS telemetry (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    temperature DECIMAL(5,2),
    setpoint DECIMAL(5,2),
    heater_output DECIMAL(5,2),
    inverter_frequency DECIMAL(6,2),
    inverter_voltage DECIMAL(6,1),
    inverter_current DECIMAL(6,2),
    inverter_dc_voltage DECIMAL(6,1),
    inverter_temperature DECIMAL(5,1),
    inverter_running BOOLEAN DEFAULT false,
    inverter_fault_code INTEGER DEFAULT 0,
    door_open BOOLEAN DEFAULT false,
    door_open_seconds INTEGER DEFAULT 0,
    door_stage INTEGER DEFAULT 0,
    timestamp TIMESTAMPTZ DEFAULT now(),
    raw_json JSONB
);

CREATE INDEX IF NOT EXISTS idx_telemetry_device_id ON telemetry(device_id);
CREATE INDEX IF NOT EXISTS idx_telemetry_timestamp ON telemetry(timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_telemetry_temperature ON telemetry(temperature);
CREATE INDEX IF NOT EXISTS idx_telemetry_device_time ON telemetry(device_id, timestamp DESC);

-- ============================================
-- TABELA: telemetry_current (Ultima leitura)
-- ============================================
CREATE TABLE IF NOT EXISTS telemetry_current (
    device_id VARCHAR(64) PRIMARY KEY REFERENCES devices(device_id),
    temperature DECIMAL(5,2),
    setpoint DECIMAL(5,2),
    heater_output DECIMAL(5,2),
    inverter_frequency DECIMAL(6,2),
    inverter_voltage DECIMAL(6,1),
    inverter_current DECIMAL(6,2),
    inverter_dc_voltage DECIMAL(6,1),
    inverter_temperature DECIMAL(5,1),
    inverter_running BOOLEAN DEFAULT false,
    inverter_fault_code INTEGER DEFAULT 0,
    door_open BOOLEAN DEFAULT false,
    door_open_seconds INTEGER DEFAULT 0,
    door_stage INTEGER DEFAULT 0,
    updated_at TIMESTAMPTZ DEFAULT now()
);

-- ============================================
-- TABELA: alerts (Historico de alertas)
-- ============================================
CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    alert_type VARCHAR(32) NOT NULL,
    severity VARCHAR(16) NOT NULL,
    message TEXT NOT NULL,
    temperature DECIMAL(5,2),
    metadata JSONB,
    acknowledged BOOLEAN DEFAULT false,
    acknowledged_by VARCHAR(128),
    acknowledged_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_alerts_device ON alerts(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(alert_type, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_unack ON alerts(acknowledged, created_at DESC) WHERE NOT acknowledged;

-- ============================================
-- TABELA: alert_config (Configuracao de alertas)
-- ============================================
CREATE TABLE IF NOT EXISTS alert_config (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    alert_type VARCHAR(32) NOT NULL,
    threshold_value DECIMAL(10,2),
    enabled BOOLEAN DEFAULT true,
    cooldown_seconds INTEGER DEFAULT 300,
    whatsapp_enabled BOOLEAN DEFAULT true,
    whatsapp_numbers TEXT[] DEFAULT '{}',
    message_template TEXT,
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now(),
    UNIQUE(device_id, alert_type)
);

-- Configuracoes padrao
INSERT INTO alert_config (device_id, alert_type, threshold_value, message_template, whatsapp_enabled)
VALUES
    ('esp32-camara-fria-01', 'temperature_high', 8.0,
     'ALERTA Camara Fria: Temperatura em {temperature}C (max: {threshold}C). Setpoint: {setpoint}C',
     true),
    ('esp32-camara-fria-01', 'temperature_critical', 12.0,
     'CRITICO Camara Fria: Temperatura em {temperature}C! Risco de deterioracao. Verificar IMEDIATAMENTE.',
     true),
    ('esp32-camara-fria-01', 'door_open', 300,
     'ALERTA Camara Fria: Porta aberta ha {door_seconds} minutos! Fechar para manter temperatura.',
     true),
    ('esp32-camara-fria-01', 'inverter_fault', 0,
     'FALHA Inversor: Codigo de erro {fault_code}. Frequencia: {frequency} Hz. Verificar equipamento.',
     true)
ON CONFLICT (device_id, alert_type) DO NOTHING;

-- ============================================
-- TABELA: whatsapp_log
-- ============================================
CREATE TABLE IF NOT EXISTS whatsapp_log (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL,
    alert_id BIGINT REFERENCES alerts(id),
    phone_number VARCHAR(20) NOT NULL,
    message TEXT NOT NULL,
    status VARCHAR(16) DEFAULT 'pending',
    evolution_response JSONB,
    sent_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_whatsapp_log_device ON whatsapp_log(device_id, created_at DESC);

-- ============================================
-- FUNCOES E TRIGGERS
-- ============================================

-- Funcao: Atualizar telemetry_current
CREATE OR REPLACE FUNCTION update_telemetry_current()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO telemetry_current (
        device_id, temperature, setpoint, heater_output,
        inverter_frequency, inverter_voltage, inverter_current,
        inverter_dc_voltage, inverter_temperature,
        inverter_running, inverter_fault_code,
        door_open, door_open_seconds, door_stage, updated_at
    ) VALUES (
        NEW.device_id, NEW.temperature, NEW.setpoint, NEW.heater_output,
        NEW.inverter_frequency, NEW.inverter_voltage, NEW.inverter_current,
        NEW.inverter_dc_voltage, NEW.inverter_temperature,
        NEW.inverter_running, NEW.inverter_fault_code,
        NEW.door_open, NEW.door_open_seconds, NEW.door_stage, now()
    )
    ON CONFLICT (device_id) DO UPDATE SET
        temperature = EXCLUDED.temperature,
        setpoint = EXCLUDED.setpoint,
        heater_output = EXCLUDED.heater_output,
        inverter_frequency = EXCLUDED.inverter_frequency,
        inverter_voltage = EXCLUDED.inverter_voltage,
        inverter_current = EXCLUDED.inverter_current,
        inverter_dc_voltage = EXCLUDED.inverter_dc_voltage,
        inverter_temperature = EXCLUDED.inverter_temperature,
        inverter_running = EXCLUDED.inverter_running,
        inverter_fault_code = EXCLUDED.inverter_fault_code,
        door_open = EXCLUDED.door_open,
        door_open_seconds = EXCLUDED.door_open_seconds,
        door_stage = EXCLUDED.door_stage,
        updated_at = now();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

CREATE TRIGGER trigger_update_current
    AFTER INSERT ON telemetry
    FOR EACH ROW
    EXECUTE FUNCTION update_telemetry_current();

-- Funcao: Auto-criar alertas
CREATE OR REPLACE FUNCTION check_and_create_alerts()
RETURNS TRIGGER AS $$
DECLARE
    config_row RECORD;
    last_alert RECORD;
BEGIN
    FOR config_row IN
        SELECT * FROM alert_config
        WHERE device_id = NEW.device_id AND enabled = true
    LOOP
        SELECT INTO last_alert created_at FROM alerts
        WHERE device_id = NEW.device_id AND alert_type = config_row.alert_type
        ORDER BY created_at DESC LIMIT 1;

        IF last_alert IS NOT NULL AND
           (EXTRACT(EPOCH FROM (now() - last_alert.created_at)) < config_row.cooldown_seconds) THEN
            CONTINUE;
        END IF;

        IF config_row.alert_type = 'temperature_high' AND
           NEW.temperature >= config_row.threshold_value AND
           NEW.temperature < (SELECT threshold_value FROM alert_config
                             WHERE device_id = NEW.device_id AND alert_type = 'temperature_critical') THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, temperature, metadata)
            VALUES (NEW.device_id, 'temperature_high', 'warning',
                replace(replace(replace(config_row.message_template,
                    '{temperature}', NEW.temperature::text),
                    '{threshold}', config_row.threshold_value::text),
                    '{setpoint}', NEW.setpoint::text),
                NEW.temperature,
                jsonb_build_object('temperature', NEW.temperature, 'setpoint', NEW.setpoint));
        END IF;

        IF config_row.alert_type = 'temperature_critical' AND
           NEW.temperature >= config_row.threshold_value THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, temperature, metadata)
            VALUES (NEW.device_id, 'temperature_critical', 'critical',
                replace(replace(replace(config_row.message_template,
                    '{temperature}', NEW.temperature::text),
                    '{threshold}', config_row.threshold_value::text),
                    '{setpoint}', NEW.setpoint::text),
                NEW.temperature,
                jsonb_build_object('temperature', NEW.temperature, 'setpoint', NEW.setpoint));
        END IF;

        IF config_row.alert_type = 'door_open' AND
           NEW.door_open = true AND
           NEW.door_open_seconds >= config_row.threshold_value THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, metadata)
            VALUES (NEW.device_id, 'door_open', 'warning',
                replace(config_row.message_template,
                    '{door_seconds}', (NEW.door_open_seconds / 60)::text),
                jsonb_build_object('door_open_seconds', NEW.door_open_seconds));
        END IF;

        IF config_row.alert_type = 'inverter_fault' AND
           NEW.inverter_fault_code > 0 THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, metadata)
            VALUES (NEW.device_id, 'inverter_fault', 'critical',
                replace(replace(config_row.message_template,
                    '{fault_code}', NEW.inverter_fault_code::text),
                    '{frequency}', NEW.inverter_frequency::text),
                jsonb_build_object('fault_code', NEW.inverter_fault_code));
        END IF;
    END LOOP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

CREATE TRIGGER trigger_check_alerts
    AFTER INSERT ON telemetry
    FOR EACH ROW
    EXECUTE FUNCTION check_and_create_alerts();

-- Funcao: update_updated_at
CREATE OR REPLACE FUNCTION update_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = now();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_devices_updated
    BEFORE UPDATE ON devices
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at();

-- ============================================
-- RLS (Row Level Security)
-- ============================================
ALTER TABLE telemetry ENABLE ROW LEVEL SECURITY;
ALTER TABLE telemetry_current ENABLE ROW LEVEL SECURITY;
ALTER TABLE alerts ENABLE ROW LEVEL SECURITY;
ALTER TABLE alert_config ENABLE ROW LEVEL SECURITY;
ALTER TABLE whatsapp_log ENABLE ROW LEVEL SECURITY;
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;

-- Policies para anon (leitura + insercao para ESP32)
CREATE POLICY "anon_read_telemetry_current" ON telemetry_current FOR SELECT TO anon USING (true);
CREATE POLICY "anon_all_telemetry_current" ON telemetry_current FOR ALL TO anon USING (true) WITH CHECK (true);
CREATE POLICY "anon_read_telemetry" ON telemetry FOR SELECT TO anon USING (true);
CREATE POLICY "anon_insert_telemetry" ON telemetry FOR INSERT TO anon WITH CHECK (true);
CREATE POLICY "anon_read_alerts" ON alerts FOR SELECT TO anon USING (true);
CREATE POLICY "anon_insert_alerts" ON alerts FOR INSERT TO anon WITH CHECK (true);
CREATE POLICY "anon_read_alert_config" ON alert_config FOR SELECT TO anon USING (true);
CREATE POLICY "anon_read_devices" ON devices FOR SELECT TO anon USING (true);
CREATE POLICY "anon_read_whatsapp_log" ON whatsapp_log FOR SELECT TO anon USING (true);
CREATE POLICY "anon_insert_whatsapp_log" ON whatsapp_log FOR INSERT TO anon WITH CHECK (true);

-- Policies para service_role (acesso completo)
CREATE POLICY "sr_all_telemetry" ON telemetry FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "sr_all_alerts" ON alerts FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "sr_all_devices" ON devices FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "sr_all_whatsapp_log" ON whatsapp_log FOR ALL TO service_role USING (true) WITH CHECK (true);

-- ============================================
-- VIEWS
-- ============================================
CREATE OR REPLACE VIEW v_current_status AS
SELECT
    d.device_id, d.name as device_name, d.location, d.is_online, d.last_seen,
    tc.temperature, tc.setpoint, tc.heater_output,
    tc.inverter_frequency, tc.inverter_voltage, tc.inverter_current,
    tc.inverter_running, tc.inverter_fault_code,
    tc.door_open, tc.door_open_seconds, tc.door_stage, tc.updated_at
FROM devices d
LEFT JOIN telemetry_current tc ON d.device_id = tc.device_id;

CREATE OR REPLACE VIEW v_pending_alerts AS
SELECT a.*, d.name as device_name
FROM alerts a
JOIN devices d ON a.device_id = d.device_id
WHERE NOT a.acknowledged
ORDER BY a.created_at DESC
LIMIT 5;

CREATE OR REPLACE VIEW v_temperature_history AS
SELECT
    date_trunc('hour', timestamp) +
    (EXTRACT(minute FROM timestamp)::int / 5) * interval '5 minutes' as time_bucket,
    device_id,
    AVG(temperature) as avg_temp,
    MIN(temperature) as min_temp,
    MAX(temperature) as max_temp,
    AVG(setpoint) as avg_setpoint,
    COUNT(*) as readings
FROM telemetry
WHERE timestamp > now() - interval '24 hours'
GROUP BY time_bucket, device_id
ORDER BY time_bucket DESC;
