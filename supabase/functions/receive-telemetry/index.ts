// ============================================
// receive-telemetry Edge Function
// Recebe dados do ESP32 e salva no Supabase
// ============================================
import { serve } from "https://deno.land/std@0.177.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers":
    "authorization, x-client-info, apikey, content-type",
};

interface TelemetryData {
  device_id: string;
  temperature: number;
  setpoint?: number;
  humidity?: number;
  heater_output?: number;
  inverter_frequency?: number;
  inverter_voltage?: number;
  inverter_current?: number;
  inverter_dc_voltage?: number;
  inverter_temperature?: number;
  inverter_running?: boolean;
  inverter_fault_code?: number;
  door_open?: boolean;
  door_open_seconds?: number;
  door_stage?: number;
  buzzer_active?: boolean;
}

serve(async (req: Request) => {
  if (req.method === "OPTIONS") {
    return new Response("ok", { headers: corsHeaders });
  }

  try {
    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
    const supabase = createClient(supabaseUrl, supabaseKey);

    if (req.method !== "POST") {
      return new Response(
        JSON.stringify({ error: "Method not allowed" }),
        {
          status: 405,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    const body: TelemetryData = await req.json();

    if (!body.device_id || body.temperature === undefined) {
      return new Response(
        JSON.stringify({
          error: "Missing required fields: device_id, temperature",
        }),
        {
          status: 400,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Insert telemetry record
    const { data: telemetry, error: telemetryError } = await supabase
      .from("telemetry")
      .insert({
        device_id: body.device_id,
        temperature: body.temperature,
        setpoint: body.setpoint ?? -18.0,
        heater_output: body.heater_output ?? 45.0,
        inverter_frequency: body.inverter_frequency ?? null,
        inverter_voltage: body.inverter_voltage ?? null,
        inverter_current: body.inverter_current ?? null,
        inverter_dc_voltage: body.inverter_dc_voltage ?? null,
        inverter_temperature: body.inverter_temperature ?? null,
        inverter_running: body.inverter_running ?? true,
        inverter_fault_code: body.inverter_fault_code ?? 0,
        humidity: body.humidity ?? null,
        door_open: body.door_open ?? false,
        door_open_seconds: body.door_open_seconds ?? 0,
        door_stage: body.door_stage ?? 0,
        buzzer_active: body.buzzer_active ?? false,
      })
      .select()
      .single();

    if (telemetryError) {
      console.error("Telemetry insert error:", telemetryError);
      return new Response(
        JSON.stringify({ error: telemetryError.message }),
        {
          status: 500,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Check for alerts
    const alerts: Array<{
      device_id: string;
      alert_type: string;
      message: string;
      severity: string;
    }> = [];

    // Temperature alert
    if (body.temperature > -8.0) {
      alerts.push({
        device_id: body.device_id,
        alert_type: "high_temperature",
        message: `Temperatura alta: ${body.temperature.toFixed(1)}°C (limite: -8.0°C)`,
        severity: body.temperature > 0 ? "critical" : "warning",
      });
    }

    // Door open alert
    if (body.door_open_seconds && body.door_open_seconds >= 300) {
      const minutes = Math.floor(body.door_open_seconds / 60);
      alerts.push({
        device_id: body.device_id,
        alert_type: "door_open",
        message: `Porta aberta ha ${minutes} minutos`,
        severity: body.door_open_seconds >= 420 ? "critical" : "warning",
      });
    }

    // Inverter fault
    if (
      body.inverter_frequency !== undefined &&
      body.inverter_frequency === 0 &&
      body.temperature > -15.0
    ) {
      alerts.push({
        device_id: body.device_id,
        alert_type: "inverter_fault",
        message: `Inversor parado! Temperatura subindo: ${body.temperature.toFixed(1)}°C`,
        severity: "critical",
      });
    }

    // Insert alerts (deduplicate: only unacknowledged alerts per type per device)
    for (const alert of alerts) {
      const { data: existing } = await supabase
        .from("alerts")
        .select("id")
        .eq("device_id", alert.device_id)
        .eq("alert_type", alert.alert_type)
        .eq("acknowledged", false)
        .limit(1);

      if (!existing || existing.length === 0) {
        await supabase.from("alerts").insert(alert);
      }
    }

    // Acknowledge alerts that are no longer active
    if (body.temperature <= -8.0) {
      await supabase
        .from("alerts")
        .update({
          acknowledged: true,
          acknowledged_at: new Date().toISOString(),
          acknowledged_by: "system",
        })
        .eq("device_id", body.device_id)
        .eq("alert_type", "high_temperature")
        .eq("acknowledged", false);
    }

    if (!body.door_open_seconds || body.door_open_seconds === 0) {
      await supabase
        .from("alerts")
        .update({
          acknowledged: true,
          acknowledged_at: new Date().toISOString(),
          acknowledged_by: "system",
        })
        .eq("device_id", body.device_id)
        .eq("alert_type", "door_open")
        .eq("acknowledged", false);
    }

    if (body.inverter_frequency && body.inverter_frequency > 0) {
      await supabase
        .from("alerts")
        .update({
          acknowledged: true,
          acknowledged_at: new Date().toISOString(),
          acknowledged_by: "system",
        })
        .eq("device_id", body.device_id)
        .eq("alert_type", "inverter_fault")
        .eq("acknowledged", false);
    }

    return new Response(
      JSON.stringify({
        success: true,
        telemetry_id: telemetry?.id,
        alerts_created: alerts.length,
      }),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    console.error("Unexpected error:", error);
    return new Response(
      JSON.stringify({ error: "Internal server error" }),
      {
        status: 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  }
});
