#include "Accel.h"
#include "Display.h"
#include "Pins.h"
#include "AppState.h"


DetectorCaida::DetectorCaida(){
  //hola
}

void DetectorCaida::init(){
  Wire.begin(ADXL345_SDA_PIN, ADXL345_SCL_PIN);
  if (!acelerometro.begin()) {
    Serial.println("--------------------------------");
    Serial.println("No se detectó el sensor ADXL345");
    Serial.println("--------------------------------");
    delay(500);
  } else {
    acelerometro.setFreeFallThreshold(0.38);
    acelerometro.setFreeFallDuration(0.06);
    acelerometro.setActivityXYZ(1, 0);
    acelerometro.setInactivityThreshold(0.1875);
    acelerometro.setTimeInactivity(5);
    acelerometro.setInactivityXYZ(1, 1);
    acelerometro.setDataRate(ADXL345_DATARATE_100_HZ);
    acelerometro.setActivityThreshold(1.7);
    acelerometro.useInterrupt(ADXL345_INT1);
    Serial.println("--------------------------------");
    Serial.println("ADXL345 INICIADO CORRECTAMENTE");
    Serial.println("--------------------------------");
    this->checkConfig();
  }
}

void DetectorCaida::checkConfig() {
  Serial.println("--------------------------------");
  Serial.println("Configuración:");
  Serial.println("--------------------------------");
  Serial.print("Free Fall Threshold = "); Serial.println(acelerometro.getFreeFallThreshold());
  Serial.print("Free Fall Duration = "); Serial.println(acelerometro.getFreeFallDuration());
  Serial.println('.');
  Serial.print("Data Rate:    ");
  switch (acelerometro.getDataRate()) {
    case ADXL345_DATARATE_3200_HZ: Serial.print("3200 "); break;
    case ADXL345_DATARATE_1600_HZ: Serial.print("1600 "); break;
    case ADXL345_DATARATE_800_HZ: Serial.print("800 "); break;
    case ADXL345_DATARATE_400_HZ: Serial.print("400 "); break;
    case ADXL345_DATARATE_200_HZ: Serial.print("200 "); break;
    case ADXL345_DATARATE_100_HZ: Serial.print("100 "); break;
    case ADXL345_DATARATE_50_HZ: Serial.print("50 "); break;
    case ADXL345_DATARATE_25_HZ: Serial.print("25 "); break;
    case ADXL345_DATARATE_12_5_HZ: Serial.print("12.5 "); break;
    case ADXL345_DATARATE_6_25HZ: Serial.print("6.25 "); break;
    case ADXL345_DATARATE_3_13_HZ: Serial.print("3.13 "); break;
    case ADXL345_DATARATE_1_56_HZ: Serial.print("1.56 "); break;
    case ADXL345_DATARATE_0_78_HZ: Serial.print("0.78 "); break;
    case ADXL345_DATARATE_0_39_HZ: Serial.print("0.39 "); break;
    case ADXL345_DATARATE_0_20_HZ: Serial.print("0.20 "); break;
    case ADXL345_DATARATE_0_10_HZ: Serial.print("0.10 "); break;
    default: Serial.print("???? "); break;
  }
  Serial.println(" Hz");
  Serial.print("Range:         +/- ");
  switch (acelerometro.getRange()) {
    case ADXL345_RANGE_16_G: Serial.print("16 "); break;
    case ADXL345_RANGE_8_G: Serial.print("8 "); break;
    case ADXL345_RANGE_4_G: Serial.print("4 "); break;
    case ADXL345_RANGE_2_G: Serial.print("2 "); break;
    default: Serial.print("?? "); break;
  }
  Serial.println(" g");
}



float getCalibracionAcelerometro(char eje) {
  float numReadings = 500;
  int Z_out; float Z_offset;
  if (eje == 'z') {
    float zSum = 0; Serial.println("Beginning Calibration");
    for (int i = 0; i < numReadings; i++) { Z_out = acelerometro.getZ(); zSum += Z_out; }
    Z_offset = (256 - (zSum / numReadings)) / 4; Serial.print("Z_offset= "); Serial.println(Z_offset); delay(1000); return Z_offset;
  }
  return 0;
}

void readAcelerometroData() {
  float magnitud = calcularMagnitud(x, y, z);
  if (magnitud >= 14) { impacto = true; Serial.println("¡Impacto detectado!"); Serial.print("Magnitud: "); Serial.println(magnitud); }
  else { impacto = false; }
  if (free_fall) Serial.println("¡Alerta! Caida.");
  if (impacto) Serial.println("¡Alerta! Impacto.");
  if (segunda_condicion_caida) Serial.println("¡Alerta! Caida segundo.");
  if (emergencia) Serial.println("¡Alerta! EMERGENCIA");
}

