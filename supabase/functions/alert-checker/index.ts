// ============================================
// CÂMARA FRIA - Edge Function: Alert Checker
// ============================================
// Verifica alertas pendentes e envia notificações
// via WhatsApp usando a Evolution API.
// Pode ser chamada como cron job ou via webhook.
// ============================================

import { serve } from "https://deno.land/std@0.208.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2.39.0";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers":
    "authorization, x-client-info, apikey, content-type",
};

interface Alert {
  id: number;
  device_id: string;
  alert_type: string;
  severity: string;
  message: string;
  temperature: number | null;
  metadata: Record<string, unknown> | null;
  created_at: string;
}

interface AlertConfig {
  alert_type: string;
  whatsapp_enabled: boolean;
  whatsapp_numbers: string[];
}

// ============================================
// ENVIO DE MENSAGEM WHATSAPP
// ============================================
async function sendWhatsAppMessage(
  phone: string,
  message: string
): Promise<{ success: boolean; response?: unknown; error?: string }> {
  const evolutionUrl = Deno.env.get("EVOLUTION_API_URL");
  const evolutionKey = Deno.env.get("EVOLUTION_API_KEY");
  const whatsappInstance = Deno.env.get("WHATSAPP_INSTANCE");

  if (!evolutionUrl || !evolutionKey || !whatsappInstance) {
    return { success: false, error: "Evolution API not configured" };
  }

  try {
    // Formata o número (remove caracteres especiais)
    const cleanNumber = phone.replace(/\D/g, "");

    const response = await fetch(
      `${evolutionUrl}/message/sendText/${whatsappInstance}`,
      {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          apikey: evolutionKey,
        },
        body: JSON.stringify({
          number: cleanNumber,
          text: message,
        }),
      }
    );

    const result = await response.json();

    if (response.ok) {
      console.log(`[WHATSAPP] Mensagem enviada para ${cleanNumber}`);
      return { success: true, response: result };
    } else {
      console.error(`[WHATSAPP] Erro: ${JSON.stringify(result)}`);
      return { success: false, error: JSON.stringify(result) };
    }
  } catch (error) {
    console.error(`[WHATSAPP] Exceção: ${error}`);
    return { success: false, error: String(error) };
  }
}

// ============================================
// GERAÇÃO DE MENSAGEM COM AI
// ============================================
function generateAlertMessage(alert: Alert): string {
  const timestamp = new Date(alert.created_at).toLocaleString("pt-BR", {
    timeZone: "America/Sao_Paulo",
  });

  let emoji = "⚠️";
  let urgency = "ALERTA";

  if (alert.severity === "critical") {
    emoji = "🔴";
    urgency = "CRÍTICO";
  } else if (alert.severity === "info") {
    emoji = "ℹ️";
    urgency = "INFORMAÇÃO";
  }

  // AI-enhanced message based on alert type
  let aiInsight = "";

  switch (alert.alert_type) {
    case "temperature_high":
      aiInsight =
        "\n\n🤖 *Análise AI:* A temperatura está acima do ideal para câmara fria. " +
        "Verifique se o compressor está funcionando corretamente e se a porta está bem fechada. " +
        "Se a temperatura continuar subindo, há risco de deterioração dos alimentos.";
      break;
    case "temperature_critical":
      aiInsight =
        "\n\n🤖 *Análise AI:* TEMPERATURA EM NÍVEL CRÍTICO! " +
        "Risco iminente de perda de carga. Ação imediata necessária. " +
        "Verifique: 1) Compressor 2) Gas refrigerante 3) Isolamento da câmara " +
        "4) Porta fechada. Considere transferir produtos perecíveis se a temperatura não normalizar em 15 minutos.";
      break;
    case "door_open":
      const doorMinutes = alert.metadata?.door_open_seconds
        ? Math.floor(Number(alert.metadata.door_open_seconds) / 60)
        : "?";
      aiInsight =
        `\n\n🤖 *Análise AI:* Porta aberta há ${doorMinutes} minutos. ` +
        "Cada minuto com porta aberta pode elevar a temperatura em 0.5-1°C. " +
        "Fechar a porta imediatamente para evitar desperdício energético e risco à carga.";
      break;
    case "inverter_fault":
      aiInsight =
        "\n\n🤖 *Análise AI:* O inversor de frequência reportou uma falha. " +
        "Isso pode afetar o funcionamento do compressor. " +
        "Verifique o código de erro e consulte o manual do equipamento.";
      break;
    default:
      aiInsight = "";
  }

  return (
    `${emoji} *${urgency} - Câmara Fria*\n\n` +
    `${alert.message}\n\n` +
    `📍 Dispositivo: ${alert.device_id}\n` +
    `🕐 Horário: ${timestamp}` +
    aiInsight
  );
}

