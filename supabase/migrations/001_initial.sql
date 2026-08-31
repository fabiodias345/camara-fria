-- ============================================
-- CÂMARA FRIA - Schema do Banco de Dados
-- ============================================
-- Migration: 001_initial
-- Descrição: Tabelas de telemetria, alertas, dispositivos e configurações

-- Extensões necessárias
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- ============================================
-- ROLES necessárias para PostgREST
-- ============================================
-- Role authenticator (usada pelo PostgREST para se conectar)
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'authenticator') THEN
        CREATE ROLE authenticator NOINHERIT LOGIN PASSWORD 'camara_fria_secret_2026';
    END IF;
END
$$;

-- Role anon (usuários anônimos - leitura)
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'anon') THEN
        CREATE ROLE anon NOLOGIN;
    END IF;
END
$$;

-- Role service_role (escrita completa via API)
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'service_role') THEN
        CREATE ROLE service_role NOLOGIN;
    END IF;
END
$$;

-- Conceder permissões das roles ao authenticator
GRANT anon TO authenticator;
GRANT service_role TO authenticator;

-- Permissões de schema
GRANT USAGE ON SCHEMA public TO anon;
GRANT USAGE ON SCHEMA public TO service_role;

-- Permissões de tabela (serão aplicadas após criação das tabelas)
-- Conceder no final do script

-- ============================================
-- TABELA: devices (Dispositivos ESP32)
-- ============================================
CREATE TABLE IF NOT EXISTS devices (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id VARCHAR(64) UNIQUE NOT NULL,  -- ID único do ESP32
    name VARCHAR(128) NOT NULL DEFAULT 'Câmara Fria',
    location VARCHAR(256),
    firmware_version VARCHAR(32) DEFAULT '1.0.0',
    is_online BOOLEAN DEFAULT false,
    last_seen TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now()
);

-- Dados do dispositivo de exemplo
INSERT INTO devices (device_id, name, location, is_online)
VALUES ('esp32-camara-fria-01', 'Câmara Fria Principal', 'Cozinha Industrial', true)
ON CONFLICT (device_id) DO NOTHING;

-- ============================================
-- TABELA: telemetry (Dados de telemetria)
-- ============================================
CREATE TABLE IF NOT EXISTS telemetry (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    
    -- Dados do controlador de temperatura
    temperature DECIMAL(5,2),          -- Temperatura atual (°C)
    setpoint DECIMAL(5,2),             -- Setpoint (°C)
    heater_output DECIMAL(5,2),        -- Saída do aquecedor (%)
    
    -- Dados do inversor de frequência
    inverter_frequency DECIMAL(6,2),   -- Frequência de saída (Hz)
    inverter_voltage DECIMAL(6,1),     -- Tensão de saída (V)
    inverter_current DECIMAL(6,2),     -- Corrente de saída (A)
    inverter_dc_voltage DECIMAL(6,1),  -- Tensão DC bus (V)
    inverter_temperature DECIMAL(5,1), -- Temperatura do inversor (°C)
    inverter_running BOOLEAN DEFAULT false,
    inverter_fault_code INTEGER DEFAULT 0,
    
    -- Dados da porta
    door_open BOOLEAN DEFAULT false,
    door_open_seconds INTEGER DEFAULT 0,
    door_stage INTEGER DEFAULT 0,      -- 0=fec, 1=norm, 2=aviso, 3=pulso, 4=cont
    
    -- Metadata
    timestamp TIMESTAMPTZ DEFAULT now(),
    raw_json JSONB                     -- Dados brutos para debug
);

-- Índices para queries rápidas
CREATE INDEX idx_telemetry_device_id ON telemetry(device_id);
CREATE INDEX idx_telemetry_timestamp ON telemetry(timestamp DESC);
CREATE INDEX idx_telemetry_temperature ON telemetry(temperature);
CREATE INDEX idx_telemetry_device_time ON telemetry(device_id, timestamp DESC);

