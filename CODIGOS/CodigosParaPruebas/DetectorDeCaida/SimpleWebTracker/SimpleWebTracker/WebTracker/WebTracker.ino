#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// --- Configuración WIFI ---
const char* WIFI_SSID = "Redmi Note 9S";
const char* WIFI_PASSWORD = "28082002";
const char* AP_SSID = "SafeRunTracker";
const char* AP_PASSWORD = "12345678";

// --- Pines (ajusta según tu hardware) ---
#define BUTTON_PIN 4// GPIO del botón (ajusta según tu placa)
#define ADXL345_SDA 45
#define ADXL345_SCL 46

#define MAX_DATA_COUNT 2000
#define TIMER_DELAY_MS 25

struct AccelData {
    float x, y, z;
    float magnitud;
    bool free_fall;
    bool impacto;
    bool segunda_condicion_caida;
    bool emergencia;
    float time_relativo;
};

Adafruit_ADXL345_Unified acelerometro = Adafruit_ADXL345_Unified(12345);
WebServer server(80);

AccelData dataBuffer[MAX_DATA_COUNT];
size_t dataBufferIndex = 0;
size_t dataBufferCount = 0;
float magnitud = 0;

bool isMonitoringActive = false;
bool isCalibrated = false;
bool free_fall = false, segunda_condicion_caida = false, emergencia = false, impacto = false;
bool prev_free_fall = false, prev_segunda_condicion_caida = false, prev_emergencia = false, prev_impacto = false;

float x = 0, y = 0, z = 0;
float time_dato = 0;
float initialX = 0, initialY = 0, initialZ = 0;
unsigned long lastReadTime = 0;
unsigned long time_of_fall = 0, tiempo_de_choque_piso = 0, tiempoimpacto = 0;
const unsigned long ventana_caida_a_choque = 500;
const unsigned long ventana_choque_a_inactividad = 7000;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

