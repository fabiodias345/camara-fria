export type Severity = 'normal' | 'warning' | 'critical'

export type Telemetry = {
  temperature: number
  humidity: number
  setpoint: number
  voltage: number
  current: number
  frequency: number
  door: 'closed' | 'open'
  doorMinutes: number
  inverterOnline: boolean
  tc300Online: boolean
  buzzer: boolean
  lastSeen: string
}

export type Alarm = { severity: Severity; title: string; detail: string; time: string }