-- ============================================
-- TABELA: telemetry_current (Última leitura por dispositivo)
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
-- TABELA: alerts (Histórico de alertas)
-- ============================================
CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    alert_type VARCHAR(32) NOT NULL,   -- 'temperature_high', 'temperature_critical', 'door_open', 'inverter_fault', 'communication_loss'
    severity VARCHAR(16) NOT NULL,     -- 'warning', 'critical', 'info'
    message TEXT NOT NULL,
    temperature DECIMAL(5,2),          -- Temperatura no momento do alerta
    metadata JSONB,                    -- Dados extras
    acknowledged BOOLEAN DEFAULT false,
    acknowledged_by VARCHAR(128),
    acknowledged_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX idx_alerts_device ON alerts(device_id, created_at DESC);
CREATE INDEX idx_alerts_type ON alerts(alert_type, created_at DESC);
CREATE INDEX idx_alerts_unack ON alerts(acknowledged, created_at DESC) WHERE NOT acknowledged;

-- ============================================
-- TABELA: alert_config (Configuração de alertas)
-- ============================================
CREATE TABLE IF NOT EXISTS alert_config (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id),
    alert_type VARCHAR(32) NOT NULL,
    threshold_value DECIMAL(10,2),
    enabled BOOLEAN DEFAULT true,
    cooldown_seconds INTEGER DEFAULT 300,  -- 5 min cooldown entre alertas
    whatsapp_enabled BOOLEAN DEFAULT true,
    whatsapp_numbers TEXT[] DEFAULT '{}',  -- Array de números para notificar
    message_template TEXT,
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now(),
    UNIQUE(device_id, alert_type)
);

-- Configurações padrão de alertas
INSERT INTO alert_config (device_id, alert_type, threshold_value, message_template, whatsapp_enabled)
VALUES
    ('esp32-camara-fria-01', 'temperature_high', 8.0,
     '⚠️ ALERTA Câmara Fria: Temperatura em {temperature}°C (máx: {threshold}°C). Setpoint: {setpoint}°C',
     true),
    ('esp32-camara-fria-01', 'temperature_critical', 12.0,
     '🔴 CRÍTICO Câmara Fria: Temperatura em {temperature}°C! Risco de deterioração dos alimentos. Verificar equipamento IMEDIATAMENTE.',
     true),
    ('esp32-camara-fria-01', 'door_open', 300,
     '🚪 ALERTA Câmara Fria: Porta aberta há {door_seconds} minutos! Fechar para manter temperatura.',
     true),
    ('esp32-camara-fria-01', 'inverter_fault', 0,
     '⚡ FALHA Inversor: Código de erro {fault_code}. Frequência: {frequency} Hz. Verificar equipamento.',
     true)
ON CONFLICT (device_id, alert_type) DO NOTHING;

