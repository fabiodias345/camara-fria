import { createClient } from '@supabase/supabase-js'

// Supabase Cloud credentials
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL || 'https://hnfdlpjkoxizrgukjcrw.supabase.co'
const supabaseKey = import.meta.env.VITE_SUPABASE_ANON_KEY || 'sb_publishable_DReGLMwglDMCB5DYoPcQwQ_3_gVWFui'

export const supabase = createClient(supabaseUrl, supabaseKey)

export async function fetchLatestTelemetry() {
  const { data, error } = await supabase
    .from('telemetry_current')
    .select('*')
    .single()
  
  if (error) {
    console.error('Error fetching telemetry:', error)
    return null
  }
  return data
}

export async function fetchTemperatureHistory(hours: number = 12) {
  const { data, error } = await supabase
    .from('telemetry')
    .select('temperature, setpoint, timestamp')
    .gte('timestamp', new Date(Date.now() - hours * 60 * 60 * 1000).toISOString())
    .order('timestamp', { ascending: true })
    .limit(200)
  
  if (error) {
    console.error('Error fetching history:', error)
    return []
  }
  return data
}

export async function fetchAlerts() {
  const { data, error } = await supabase
    .from('alerts')
    .select('*')
    .order('created_at', { ascending: false })
    .limit(10)
  
  if (error) {
    console.error('Error fetching alerts:', error)
    return []
  }
  return data
}

export async function fetchDevices() {
  const { data, error } = await supabase
    .from('devices')
    .select('*')
  
  if (error) {
    console.error('Error fetching devices:', error)
    return []
  }
  return data
}

export function subscribeToTelemetry(callback: (payload: any) => void) {
  return supabase
    .channel('telemetry-changes')
    .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'telemetry' }, callback)
    .subscribe()
}
