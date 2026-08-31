// ============================================
// get-status Edge Function
// Retorna dados consolidados para o dashboard
// ============================================
import { serve } from "https://deno.land/std@0.177.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

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
    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
    const supabase = createClient(supabaseUrl, supabaseKey);

    // Get current status
    const { data: current } = await supabase
      .from("telemetry_current")
      .select("*")
      .single();

    // Get active (unacknowledged) alerts count
    const { count: alertCount } = await supabase
      .from("alerts")
      .select("*", { count: "exact", head: true })
      .eq("acknowledged", false);

    // Get last 50 telemetry points for chart
    const { data: history } = await supabase
      .from("telemetry")
      .select("temperature, setpoint, timestamp")
      .order("timestamp", { ascending: false })
      .limit(50);

    // Get recent alerts
    const { data: recentAlerts } = await supabase
      .from("alerts")
      .select("*")
      .order("created_at", { ascending: false })
      .limit(10);

    return new Response(
      JSON.stringify({
        current: current || null,
        alert_count: alertCount || 0,
        history: history ? history.reverse() : [],
        recent_alerts: recentAlerts || [],
      }),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  } catch (error) {
    console.error("Error:", error);
    return new Response(
      JSON.stringify({ error: "Internal server error" }),
      {
        status: 500,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );
  }
});
