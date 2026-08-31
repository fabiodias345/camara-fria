// ============================================
// CÂMARA FRIA - Edge Function: Send WhatsApp
// ============================================
// Função utilitária para envio direto de mensagens
// WhatsApp via Evolution API.
// ============================================

import { serve } from "https://deno.land/std@0.208.0/http/server.ts";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers":
    "authorization, x-client-info, apikey, content-type",
};

serve(async (req: Request) => {
  if (req.method === "OPTIONS") {
    return new Response("ok", { headers: corsHeaders });
  }

  try {
    const { number, message } = await req.json();

    if (!number || !message) {
      return new Response(
        JSON.stringify({ error: "number and message are required" }),
        { status: 400, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    const evolutionUrl = Deno.env.get("EVOLUTION_API_URL");
    const evolutionKey = Deno.env.get("EVOLUTION_API_KEY");
    const whatsappInstance = Deno.env.get("WHATSAPP_INSTANCE");

    if (!evolutionUrl || !evolutionKey || !whatsappInstance) {
      return new Response(
        JSON.stringify({ error: "Evolution API not configured" }),
        { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    const cleanNumber = number.replace(/\D/g, "");

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

    return new Response(
      JSON.stringify({
        success: response.ok,
        data: result,
      }),
      {
        status: response.ok ? 200 : 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    return new Response(
      JSON.stringify({ error: "Internal server error", details: String(error) }),
      {
        status: 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  }
});
