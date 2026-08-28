import { createClient } from '@supabase/supabase-js'
import type { Telemetry } from '../types'

const url = import.meta.env.VITE_SUPABASE_URL as string | undefined
const key = import.meta.env.VITE_SUPABASE_ANON_KEY as string | undefined
export const supabase = url && key ? createClient(url, key) : null

export async function loadLatestTelemetry(): Promise<Partial<Telemetry> | null> {
  if (!supabase) return null
  const { data } = await supabase.from('telemetry').select('*').order('recorded_at', { ascending: false }).limit(1).maybeSingle()
  if (!data) return null
  return { temperature: Number(data.temperature_c), humidity: Number(data.humidity_pct), setpoint: Number(data.setpoint_c), door: data.door_open ? 'open' : 'closed', buzzer: Boolean(data.buzzer_on), lastSeen: new Date(data.recorded_at).toLocaleTimeString('pt-BR') }
}
