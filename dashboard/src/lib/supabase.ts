import { createClient } from '@supabase/supabase-js'

// Supabase Cloud credentials
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL || 'https://hnfdlpjkoxizrgukjcrw.supabase.co'
const supabaseKey = import.meta.env.VITE_SUPABASE_ANON_KEY || 'sb_publishable_DReGLMwglDMCB5DYoPcQwQ_3_gVWFui'

export const supabase = createClient(supabaseUrl, supabaseKey)

// Edge Function URL
const FUNCTIONS_URL = `${supabaseUrl}/functions/v1`

export async function fetchLatestTelemetry() {
  // Use the v_current_status view for consolidated data
  const { data, error } = await supabase
    .from('v_current_status')
    .select('*')
    .single()
  
  if (error) {
    console.error('Error fetching telemetry:', error)
    // Fallback to telemetry table
    const { data: fallback } = await supabase
      .from('telemetry')
      .select('*')
      .order('timestamp', { ascending: false })
      .limit(1)
      .single()
    return fallback
  }
  return data
}

export async function fetchTemperatureHistory(hours: number = 12) {
  // Use the v_temperature_history view
  const { data, error } = await supabase
    .from('v_temperature_history')
    .select('temperature, setpoint, humidity, timestamp')
    .gte('timestamp', new Date(Date.now() - hours * 60 * 60 * 1000).toISOString())
    .order('timestamp', { ascending: true })
    .limit(200)
  
  if (error) {
    console.error('Error fetching history:', error)
    // Fallback to telemetry table
    const { data: fallback } = await supabase
      .from('telemetry')
      .select('temperature, setpoint, humidity, timestamp')
      .gte('timestamp', new Date(Date.now() - hours * 60 * 60 * 1000).toISOString())
      .order('timestamp', { ascending: true })
      .limit(200)
    return fallback || []
  }
  return data || []
}

export async function fetchAlerts() {
  // Use the v_active_alerts view
  const { data, error } = await supabase
    .from('v_active_alerts')
    .select('*')
    .order('created_at', { ascending: false })
    .limit(10)
  
  if (error) {
    console.error('Error fetching alerts:', error)
    // Fallback to alerts table
    const { data: fallback } = await supabase
      .from('alerts')
      .select('*')
      .eq('acknowledged', false)
      .order('created_at', { ascending: false })
      .limit(10)
    return fallback || []
  }
  return data || []
}

export async function fetchDevices() {
  const { data, error } = await supabase
    .from('devices')
    .select('*')
  
  if (error) {
    console.error('Error fetching devices:', error)
    return []
  }
  return data || []
}

// Call Edge Function to check alerts (server-side logic)
export async function checkAlerts() {
  try {
    const response = await fetch(`${FUNCTIONS_URL}/check-alerts`, {
      headers: {
        'apikey': supabaseKey,
        'Authorization': `Bearer ${supabaseKey}`
      }
    })
    return await response.json()
  } catch (error) {
    console.error('Error calling check-alerts:', error)
    return null
  }
}

// Call Edge Function to get consolidated status
export async function getStatus() {
  try {
    const response = await fetch(`${FUNCTIONS_URL}/get-status`, {
      headers: {
        'apikey': supabaseKey,
        'Authorization': `Bearer ${supabaseKey}`
      }
    })
    return await response.json()
  } catch (error) {
    console.error('Error calling get-status:', error)
    return null
  }
}

export function subscribeToTelemetry(callback: (payload: any) => void) {
  return supabase
    .channel('telemetry-changes')
    .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'telemetry' }, callback)
    .subscribe()
}