// ============================================
// HANDLER PRINCIPAL
// ============================================
serve(async (req: Request) => {
  // Handle CORS preflight
  if (req.method === "OPTIONS") {
    return new Response("ok", { headers: corsHeaders });
  }

  try {
    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
    const supabase = createClient(supabaseUrl, supabaseKey);

    console.log("[CHECKER] Iniciando verificação de alertas...");

    // Busca alertas não reconhecidos
    const { data: alerts, error: alertError } = await supabase
      .from("alerts")
      .select("*")
      .eq("acknowledged", false)
      .order("created_at", { ascending: false })
      .limit(10);

    if (alertError) {
      console.error("[CHECKER] Erro ao buscar alertas:", alertError);
      return new Response(
        JSON.stringify({ error: "Failed to fetch alerts" }),
        { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    if (!alerts || alerts.length === 0) {
      console.log("[CHECKER] Nenhum alerta pendente");
      return new Response(
        JSON.stringify({ success: true, message: "No pending alerts", sent: 0 }),
        { status: 200, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    console.log(`[CHECKER] ${alerts.length} alertas pendentes encontrados`);

    let sentCount = 0;
    const results = [];

    for (const alert of alerts) {
      // Busca configuração de WhatsApp para este tipo de alerta
      const { data: config } = await supabase
        .from("alert_config")
        .select("*")
        .eq("device_id", alert.device_id)
        .eq("alert_type", alert.alert_type)
        .eq("enabled", true)
        .single();

      if (!config || !config.whatsapp_enabled) {
        console.log(`[CHECKER] WhatsApp desabilitado para ${alert.alert_type}`);
        continue;
      }

      // Verifica se já existe log de envio para este alerta
      const { data: existingLog } = await supabase
        .from("whatsapp_log")
        .select("id")
        .eq("alert_id", alert.id)
        .eq("status", "sent")
        .single();

      if (existingLog) {
        console.log(`[CHECKER] Alerta ${alert.id} já notificado`);
        continue;
      }

      // Gera a mensagem com AI
      const message = generateAlertMessage(alert);

      // Envia para cada número configurado
      const numbers = config.whatsapp_numbers?.length
        ? config.whatsapp_numbers
        : [Deno.env.get("WHATSAPP_NOTIFY_NUMBER") || ""];

      for (const number of numbers) {
        if (!number) continue;

        const result = await sendWhatsAppMessage(number, message);

        // Registra o log
        await supabase.from("whatsapp_log").insert({
          device_id: alert.device_id,
          alert_id: alert.id,
          phone_number: number,
          message: message,
          status: result.success ? "sent" : "failed",
          evolution_response: result.response || result.error,
          sent_at: result.success ? new Date().toISOString() : null,
        });

        if (result.success) {
          sentCount++;
        }

        results.push({
          alert_id: alert.id,
          alert_type: alert.alert_type,
          phone: number,
          status: result.success ? "sent" : "failed",
        });
      }
    }

    console.log(`[CHECKER] ${sentCount} mensagens enviadas`);

    return new Response(
      JSON.stringify({
        success: true,
        alerts_processed: alerts.length,
        messages_sent: sentCount,
        results,
      }),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    console.error("[CHECKER] Erro geral:", error);
    return new Response(
      JSON.stringify({ error: "Internal server error" }),
      {
        status: 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  }
});