String getHtml() {
  String html = R"====(
  <html>
  <head>
    <title>SafeRun Tracker</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      body { font-family: Arial; text-align: center; padding: 20px; }
      h1 { color: #333; }
      .circle {
        display: inline-block;
        width: 50px; height: 50px;
        border-radius: 50%;
        margin: 10px;
        background-color: #ccc;
        box-shadow: 0 0 8px rgba(0,0,0,0.2);
        transition: background-color 0.3s;
      }
      .active { background-color: #4CAF50; } /* verde activo */
      .freefall.active { background-color: #FFEB3B; } /* amarillo */
      .impacto.active { background-color: #FF9800; } /* naranja */
      .segunda.active { background-color: #F44336; } /* rojo */
      .emergencia.active { background-color: #000; } /* negro */
      a {
        display: inline-block;
        background: #2196F3; color: white;
        padding: 10px 24px; border-radius: 6px;
        text-decoration: none; margin-top: 20px;
      }
    </style>
    <script>
      async function updateStates() {
        const res = await fetch('/status');
        const data = await res.json();
        document.getElementById('freefall').classList.toggle('active', data.free_fall);
        document.getElementById('impacto').classList.toggle('active', data.impacto);
        document.getElementById('segunda').classList.toggle('active', data.segunda_condicion_caida);
        document.getElementById('emergencia').classList.toggle('active', data.emergencia);
      }
      setInterval(updateStates, 500); // actualizar cada medio segundo
      window.onload = updateStates;
    </script>
  </head>
  <body>
    <h1>SafeRun Tracker</h1>
    <div>
      <div id='freefall' class='circle freefall'></div>
      <div id='impacto' class='circle impacto'></div>
      <div id='segunda' class='circle segunda'></div>
      <div id='emergencia' class='circle emergencia'></div>
    </div>
    <p><a href='/data.csv'>Descargar CSV</a></p>
  </body>
  </html>
  )====";
  return html;
}


void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(ADXL345_SDA, ADXL345_SCL);
  if (!acelerometro.begin()) { Serial.println("No ADXL345"); while (1) delay(1000); }
  acelerometro.setFreeFallThreshold(0.38);
  acelerometro.setFreeFallDuration(0.06);
  acelerometro.setActivityXYZ(1, 0);
  acelerometro.setInactivityThreshold(0.1875);
  acelerometro.setTimeInactivity(5);
  acelerometro.setInactivityXYZ(1, 1);
  acelerometro.setDataRate(ADXL345_DATARATE_100_HZ);
  acelerometro.setActivityThreshold(1.7);
  acelerometro.useInterrupt(ADXL345_INT1);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando...");
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nIP: "); Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("\nAP IP: "); Serial.println(WiFi.softAPIP());
  }
  server.on("/", [](){ server.send(200, "text/html", getHtml()); });
  server.on("/data.csv", handleCSV);
  server.onNotFound([](){ server.send(404, "text/plain", "Not Found"); });
  server.on("/status", []() {
  String json = "{";
  json += "\"free_fall\":" + String(free_fall ? "true" : "false") + ",";
  json += "\"impacto\":" + String(impacto ? "true" : "false") + ",";
  json += "\"segunda_condicion_caida\":" + String(segunda_condicion_caida ? "true" : "false") + ",";
  json += "\"emergencia\":" + String(emergencia ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);});

  server.begin();
}

void loop() {
  checkButton();
  if (isMonitoringActive) checkStatus();
  server.handleClient();
  delay(10);
}

void checkButton() {
  
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
      isMonitoringActive = !isMonitoringActive;
      if (isMonitoringActive) {
        Serial.println("MONITOREO INICIADO");
        calibrateStandingPosition();
        free_fall = segunda_condicion_caida = emergencia = impacto = false;
        prev_free_fall = prev_segunda_condicion_caida = prev_emergencia = prev_impacto = false;
        time_dato = 0; dataBufferCount = 0; dataBufferIndex = 0;
      } else {
        Serial.println("MONITOREO PARADO");
        isCalibrated = false;
      }
    }
  lastButtonState = buttonState;
}

void calibrateStandingPosition() {
  sensors_event_t event; acelerometro.getEvent(&event);
  initialX = event.acceleration.x;
  initialY = event.acceleration.y;
  initialZ = event.acceleration.z;
  isCalibrated = true;
  Serial.println("Calibración completada");
}

/*
void checkStatus() {
  if (isMonitoringActive) {  // Monitorear detector de caída solo si se está haciendo ejercicio
    if (!isCalibrated) {  // Calibrar al momento de iniciar actividad física
      calibrateStandingPosition();
      free_fall = false;
      segunda_condicion_caida = false;
      emergencia = false;
      impacto = false;
      isCalibrated = true;

      prev_impacto = impacto;
      prev_free_fall = free_fall;
      prev_segunda_condicion_caida = segunda_condicion_caida;
      prev_emergencia = emergencia;

    } else if (millis() - lastReadTime >= TIMER_DELAY_MS) {  // Leer el acelerómetro periódicamente
      lastReadTime = millis();

      Activites activ = acelerometro.readActivites(); 

      sensors_event_t event;
      acelerometro.getEvent(&event);
      x = event.acceleration.x;
      y = event.acceleration.y;
      z = event.acceleration.z;
      time_dato += 0.025;

      float magnitud = sqrt(x * x + y * y + z * z);

      // --- Lógica de detección ---

      if (impacto)
        tiempoimpacto = millis();

      // Limpiar si pasó mucho tiempo desde la caída libre sin impacto
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && free_fall) {
        impacto = false;
        tiempoimpacto = 0;
        free_fall = false;
        time_of_fall = 0;
      }

      // Caída libre detectada
      if (magnitud < 0.5) {
        Serial.println("Free Fall Detected!");
        free_fall = true;
        time_of_fall = millis();
      }

      // Impacto después de caída libre o antes (segunda condición)
      if ((magnitud > 13.0 && free_fall) || (magnitud > 13.0 && impacto)) {
        segunda_condicion_caida = true;
        tiempo_de_choque_piso = millis();
      }

      // Si ya pasó mucho tiempo sin inactividad después del choque
      if ((millis() - tiempo_de_choque_piso >= ventana_choque_a_inactividad) && segunda_condicion_caida) {
        segunda_condicion_caida = false;
        tiempo_de_choque_piso = 0;
      }

      // Detectar impacto
      if (magnitud > 13.0) {
        impacto = true;
        Serial.println("¡Impacto detectado!");
      } else {
        impacto = false;
      }

      // Si hay inactividad después de las condiciones y no está de pie → emergencia
      if ((segunda_condicion_caida && !isPersonStanding()) || (impacto && !isPersonStanding())) {
        emergencia = true;
      }

      // Guardar datos
      addData(x, y, z, magnitud, free_fall, impacto, segunda_condicion_caida, emergencia, time_dato);

      // Log para depuración
      bool stateChanged = (impacto != prev_impacto ||
                           free_fall != prev_free_fall ||
                           segunda_condicion_caida != prev_segunda_condicion_caida ||
                           emergencia != prev_emergencia);

      if (stateChanged) {
        if (free_fall) Serial.println("¡Alerta! Caída libre.");
        if (impacto) Serial.println("¡Alerta! Impacto.");
        if (segunda_condicion_caida) Serial.println("¡Alerta! Segunda condición.");
        if (emergencia) Serial.println("¡Alerta! EMERGENCIA");

        prev_impacto = impacto;
        prev_free_fall = free_fall;
        prev_segunda_condicion_caida = segunda_condicion_caida;
        prev_emergencia = emergencia;
      }
    }
  } else {
    // Si se detuvo el monitoreo, limpiar estados
    bool actual_state_changed_to_false = false;
    if (impacto) { impacto = false; actual_state_changed_to_false = true; }
    if (free_fall) { free_fall = false; actual_state_changed_to_false = true; }
    if (segunda_condicion_caida) { segunda_condicion_caida = false; actual_state_changed_to_false = true; }
    if (isCalibrated) { isCalibrated = false; }

    bool prev_states_need_sync = (prev_impacto != impacto ||
                                  prev_free_fall != free_fall ||
                                  prev_segunda_condicion_caida != segunda_condicion_caida ||
                                  prev_emergencia != emergencia);

    if (actual_state_changed_to_false || prev_states_need_sync) {
      prev_impacto = impacto;
      prev_free_fall = free_fall;
      prev_segunda_condicion_caida = segunda_condicion_caida;
      prev_emergencia = emergencia;
    }
  }
}

*/

void checkStatus() {
  if (isMonitoringActive) {  //Monitorear detector de caida solo si se está haciendo ejercicio
    if (!isCalibrated) { //Calibrar al momento de iniciar actividad fisica
      calibrateStandingPosition();
      free_fall = false; segunda_condicion_caida = false; emergencia = false; isCalibrated = true; impacto = false; //Condiciones iniciales 
      prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
    } else if (millis() - lastReadTime >= TIMER_DELAY_MS) { //Leer el acelerometro de manera periodica
      lastReadTime = millis();
      Activites activ = acelerometro.readActivites(); 
      if (impacto) tiempoimpacto = millis();
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) { impacto = false; tiempoimpacto = 0; free_fall = false; time_of_fall = 0; } //Limpiar si pasó mucho tiempo desde la caida libre y no hubo impacto
      if (activ.isFreeFall) { Serial.println("Free Fall Detected!"); free_fall = true; time_of_fall = millis(); } //Caida libre detectada
      if (activ.isActivity && free_fall ||activ.isActivity && impacto) { segunda_condicion_caida = true; tiempo_de_choque_piso = millis(); } //Impacto despues de la caida libre o impacto antes de la caida libre
      if ((millis() - tiempo_de_choque_piso >= ventana_choque_a_inactividad) && (segunda_condicion_caida)) { segunda_condicion_caida = false; tiempo_de_choque_piso = 0; } //Si ya pasó mucho tiempo no hubo inactividad despues de choque
      if ((activ.isInactivity && segunda_condicion_caida && !isPersonStanding()) || (activ.isInactivity && impacto && !isPersonStanding())) { emergencia = true; } //Si se realizaron las 3 condiciones, emergencia 
      sensors_event_t event; acelerometro.getEvent(&event); x = event.acceleration.x; y = event.acceleration.y; z = event.acceleration.z; time_dato += 0.025;
      readAcelerometroData();
      addData(x, y, z, magnitud, free_fall, impacto, segunda_condicion_caida, emergencia, time_dato);

      bool stateChanged = (impacto != prev_impacto || free_fall != prev_free_fall || segunda_condicion_caida != prev_segunda_condicion_caida || emergencia != prev_emergencia);
      if (stateChanged) {
        prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
      }
    }
  } else {
    bool actual_state_changed_to_false = false;
    if (impacto) { impacto = false; actual_state_changed_to_false = true; }
    if (free_fall) { free_fall = false; actual_state_changed_to_false = true; }
    if (segunda_condicion_caida) { segunda_condicion_caida = false; actual_state_changed_to_false = true; }
    if (isCalibrated) { isCalibrated = false; }
    bool prev_states_need_sync = (prev_impacto != impacto || prev_free_fall != free_fall || prev_segunda_condicion_caida != segunda_condicion_caida || prev_emergencia != emergencia);
    if (actual_state_changed_to_false || prev_states_need_sync) {
      prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
    }
  }
}
bool isPersonStanding() {
  if (!isCalibrated) return false;
  sensors_event_t event; acelerometro.getEvent(&event);
  float deltaX = abs(event.acceleration.x - initialX);
  float deltaY = abs(event.acceleration.y - initialY);
  float deltaZ = abs(event.acceleration.z - initialZ);
  float threshold = 5.0;
  return (deltaX <= threshold && deltaY <= threshold && deltaZ <= threshold);
}

void addData(float x, float y, float z, float magnitud, bool free_fall, bool impacto, bool segunda_condicion_caida, bool emergencia, float time_relativo) {
  if (dataBufferIndex >= MAX_DATA_COUNT) dataBufferIndex = 0;
  dataBuffer[dataBufferIndex] = { x, y, z, magnitud, free_fall, impacto, segunda_condicion_caida, emergencia, time_relativo };
  if (dataBufferCount < MAX_DATA_COUNT) dataBufferCount++;
  dataBufferIndex++;
}

void handleCSV() {
  String csv = "time_relativo,x,y,z,magnitud,free_fall,impacto,segunda_condicion_caida,emergencia\n";
  size_t startIdx = (dataBufferCount >= 2000) ? (dataBufferIndex) : 0;
  size_t count = (dataBufferCount >= 2000) ? 2000 : dataBufferCount;
  for (size_t i = 0; i < count; i++) {
    size_t idx = (startIdx + i) % MAX_DATA_COUNT;
    const AccelData& d = dataBuffer[idx];
    char line[128];
    snprintf(line, sizeof(line), "%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d\n", d.time_relativo, d.x, d.y, d.z, d.magnitud, d.free_fall, d.impacto, d.segunda_condicion_caida, d.emergencia);
    csv += line;
  }
  server.sendHeader("Content-Disposition","attachment; filename=acelerometro_data.csv");
  server.send(200, "text/csv", csv);
}


void readAcelerometroData() {
  magnitud = calcularMagnitud(x, y, z);
  if (magnitud >= 28) { impacto = true; Serial.println("¡Impacto detectado!"); Serial.print("Magnitud: "); Serial.println(magnitud); }
  else { impacto = false; }
  if (free_fall) Serial.println("¡Alerta! Caida.");
  if (impacto) Serial.println("¡Alerta! Impacto.");
  if (segunda_condicion_caida) Serial.println("¡Alerta! Caida segundo.");
  if (emergencia) Serial.println("¡Alerta! EMERGENCIA");
}

float calcularMagnitud(float x_, float y_, float z_) { return sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