-- ============================================
-- TABELA: whatsapp_log (Log de mensagens enviadas)
-- ============================================
CREATE TABLE IF NOT EXISTS whatsapp_log (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL,
    alert_id BIGINT REFERENCES alerts(id),
    phone_number VARCHAR(20) NOT NULL,
    message TEXT NOT NULL,
    status VARCHAR(16) DEFAULT 'pending',  -- pending, sent, failed
    evolution_response JSONB,
    sent_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX idx_whatsapp_log_device ON whatsapp_log(device_id, created_at DESC);

-- ============================================
-- FUNÇÃO: Atualizar telemetry_current
-- ============================================
CREATE OR REPLACE FUNCTION update_telemetry_current()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO telemetry_current (
        device_id, temperature, setpoint, heater_output,
        inverter_frequency, inverter_voltage, inverter_current,
        inverter_dc_voltage, inverter_temperature,
        inverter_running, inverter_fault_code,
        door_open, door_open_seconds, door_stage,
        updated_at
    ) VALUES (
        NEW.device_id, NEW.temperature, NEW.setpoint, NEW.heater_output,
        NEW.inverter_frequency, NEW.inverter_voltage, NEW.inverter_current,
        NEW.inverter_dc_voltage, NEW.inverter_temperature,
        NEW.inverter_running, NEW.inverter_fault_code,
        NEW.door_open, NEW.door_open_seconds, NEW.door_stage,
        now()
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

-- Trigger para atualizar dados atuais
-- SECURITY DEFINER permite que o trigger rode com privilégios do owner (postgres)
CREATE TRIGGER trigger_update_current
    AFTER INSERT ON telemetry
    FOR EACH ROW
    EXECUTE FUNCTION update_telemetry_current();

-- ============================================
-- FUNÇÃO: Auto-criar alertas baseado nos thresholds
-- ============================================
CREATE OR REPLACE FUNCTION check_and_create_alerts()
RETURNS TRIGGER AS $$
DECLARE
    config_row RECORD;
    last_alert RECORD;
BEGIN
    -- Verifica cada configuração de alerta para o dispositivo
    FOR config_row IN 
        SELECT * FROM alert_config 
        WHERE device_id = NEW.device_id AND enabled = true
    LOOP
        -- Verifica se o último alerta do mesmo tipo está dentro do cooldown
        SELECT INTO last_alert
            created_at FROM alerts
        WHERE device_id = NEW.device_id 
            AND alert_type = config_row.alert_type
        ORDER BY created_at DESC
        LIMIT 1;
        
        IF last_alert IS NOT NULL AND 
           (EXTRACT(EPOCH FROM (now() - last_alert.created_at)) < config_row.cooldown_seconds) THEN
            CONTINUE;  -- Ainda no cooldown, pula
        END IF;
        
        -- Alerta de temperatura alta
        IF config_row.alert_type = 'temperature_high' AND 
           NEW.temperature >= config_row.threshold_value AND
           NEW.temperature < (SELECT threshold_value FROM alert_config 
                             WHERE device_id = NEW.device_id 
                             AND alert_type = 'temperature_critical') THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, temperature, metadata)
            VALUES (
                NEW.device_id,
                'temperature_high',
                'warning',
                replace(replace(replace(config_row.message_template,
                    '{temperature}', NEW.temperature::text),
                    '{threshold}', config_row.threshold_value::text),
                    '{setpoint}', NEW.setpoint::text),
                NEW.temperature,
                jsonb_build_object('temperature', NEW.temperature, 'setpoint', NEW.setpoint)
            );
        END IF;
        
        -- Alerta de temperatura crítica
        IF config_row.alert_type = 'temperature_critical' AND 
           NEW.temperature >= config_row.threshold_value THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, temperature, metadata)
            VALUES (
                NEW.device_id,
                'temperature_critical',
                'critical',
                replace(replace(replace(config_row.message_template,
                    '{temperature}', NEW.temperature::text),
                    '{threshold}', config_row.threshold_value::text),
                    '{setpoint}', NEW.setpoint::text),
                NEW.temperature,
                jsonb_build_object('temperature', NEW.temperature, 'setpoint', NEW.setpoint)
            );
        END IF;
        
        -- Alerta de porta aberta
        IF config_row.alert_type = 'door_open' AND 
           NEW.door_open = true AND
           NEW.door_open_seconds >= config_row.threshold_value THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, metadata)
            VALUES (
                NEW.device_id,
                'door_open',
                'warning',
                replace(config_row.message_template,
                    '{door_seconds}', (NEW.door_open_seconds / 60)::text),
                jsonb_build_object('door_open_seconds', NEW.door_open_seconds)
            );
        END IF;
        
        -- Alerta de falha no inversor
        IF config_row.alert_type = 'inverter_fault' AND 
           NEW.inverter_fault_code > 0 THEN
            INSERT INTO alerts (device_id, alert_type, severity, message, metadata)
            VALUES (
                NEW.device_id,
                'inverter_fault',
                'critical',
                replace(replace(config_row.message_template,
                    '{fault_code}', NEW.inverter_fault_code::text),
                    '{frequency}', NEW.inverter_frequency::text),
                jsonb_build_object('fault_code', NEW.inverter_fault_code)
            );
        END IF;
    END LOOP;
    
    RETURN NEW;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- Trigger para auto-alertas