void calibrateStandingPosition() {
  sensors_event_t event; acelerometro.getEvent(&event);
  initialX = event.acceleration.x; initialY = event.acceleration.y; initialZ = event.acceleration.z;
  isCalibrated = true; Serial.println("Calibración completada. Condiciones iniciales guardadas:");
  Serial.print("X: "); Serial.println(initialX);
  Serial.print("Y: "); Serial.println(initialY);
  Serial.print("Z: "); Serial.println(initialZ);
}

bool isPersonStanding() {
  if (!isCalibrated) { Serial.println("Error: No se ha realizado la calibración."); return false; }
  sensors_event_t event; acelerometro.getEvent(&event);
  float deltaX = abs(event.acceleration.x - initialX);
  float deltaY = abs(event.acceleration.y - initialY);
  float deltaZ = abs(event.acceleration.z - initialZ);
  Serial.print("deltaX: "); Serial.println(deltaX);
  Serial.print("deltaY: "); Serial.println(deltaY);
  Serial.print("deltaZ: "); Serial.println(deltaZ);
  float threshold = 5.0;
  return (deltaX <= threshold && deltaY <= threshold && deltaZ <= threshold);
}

float calcularMagnitud(float x_, float y_, float z_) { return sqrt(x_ * x_ + y_ * y_ + z_ * z_); }

void accelLoop() {
  if (isMonitoringActive) {
    if (!isCalibrated) {
      calibrateStandingPosition();
      free_fall = false; segunda_condicion_caida = false; emergencia = false; isCalibrated = true; impacto = false;
      prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
      if (currentScreen == SCREEN_MONITORING) drawMonitoringScreen();
    } else if (millis() - lastReadTime_Acelerometro >= timerDelay_Acelerometro) {
      lastReadTime_Acelerometro = millis();
      Activites activ = acelerometro.readActivites();
      if (impacto) tiempoimpacto = millis();
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) { impacto = false; tiempoimpacto = 0; }
      if (activ.isFreeFall) { Serial.println("Free Fall Detected!"); free_fall = true; time_of_fall = millis(); }
      if ((millis() - time_of_fall >= ventana_caida_a_choque) && (free_fall)) { free_fall = false; time_of_fall = 0; }
      if (activ.isActivity && free_fall) { segunda_condicion_caida = true; tiempo_de_choque_piso = millis(); }
      if (activ.isActivity && impacto) { segunda_condicion_caida = true; tiempo_de_choque_piso = millis(); }
      if ((millis() - tiempo_de_choque_piso >= ventana_choque_a_inactividad) && (segunda_condicion_caida)) { segunda_condicion_caida = false; tiempo_de_choque_piso = 0; }
      if ((activ.isInactivity && segunda_condicion_caida && !isPersonStanding()) || (activ.isInactivity && impacto && !isPersonStanding())) { emergencia = true; }
      sensors_event_t event; acelerometro.getEvent(&event); x = event.acceleration.x; y = event.acceleration.y; z = event.acceleration.z; time_dato += 0.025;
      readAcelerometroData();
      bool stateChanged = (impacto != prev_impacto || free_fall != prev_free_fall || segunda_condicion_caida != prev_segunda_condicion_caida || emergencia != prev_emergencia);
      if (stateChanged) {
        if (currentScreen == SCREEN_MONITORING) drawMonitoringScreen();
        prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
      }
    }
  } else {
    bool actual_state_changed_to_false = false;
    if (impacto) { impacto = false; actual_state_changed_to_false = true; }
    if (free_fall) { free_fall = false; actual_state_changed_to_false = true; }
    if (segunda_condicion_caida) { segunda_condicion_caida = false; actual_state_changed_to_false = true; }
    //if (emergencia) { emergencia = false; actual_state_changed_to_false = true; }
    if (isCalibrated) { isCalibrated = false; }
    bool prev_states_need_sync = (prev_impacto != impacto || prev_free_fall != free_fall || prev_segunda_condicion_caida != segunda_condicion_caida || prev_emergencia != emergencia);
    if (actual_state_changed_to_false || prev_states_need_sync) {
      prev_impacto = impacto; prev_free_fall = free_fall; prev_segunda_condicion_caida = segunda_condicion_caida; prev_emergencia = emergencia;
      if (currentScreen == SCREEN_MONITORING) drawMonitoringScreen();
    }
  }
}


