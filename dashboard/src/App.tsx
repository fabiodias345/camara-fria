import { useEffect, useState, useCallback } from 'react'
import { Activity, Bell, Check, ChevronRight, CloudOff, DoorClosed, Fan, Gauge, LockKeyhole, Radio, Snowflake, Thermometer, Waves, Zap, type LucideIcon } from 'lucide-react'
import { Area, AreaChart, CartesianGrid, Line, PolarAngleAxis, RadialBar, RadialBarChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts'
import type { Telemetry, Alarm } from './types'
import { fetchLatestTelemetry, fetchTemperatureHistory, fetchAlerts, subscribeToTelemetry } from './lib/supabase'

const initial: Telemetry = { temperature: -18.4, humidity: 45, setpoint: -18, voltage: 380, current: 12.8, frequency: 49.8, door: 'closed', doorMinutes: 0, inverterOnline: true, tc300Online: true, buzzer: false, lastSeen: 'agora' }

function Dial({ value, min, max, label, unit, color, large = false }: { value: number; min: number; max: number; label: string; unit: string; color: string; large?: boolean }) {
  const percent = Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100))
  return <div className={`dial ${large ? 'dial-large' : ''}`}><ResponsiveContainer width="100%" height="100%"><RadialBarChart innerRadius={large ? '75%' : '65%'} outerRadius="94%" startAngle={220} endAngle={-40} data={[{ value: percent, fill: color }]} barSize={large ? 14 : 9}><PolarAngleAxis type="number" domain={[0, 100]} tick={false} /><RadialBar dataKey="value" background={{ fill: '#193442' }} cornerRadius={10} /></RadialBarChart></ResponsiveContainer><div className="dial-text"><strong>{value}</strong><span>{unit}</span><small>{label}</small></div></div>
}
function Kpi({ icon: Icon, label, value, unit, color }: { icon: LucideIcon; label: string; value: number; unit: string; color: string }) { return <div className={`kpi kpi-${color}`}><div><span>{label}</span><Icon size={17} /></div><strong>{value}<small>{unit}</small></strong></div> }
function Device({ name, detail, online, icon: Icon }: { name: string; detail: string; online: boolean; icon: LucideIcon }) { return <div className="device-row"><span className={`device-check ${online ? '' : 'bad'}`}>{online ? <Check size={12} /> : <CloudOff size={12} />}</span><div className="device-icon"><Icon size={22} /></div><div className="device-copy"><b>{name}</b><p>{detail}</p></div><span className={`online-label ${online ? '' : 'offline'}`}>{online ? 'ONLINE' : 'OFFLINE'}</span></div> }

