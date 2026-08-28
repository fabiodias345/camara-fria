export type Severity = 'normal' | 'warning' | 'critical'

export type Telemetry = {
  temperature: number
  humidity: number
  setpoint: number
  door: 'closed' | 'open'
  doorMinutes: number
  tc300Online: boolean
  buzzer: boolean
  lastSeen: string
}

export type Alarm = { severity: Severity; title: string; detail: string; time: string }