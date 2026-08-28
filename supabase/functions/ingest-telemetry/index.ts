import { createClient } from 'https://esm.sh/@supabase/supabase-js@2.57.4'

Deno.serve(async (request) => {
  if (request.method !== 'POST') return new Response('Method Not Allowed', { status: 405 })
  const deviceToken = request.headers.get('x-device-token')
  if (!deviceToken || deviceToken !== Deno.env.get('CAMARA_DEVICE_INGEST_TOKEN')) return Response.json({ error: 'invalid device token' }, { status: 401 })
  const body = await request.json()
  if (typeof body.device_id !== 'string') return Response.json({ error: 'device_id is required' }, { status: 400 })
  const client = createClient(Deno.env.get('SUPABASE_URL')!, Deno.env.get('SUPABASE_SERVICE_ROLE_KEY')!)
  const { data, error } = await client.from('telemetry').insert({ device_id: body.device_id, temperature_c: body.temperature_c, humidity_pct: body.humidity_pct, setpoint_c: body.setpoint_c, voltage_v: body.voltage_v, current_a: body.current_a, frequency_hz: body.frequency_hz, door_open: body.door_open, buzzer_on: body.buzzer_on, raw_payload: body }).select('id, recorded_at').single()
  if (error) return Response.json({ error: error.message }, { status: 500 })
  return Response.json(data, { status: 201 })
})
