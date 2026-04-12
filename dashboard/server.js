/*
 * ============================================================
 * Library Monitoring System — Blynk ↔ Dashboard Bridge
 * 
 * This Node.js server:
 *   1. Receives data from ESP32 via Blynk WebHook
 *   2. Serves the dashboard at localhost:3000
 *   3. Pushes real-time updates to dashboard via SSE
 * 
 * Install: npm install express cors
 * Run:     node server.js
 * ============================================================
 */

const express = require('express');
const cors    = require('cors');
const path    = require('path');

const app  = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../dashboard')));

// ─── Sensor State Store ──────────────────────────────────────
let sensorData = {
  temperature : 0,
  humidity    : 0,
  noise       : 0,
  airQuality  : 0,
  occupancy   : 0,
  pir         : false,
  comfort     : 0,
  status      : 'INITIALIZING',
  timestamp   : Date.now(),
};

// ─── SSE Clients ─────────────────────────────────────────────
const clients = new Set();

function broadcast(data) {
  const payload = `data: ${JSON.stringify(data)}\n\n`;
  clients.forEach(res => res.write(payload));
}

// ─── SSE Endpoint (dashboard subscribes here) ───────────────
app.get('/events', (req, res) => {
  res.setHeader('Content-Type',  'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection',    'keep-alive');
  res.flushHeaders();

  // Send current state immediately on connect
  res.write(`data: ${JSON.stringify(sensorData)}\n\n`);
  clients.add(res);

  req.on('close', () => clients.delete(res));
});

// ─── Blynk WebHook Endpoint ──────────────────────────────────
// Configure in Blynk: POST http://<your-ip>:3000/blynk
// Body: {"pin":"V0","value":"24.5"}
app.post('/blynk', (req, res) => {
  const { pin, value } = req.body;

  const pinMap = {
    V0: 'temperature',
    V1: 'humidity',
    V2: 'noise',
    V3: 'airQuality',
    V4: 'occupancy',
    V5: 'pir',
    V6: 'comfort',
    V9: 'status',
  };

  const field = pinMap[pin];
  if (field) {
    sensorData[field] = pin === 'V5' ? value === '1' : 
                        pin === 'V9' ? value :
                        parseFloat(value);
    sensorData.timestamp = Date.now();
    broadcast(sensorData);
    console.log(`[BLYNK] ${pin} (${field}) = ${value}`);
  }

  res.json({ ok: true });
});

// ─── Direct ESP32 JSON endpoint (alternative to Blynk) ──────
// ESP32 can POST here directly if not using Blynk
app.post('/sensor', (req, res) => {
  const d = req.body;
  sensorData = { ...sensorData, ...d, timestamp: Date.now() };
  broadcast(sensorData);
  console.log('[ESP32]', JSON.stringify(d));
  res.json({ ok: true });
});

// ─── GET current state ───────────────────────────────────────
app.get('/api/state', (req, res) => res.json(sensorData));

// ─── Start ───────────────────────────────────────────────────
app.listen(PORT, () => {
  console.log(`\n┌─────────────────────────────────────┐`);
  console.log(`│  Library Monitoring System Bridge    │`);
  console.log(`│  Dashboard : http://localhost:${PORT}  │`);
  console.log(`│  Blynk hook: POST /blynk              │`);
  console.log(`│  Direct    : POST /sensor             │`);
  console.log(`└─────────────────────────────────────┘\n`);
});
