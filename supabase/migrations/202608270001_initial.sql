create table public.devices (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  device_type text not null check (device_type in ('gateway', 'clp_inversor', 'tc300')),
  modbus_address smallint check (modbus_address between 1 and 247),
  online boolean not null default false,
  last_seen timestamptz,
  created_at timestamptz not null default now()
);

create table public.telemetry (
  id bigint generated always as identity primary key,
  device_id uuid not null references public.devices(id) on delete cascade,
  temperature_c numeric(6,2), humidity_pct numeric(6,2), setpoint_c numeric(6,2),
  voltage_v numeric(8,2), current_a numeric(8,2), frequency_hz numeric(8,2),
  door_open boolean, buzzer_on boolean not null default false,
  raw_payload jsonb not null default '{}'::jsonb,
  recorded_at timestamptz not null default now()
);

create table public.alarms (
  id bigint generated always as identity primary key,
  source_device_id uuid references public.devices(id),
  code text not null, title text not null, severity text not null check (severity in ('warning','critical')),
  active boolean not null default true, detail text, started_at timestamptz not null default now(), ended_at timestamptz
);

alter table public.devices enable row level security;
alter table public.telemetry enable row level security;
alter table public.alarms enable row level security;
create policy "authenticated users read devices" on public.devices for select to authenticated using (true);
create policy "authenticated users read telemetry" on public.telemetry for select to authenticated using (true);
create policy "authenticated users read alarms" on public.alarms for select to authenticated using (true);

create index telemetry_device_recorded_idx on public.telemetry(device_id, recorded_at desc);
create index alarms_active_idx on public.alarms(active, started_at desc);
