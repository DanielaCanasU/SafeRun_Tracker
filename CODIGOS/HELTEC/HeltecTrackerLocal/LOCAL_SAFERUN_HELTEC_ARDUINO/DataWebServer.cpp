#include "DataWebServer.h"
#include "DataLogger.h"
#include "GPS.h"

WebServer server(80);
static SensorData lastLoRaData;
static bool hasLastLoRaData = false;

void updateLastLoRaData(const SensorData& data) {
    lastLoRaData = data;
    hasLastLoRaData = true;
}

String getHtml() {
    String html = R"====(
  <html>
  <head>
    <title>SafeRun GPS Data Logger</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      body { font-family: Arial; text-align: center; padding: 20px; }
      h1 { color: #333; }
      .info { margin: 20px 0; padding: 15px; background: #f0f0f0; border-radius: 8px; }
      a {
        display: inline-block;
        background: #2196F3; color: white;
        padding: 10px 24px; border-radius: 6px;
        text-decoration: none; margin: 10px;
      }
      button {
        background: #4CAF50; color: white;
        padding: 10px 24px; border: none; border-radius: 6px;
        cursor: pointer; margin: 10px;
      }
    </style>
    <script>
      async function updateStats() {
        const res = await fetch('/stats');
        const data = await res.json();
        document.getElementById('recordCount').textContent = data.count;
        document.getElementById('maxRecords').textContent = data.maxRecords;
      }
      setInterval(updateStats, 2000);
      window.onload = updateStats;
    </script>
  </head>
  <body>
    <h1>SafeRun GPS Data Logger</h1>
    <div class="info">
      <p>Registros almacenados: <strong id="recordCount">0</strong> / <strong id="maxRecords">800</strong></p>
    </div>
    <p><a href='/data.csv'>Descargar CSV</a></p>
    <p><button onclick='saveManual()'>Guardar Punto Manual</button></p>
    <script>
      async function saveManual() {
        const res = await fetch('/save-manual', {method: 'POST'});
        const data = await res.json();
        alert(data.message);
        updateStats();
      }
    </script>
  </body>
  </html>
  )====";
    return html;
}

void handleCSV() {
    String csv = generateCSV();
    if (csv.length() == 0) {
        server.send(404, "text/plain", "No hay datos para descargar");
        return;
    }
    server.sendHeader("Content-Disposition", "attachment; filename=gps_lora_data.csv");
    server.send(200, "text/csv", csv);
}

void handleStats() {
    uint16_t count = getRecordCount();
    uint16_t maxRecords = getMaxRecords();
    String json = "{";
    json += "\"count\":" + String(count) + ",";
    json += "\"maxRecords\":" + String(maxRecords);
    json += "}";
    server.send(200, "application/json", json);
}

void handleSaveManual() {
    if (!hasLastLoRaData) {
        server.send(200, "application/json", "{\"success\":false,\"message\":\"No hay datos de LoRa disponibles\"}");
        return;
    }
    
    if (!isGPSValid()) {
        server.send(200, "application/json", "{\"success\":false,\"message\":\"GPS no valido\"}");
        return;
    }
    
    float localLat = getLatitude();
    float localLon = getLongitude();
    
    if (saveManualRecord(lastLoRaData, localLat, localLon)) {
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Punto guardado exitosamente\"}");
    } else {
        server.send(200, "application/json", "{\"success\":false,\"message\":\"Error al guardar\"}");
    }
}

bool initDataWebServer() {
    // Eliminada inicialización extra innecesaria
    server.on("/", [](){ server.send(200, "text/html", getHtml()); });
    server.on("/data.csv", handleCSV);
    server.on("/stats", handleStats);
    server.on("/save-manual", HTTP_POST, handleSaveManual);
    server.onNotFound([](){ server.send(404, "text/plain", "Not Found"); });
    server.begin();
    Serial.println("Servidor web iniciado");
    return true;
}

void handleDataWebServer() {
    server.handleClient();
}