-- SECURITY DEFINER permite que o trigger rode com privilégios do owner (postgres)
CREATE TRIGGER trigger_check_alerts
    AFTER INSERT ON telemetry
    FOR EACH ROW
    EXECUTE FUNCTION check_and_create_alerts();

-- ============================================
-- FUNÇÃO: Atualizar updated_at automaticamente
-- ============================================
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

-- Policy: Anon pode ler dados atuais
CREATE POLICY "Allow anon read telemetry_current"
    ON telemetry_current FOR SELECT
    TO anon
    USING (true);

-- Policy: Anon pode ler dados de telemetria
CREATE POLICY "Allow anon read telemetry"
    ON telemetry FOR SELECT
    TO anon
    USING (true);

-- Policy: Service role pode inserir dados (ESP32)
CREATE POLICY "Allow service role insert telemetry"
    ON telemetry FOR INSERT
    TO service_role
    WITH CHECK (true);

-- Policy: Anon pode ler alertas
CREATE POLICY "Allow anon read alerts"
    ON alerts FOR SELECT
    TO anon
    USING (true);

-- Policy: Service role pode inserir alertas
CREATE POLICY "Allow service role insert alerts"
    ON alerts FOR INSERT
    TO service_role
    WITH CHECK (true);

-- Policy: Anon pode ler config de alertas
CREATE POLICY "Allow anon read alert_config"
    ON alert_config FOR SELECT
    TO anon
    USING (true);

-- Policy: Anon pode ler dispositivos
CREATE POLICY "Allow anon read devices"
    ON devices FOR SELECT
    TO anon
    USING (true);

-- Policy: Service role pode atualizar dispositivos
CREATE POLICY "Allow service role update devices"
    ON devices FOR UPDATE
    TO service_role
    USING (true);

-- Policy: Service role pode inserir log whatsapp
CREATE POLICY "Allow service role insert whatsapp_log"
    ON whatsapp_log FOR INSERT
    TO service_role
    WITH CHECK (true);

-- Policy: Anon pode ler log whatsapp
CREATE POLICY "Allow anon read whatsapp_log"
    ON whatsapp_log FOR SELECT
    TO anon
    USING (true);

-- ============================================
-- VIEWS úteis
-- ============================================

-- View: Último status por dispositivo
CREATE OR REPLACE VIEW v_current_status AS
SELECT 
    d.device_id,
    d.name as device_name,
    d.location,
    d.is_online,
    d.last_seen,
    tc.temperature,
    tc.setpoint,
    tc.heater_output,
    tc.inverter_frequency,
    tc.inverter_voltage,
    tc.inverter_current,
    tc.inverter_running,
    tc.inverter_fault_code,
    tc.door_open,
    tc.door_open_seconds,
    tc.door_stage,
    tc.updated_at
FROM devices d
LEFT JOIN telemetry_current tc ON d.device_id = tc.device_id;

-- View: Últimos 5 alertas não reconhecidos
CREATE OR REPLACE VIEW v_pending_alerts AS
SELECT 
    a.*,
    d.name as device_name
FROM alerts a
JOIN devices d ON a.device_id = d.device_id
WHERE NOT a.acknowledged
ORDER BY a.created_at DESC
LIMIT 5;

-- View: Histórico de temperatura (últimas 24h em intervalos de 5min)
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

-- ============================================
-- GRANTS (permissões de tabela para roles)
-- ============================================
-- Permissões para anon (leitura e escrita para ESP32/API)
GRANT ALL ON ALL TABLES IN SCHEMA public TO anon;
GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO anon;
GRANT ALL ON ALL FUNCTIONS IN SCHEMA public TO anon;

-- Permissões para service_role (leitura e escrita completa)
GRANT ALL ON ALL TABLES IN SCHEMA public TO service_role;
GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO service_role;
GRANT ALL ON ALL FUNCTIONS IN SCHEMA public TO service_role;

-- Permissões para o owner (postgres) das funções SECURITY DEFINER
GRANT ALL ON ALL TABLES IN SCHEMA public TO postgres;
GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO postgres;
