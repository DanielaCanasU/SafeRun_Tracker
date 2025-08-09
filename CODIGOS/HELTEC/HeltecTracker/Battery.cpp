#include "Battery.h"

float readBatteryVoltage() {
  int raw = analogRead(VBAT_ADC_PIN);
  float voltage = ((raw / 4095.0f) * 3.3f) * 4.9f;
  return voltage;
}

int batteryVoltageToPercent(float voltage) {
  if (voltage >= 4.2f) return 100;
  if (voltage <= 3.2f) return 0;
  return (int)(((voltage - 3.2f) / (4.2f - 3.2f)) * 100.0f);
}


