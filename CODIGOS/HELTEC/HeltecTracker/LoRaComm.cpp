#include "LoRaComm.h"
#include <SPI.h>
#include "SX1262_Settings.h"

bool sendCommand(String command, unsigned long timeout, bool waitForOK) {
  Serial.print("Enviando Comando: ");
  Serial.println(command);
  // Si se habilita el RYLR998, aquí iría SerialLoRa.println(command)
  unsigned long startTime = millis();
  String response = ""; bool okReceived = false;
  while (millis() - startTime < timeout) {
    // Leer de SerialLoRa si se usa ese módulo
  }
  if (waitForOK) return okReceived;
  return true;
}

void configureSX1262() {
  LT.setMode(MODE_STDBY_RC);
  LT.setRegulatorMode(USE_DCDC);
  LT.setPaConfig(0x04, PAAUTO, LORA_DEVICE);
  LT.setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
  LT.calibrateDevice(ALLDevices);
  LT.calibrateImage(Frequency);
  LT.setDIO2AsRfSwitchCtrl();
  LT.setPacketType(PACKET_TYPE_LORA);
  LT.setRfFrequency(Frequency, Offset);
  LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);
  LT.setBufferBaseAddress(0, 0);
  LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
  LT.setHighSensitivity();
  LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
}

void sendMessage(const char* message, bool verbose) {
  int TXPacketL = strlen(message);
  if (verbose) {
    Serial.print("Contenido de message: "); Serial.println(message);
    Serial.print("Bytes como int: ");
    for (int i = 0; i < TXPacketL; i++) { Serial.print((int)message[i]); Serial.print(" "); }
    Serial.println();
    Serial.print("Longitud real del mensaje: "); Serial.println(TXPacketL);
    Serial.print(TXpower); Serial.print(F("dBm ")); Serial.print(F("Packet> ")); Serial.flush();
  }
  unsigned long startmS = millis();
  delay(10);
  if (TRANSMISION_COMPLETADA || TRANSMISION_FALLIDA) {
    LT.transmitDaniela((uint8_t*)message, TXPacketL, 10000, TXpower, WAIT_TX);
    TRANSMISION_COMPLETADA = false; TRANSMISION_FALLIDA = false;
  } else if (digitalRead(14)) {
    if (LT.readIrqStatus() & IRQ_RX_TX_TIMEOUT) TRANSMISION_FALLIDA = true; else TRANSMISION_COMPLETADA = true;
  }
  if (TRANSMISION_COMPLETADA) {
    unsigned long endmS = millis();
    if (verbose) {
      uint16_t localCRC = LT.CRCCCITT((uint8_t*)message, TXPacketL, 0xFFFF);
      Serial.print(F("  BytesSent,")); Serial.print(TXPacketL);
      Serial.print(F("  CRC,")); Serial.print(localCRC, HEX);
      Serial.print(F("  TransmitTime,")); Serial.print(endmS - startmS); Serial.print(F("mS"));
      Serial.print(F("  PacketsSent,")); Serial.print(TXPacketCount);
    }
  }
  if (TRANSMISION_FALLIDA) {
    uint16_t IRQStatus = LT.readIrqStatus();
    if (verbose) {
      Serial.print(F(" SendError,")); Serial.print(F("Length,")); Serial.print(TXPacketL);
      Serial.print(F(",IRQreg,")); Serial.print(IRQStatus, HEX);
    }
    LT.printIrqStatus();
  }
}

void checkForIncomingMessage() {
  // Si se usa RYLR998, implementar aquí la lectura de +RCV
}


