# Câmara Fria

Dashboard web responsivo e base de telemetria para ESP32 + RS-485 + Ethernet.

## Rodar o dashboard

```powershell
npm install
npm run dev
```

Sem variáveis Supabase, a tela inicia em modo demonstrativo. Para conectar ao backend, copie `supabase/.env.example` para `.env` e configure `VITE_SUPABASE_URL` e `VITE_SUPABASE_ANON_KEY`.

## Supabase local

```powershell
supabase start
supabase db reset
```

Depois configure as variáveis com as chaves exibidas por `supabase status`.

Para a ingestão do ESP32, configure `CAMARA_DEVICE_INGEST_TOKEN` na Edge Function. O token é enviado no cabeçalho `x-device-token`; a chave `SUPABASE_SERVICE_ROLE_KEY` nunca vai para o frontend ou firmware.
