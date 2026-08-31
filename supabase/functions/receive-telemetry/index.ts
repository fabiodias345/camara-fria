// ============================================
// CÂMARA FRIA - Edge Function: Receive Telemetry
// ============================================
// Recebe dados de telemetria do ESP32 via HTTP POST
// e armazena no banco de dados Supabase.
// ============================================

import { serve } from "https://deno.land/std@0.208.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2.39.0";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers":
    "authorization, x-client-info, apikey, content-type",
};

interface TelemetryData {
  device_id: string;
  temperature: number;
  setpoint: number;
  heater_output: number;
  inverter_frequency: number;
  inverter_voltage: number;
  inverter_current: number;
  inverter_dc_voltage: number;
  inverter_temperature: number;
  inverter_running: boolean;
  inverter_fault_code: number;
  door_open: boolean;
  door_open_seconds: number;
  door_stage: number;
  timestamp: number;
}

serve(async (req: Request) => {
  // Handle CORS preflight
  if (req.method === "OPTIONS") {
    return new Response("ok", { headers: corsHeaders });
  }

  try {
    // Create Supabase client with service role for full access
    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
    
    const supabase = createClient(supabaseUrl, supabaseKey);

    // Parse request body
    const data: TelemetryData = await req.json();
    
    console.log(`[RECEIVE] Dados recebidos de ${data.device_id}:`);
    console.log(`  Temp: ${data.temperature}°C | Setpoint: ${data.setpoint}°C`);
    console.log(`  Inv: ${data.inverter_frequency}Hz | Porta: ${data.door_open ? "ABERTA" : "FECHADA"}`);

    // Validate required fields
    if (!data.device_id) {
      return new Response(
        JSON.stringify({ error: "device_id is required" }),
        { status: 400, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    // Upsert device if not exists
    const { error: deviceError } = await supabase
      .from("devices")
      .upsert(
        {
          device_id: data.device_id,
          is_online: true,
          last_seen: new Date().toISOString(),
        },
        { onConflict: "device_id" }
      );

    if (deviceError) {
      console.error("[RECEIVE] Erro ao atualizar device:", deviceError);
    }

    // Insert telemetry data
    const { data: telemetryRecord, error: telemetryError } = await supabase
      .from("telemetry")
      .insert({
        device_id: data.device_id,
        temperature: data.temperature,
        setpoint: data.setpoint,
        heater_output: data.heater_output,
        inverter_frequency: data.inverter_frequency,
        inverter_voltage: data.inverter_voltage,
        inverter_current: data.inverter_current,
        inverter_dc_voltage: data.inverter_dc_voltage,
        inverter_temperature: data.inverter_temperature,
        inverter_running: data.inverter_running,
        inverter_fault_code: data.inverter_fault_code,
        door_open: data.door_open,
        door_open_seconds: data.door_open_seconds,
        door_stage: data.door_stage,
        timestamp: new Date().toISOString(),
        raw_json: data as unknown as Record<string, unknown>,
      })
      .select("id")
      .single();

    if (telemetryError) {
      console.error("[RECEIVE] Erro ao inserir telemetria:", telemetryError);
      return new Response(
        JSON.stringify({ error: "Failed to store telemetry", details: telemetryError }),
        { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    console.log(`[RECEIVE] Telemetria salva com ID: ${telemetryRecord?.id}`);

    // Check for new alerts that were auto-created by triggers
    const { data: newAlerts } = await supabase
      .from("alerts")
      .select("id, alert_type, severity, message")
      .eq("device_id", data.device_id)
      .eq("acknowledged", false)
      .order("created_at", { ascending: false })
      .limit(5);

    // Return success with any new alerts
    return new Response(
      JSON.stringify({
        success: true,
        telemetry_id: telemetryRecord?.id,
        new_alerts: newAlerts || [],
      }),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    console.error("[RECEIVE] Erro geral:", error);
    return new Response(
      JSON.stringify({ error: "Internal server error" }),
      {
        status: 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  }
});