export function App() {
  const [data, setData] = useState(initial)
  const [history, setHistory] = useState(() => Array.from({ length: 13 }, (_, i) => ({ t: `${i + 9}h`, temp: [-19.2, -19, -19.1, -19, -18.9, -18.3, -18.1, -18.6, -18.7, -18.6, -18.6, -18.5, -18.4][i], setpoint: -18 })))
  const [alarms, setAlarms] = useState<Alarm[]>([
    { severity: 'critical', title: 'Temperatura acima do limite máximo', detail: 'Limite: -10.0 °C', time: '01:15:48' },
    { severity: 'warning', title: 'Falha de comunicação com TC300', detail: 'Verificar conexão RS-485', time: '01:21:58' }
  ])
  const [connected, setConnected] = useState(true)
  const [time, setTime] = useState(new Date())
  const [range, setRange] = useState<12 | 24 | 7 | 30>(12)
  const [lastUpdate, setLastUpdate] = useState(new Date())

  // Clock update
  useEffect(() => { const id = window.setInterval(() => setTime(new Date()), 1000); return () => window.clearInterval(id) }, [])

  // Fetch initial data from Supabase
  const loadData = useCallback(async () => {
    try {
      const [telemetry, historyData, alertsData] = await Promise.all([
        fetchLatestTelemetry(),
        fetchTemperatureHistory(range),
        fetchAlerts()
      ])
      
      if (telemetry) {
        setData({
          temperature: telemetry.temperature ?? -18.4,
          humidity: telemetry.heater_output ?? 45,
          setpoint: telemetry.setpoint ?? -18,
          voltage: telemetry.inverter_voltage ?? 380,
          current: telemetry.inverter_current ?? 12.8,
          frequency: telemetry.inverter_frequency ?? 49.8,
          door: telemetry.door_open ? 'open' : 'closed',
          doorMinutes: telemetry.door_open_seconds ? Math.floor(telemetry.door_open_seconds / 60) : 0,
          inverterOnline: telemetry.inverter_running ?? true,
          tc300Online: true,
          buzzer: (telemetry.door_stage ?? 0) > 0,
          lastSeen: 'agora'
        })
        setLastUpdate(new Date())
        setConnected(true)
      }

      if (historyData && historyData.length > 0) {
        const formatted = historyData.map((h: any) => ({
          t: new Date(h.timestamp).toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit' }),
          temp: h.temperature,
          setpoint: h.setpoint
        }))
        setHistory(formatted)
      }

      if (alertsData && alertsData.length > 0) {
        setAlarms(alertsData.map((a: any) => ({
          severity: a.severity as 'warning' | 'critical',
          title: a.message,
          detail: a.device_id,
          time: new Date(a.created_at).toLocaleTimeString('pt-BR')
        })))
      }
    } catch (err) {
      console.log('Using demo data (Supabase not connected)')
      setConnected(false)
    }
  }, [range])

  // Initial load
  useEffect(() => { loadData() }, [loadData])

  // Realtime subscription
  useEffect(() => {
    const sub = subscribeToTelemetry((payload) => {
      console.log('New telemetry:', payload)
      loadData()
    })
    return () => { sub.unsubscribe() }
  }, [loadData])

  // Auto-refresh every 30s
  useEffect(() => {
    const id = window.setInterval(loadData, 30000)
    return () => window.clearInterval(id)
  }, [loadData])

  const toggleDoor = () => setData(v => ({ ...v, door: v.door === 'closed' ? 'open' : 'closed', doorMinutes: v.door === 'closed' ? 1 : 0 }))

  return <main className="iceberg-shell">
    <header className="iceberg-header"><div className="iceberg-brand"><div className="mountain-mark"><Snowflake size={30} /></div><div><div className="brand-kicker">ICEBERG</div><h1>EXECUTIVE</h1><p>MONITORAMENTO PREMIUM</p></div><div className="brand-divider" /><div className="room-name"><b>CÂMARA FRIA <em>01</em></b><span>MONITORAMENTO EM TEMPO REAL</span></div></div><div className="header-meta"><span>DATA <b>{time.toLocaleDateString('pt-BR')}</b></span><span>HORA <b>{time.toLocaleTimeString('pt-BR')}</b></span></div><div className="header-actions"><div className="connection"><span className="pulse" /> SISTEMA <b>{connected ? 'CONECTADO' : 'OFFLINE'}</b></div><button className="round-action"><Bell size={17} /><sup>{alarms.length}</sup></button><div className="profile">AH</div></div></header>
    <section className="iceberg-layout"><aside className="left-column"><article className="panel executive-summary"><span className="section-label">RESUMO EXECUTIVO</span><div className="summary-grid"><Summary label="TEMPERATURA ATUAL" value={data.temperature} unit="°C" color="blue" /><Summary label="SETPOINT" value={data.setpoint} unit="°C" color="blue" /><Summary label="HEATER OUTPUT" value={data.humidity} unit="%" color="green" /></div><div className="summary-grid secondary"><Summary label="FREQUÊNCIA" value={data.frequency} unit="Hz" color="blue" /><Summary label="TENSÃO" value={data.voltage} unit="V" color="amber" /><Summary label="CORRENTE" value={data.current} unit="A" color="coral" /></div></article><article className="panel behavior-panel"><div className="panel-title"><div><span className="section-label">COMPORTAMENTO TÉRMICO</span><h2>Temperatura e umidade relativa</h2></div><div className="range-tabs"><button className={range === 12 ? 'selected' : ''} onClick={() => setRange(12)}>12H</button><button className={range === 24 ? 'selected' : ''} onClick={() => setRange(24)}>24H</button><button className={range === 7 ? 'selected' : ''} onClick={() => setRange(7)}>7D</button><button className={range === 30 ? 'selected' : ''} onClick={() => setRange(30)}>30D</button></div></div><div className="chart-legend"><span className="line-temp" /> Temperatura (°C)<span className="line-set" /> Setpoint (°C)<span className="line-hum" /> Heater Output (%)</div><div className="behavior-chart"><ResponsiveContainer width="100%" height="100%"><AreaChart data={history} margin={{ top: 5, right: 4, left: -25, bottom: 0 }}><defs><linearGradient id="iceFill" x1="0" x2="0" y1="0" y2="1"><stop offset="0" stopColor="#13c8f3" stopOpacity=".25" /><stop offset="1" stopColor="#13c8f3" stopOpacity="0" /></linearGradient></defs><CartesianGrid stroke="#163947" /><XAxis dataKey="t" tick={{ fill: '#7894a2', fontSize: 9 }} axisLine={false} tickLine={false} /><YAxis yAxisId="temp" domain={[-24, -10]} tick={{ fill: '#7894a2', fontSize: 9 }} axisLine={false} tickLine={false} /><YAxis yAxisId="hum" orientation="right" domain={[0, 100]} tick={{ fill: '#7894a2', fontSize: 9 }} axisLine={false} tickLine={false} /><Tooltip contentStyle={{ background: '#061722', border: '1px solid #255265', borderRadius: 7 }} /><Area yAxisId="temp" dataKey="temp" type="monotone" stroke="#14c9f2" strokeWidth={2.5} fill="url(#iceFill)" /><Line yAxisId="temp" dataKey="setpoint" stroke="#658ce8" strokeDasharray="6 5" dot={false} /></AreaChart></ResponsiveContainer></div><div className="chart-footer"><b>{data.temperature} °C</b><span>TEMPERATURA ATUAL</span><b>{data.setpoint} °C</b><span>SETPOINT</span><b className="green">{data.humidity} %</b><span>HEATER OUTPUT</span></div></article></aside>
      <section className="center-column"><article className="installation-status"><span className="section-label">STATUS DA INSTALAÇÃO</span><div className="status-line"><span className="status-led" /><strong>{connected ? 'NORMAL' : 'OFFLINE'}</strong><span className="live-badge">ATUALIZAÇÃO AO VIVO</span></div><p>Última leitura recebida {data.lastSeen} pelo gateway ESP32 / Ethernet.</p></article><article className="hero-dial"><div className="dial-halo"><Dial value={data.temperature} min={-30} max={0} label="TEMPERATURA ATUAL" unit="°C" color="#13c9f1" large /><div className="dial-snow"><Snowflake size={25} /></div><div className="dial-setpoint"><span>SETPOINT</span><b>{data.setpoint} °C</b></div></div></article><div className="hero-actions"><button className="action-cyan"><Snowflake /> RESFRIAMENTO <small>Ativar</small></button><button className="action-blue"><Fan /> DEGELO <small>Iniciar</small></button></div></section>
      <aside className="right-column"><article className="panel devices-panel"><div className="panel-title"><div><span className="section-label">EQUIPAMENTOS & CONECTIVIDADE</span><h2>Saúde dos dispositivos</h2></div><Radio size={15} /></div><Device name="CLP / INVERSOR" detail={`${data.voltage} V  •  ${data.frequency} Hz  •  ${data.current} A`} online={data.inverterOnline} icon={Zap} /><Device name="CONTROLADOR TC300" detail={`${data.temperature} °C  •  ${data.humidity}% UR`} online={data.tc300Online} icon={Thermometer} /><Device name="GATEWAY ESP32" detail="Ethernet  •  Protocolo RS-485" online={connected} icon={Radio} /></article><article className="panel gauges-panel"><Dial value={data.frequency} min={40} max={60} label="FREQUÊNCIA" unit="Hz" color="#438bf4" /><Dial value={data.voltage} min={300} max={500} label="TENSÃO" unit="V" color="#f2ad31" /><Dial value={data.current} min={0} max={20} label="CORRENTE" unit="A" color="#ff7167" /></article><article className="panel door-panel"><div className="door-ring"><DoorClosed size={27} /></div><div><span className="section-label">PORTA DA CÂMARA</span><h2>PORTA {data.door === 'closed' ? 'FECHADA' : 'ABERTA'}</h2><p>{data.door === 'closed' ? 'Sensores em estado seguro' : `Aberta há ${data.doorMinutes} minuto${data.doorMinutes > 1 ? 's' : ''}`}</p></div><span className="lock"><LockKeyhole size={21} /></span><button className="tiny-button" onClick={toggleDoor}>TESTAR</button></article></aside>
    </section>
    <section className="bottom-band"><article className="panel alarms-panel"><div className="panel-title"><div><span className="section-label">LINHA DO TEMPO DE ALARMES</span><h2>Alarmes ativos <b className="alarm-count">{alarms.length}</b></h2></div><Bell size={15} /></div>{alarms.map((a, i) => <Alarm key={i} level={a.severity === 'critical' ? 'ALTA' : 'MÉDIA'} title={a.title} detail={a.detail} time={a.time} critical={a.severity === 'critical'} />)}<button className="history-button">VER HISTÓRICO COMPLETO <ChevronRight size={14} /></button></article><article className="panel actions-panel"><div className="panel-title"><div><span className="section-label">COMANDOS RÁPIDOS</span><h2>Ações operacionais</h2></div><Activity size={15} /></div><div className="quick-actions"><button className="action-amber"><Bell /> SILENCIAR ALARMES <small>5 min</small></button><button className="action-teal"><Activity /> RELATÓRIO <small>Gerar</small></button><button className="action-blue wide" onClick={() => setConnected(v => !v)}><CloudOff /> {connected ? 'SIMULAR QUEDA DE REDE' : 'RECONECTAR'}</button></div></article><article className="panel health-panel"><span className="section-label">SAÚDE DO SISTEMA</span><div className="health-content"><Dial value={connected ? 100 : 0} min={0} max={100} label={connected ? 'SAÚDE EXCELENTE' : 'SISTEMA OFFLINE'} unit="%" color="#15d5ad" /><div className="health-list"><span><Check /> Comunicação <b>{connected ? 'OK' : 'ERRO'}</b></span><span><Check /> Sensores <b>OK</b></span><span><Check /> Controladores <b>{data.tc300Online ? 'OK' : 'ERRO'}</b></span><span><Check /> Rede <b>{connected ? 'OK' : 'ERRO'}</b></span></div></div></article></section>
    <footer className="iceberg-footer"><span><Snowflake size={12} /> ICEBERG EXECUTIVE <i>•</i> PRECISÃO QUE PRESERVA, CONTROLE QUE INSPIRA.</span><span>DADOS ATUALIZADOS EM {time.toLocaleDateString('pt-BR')} {time.toLocaleTimeString('pt-BR')} <span className="pulse" /></span></footer>
  </main>
}
function Summary({ label, value, unit, color }: { label: string; value: number; unit: string; color: string }) { return <div className="summary-item"><span>{label}</span><b className={color}>{value}<small>{unit}</small></b></div> }
function Alarm({ level, title, detail, time, critical = false }: { level: string; title: string; detail: string; time: string; critical?: boolean }) { return <div className={`alarm-item ${critical ? 'critical' : 'warning'}`}><span>{level}</span><div><b>{title}</b><small>{detail}</small></div><time>{time}</time><strong>ATIVO</strong></div> }
