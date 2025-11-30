#include "LoRaComm.h"
#include <SPI.h>
#include "SX1262_Settings.h"
#include "Config.h"
#include "AppState.h"
#include <AES.h> // Librería de Matej Sychra

// Clave y vector de inicialización para AES (16 bytes cada uno)
static uint8_t aes_key[16] = { 'S', 'A', 'F', 'E', 'R', 'U', 'N', 'C', 'I', 'F', 'R', 'A', 'D', 'O', '1', '2' };
static uint8_t aes_iv[16]  = { 'I', 'n', 'i', 'c', 'i', 'a', 'l', 'I', 'V', '1', '2', '3', '4', '5', '6', '7' };
AES aes;


static uint32_t RXpacketCount = 0;
static uint32_t errors = 0;
static uint8_t RXBUFFER[RXBUFFER_SIZE];
static uint8_t RXPacketL = 0;
static int8_t PacketRSSI = 0;
static int8_t PacketSNR = 0;
static String lastPacket = "";
const unsigned long LORA_CHECK_INTERVAL = 400; // 100ms



LoRa::LoRa(){}


void LoRa::init(){
  SPI.begin();
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, SW, LORA_DEVICE)) {
    Serial.println("--------------------------------");
    Serial.println("LORA INICIADO CORRECTAMENTE");
    Serial.println("--------------------------------");
      delay(1000);
    } else {
      Serial.println("--------------------------------");
      Serial.println("ERROR INICIANDO LORA");
      Serial.println("--------------------------------");
    }
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

bool LoRa::checkIncomeMessage() {
  unsigned long now = millis();
  // LoRa reception logic (exactly like ESP32)
  if(!WAITING_LORA) {
      LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on RX done or timeout
      LT.setRx(10000);
      WAITING_LORA = true;
      //SI ENTRA AQUI
  }
  
  // Process LoRa messages
  if (now - lastLoraCheck >= LORA_CHECK_INTERVAL) {
      if(digitalRead(14)) {
              Serial.println("ACTIVADO");
              bool ACKrecibido = processLoRaMessage();
              Serial.println("Valid LoRa message received");
                      
              if(ACKrecibido) {
                WAITING_LORA = false;
              }
              
              //LT.setTx(1000);
              return ACKrecibido;
      } 
      delay(10);
      lastLoraCheck = now;  
      return false;
  }
}


void LoRa::sendMessage(const char* message, bool verbose) {
  const unsigned long currentTime = millis();

  // Cifrar solo el mensaje de datos (sin los IDs)
  String cifrado = cifrarValor(String(message));
  
  // Construir mensaje completo con IDs sin cifrar al inicio
  // Formato: FROM:deviceId;TO:targetId;DATA:cifrado
  String mensajeCompleto = "FROM:" + String(DEVICE_ID) + ";TO:" + String(TARGET_DEVICE_ID) + ";ACK:" + String(loRa.lastSendACK) + ";DATA:" + cifrado;
  
  int TXPacketL = mensajeCompleto.length();
  if (verbose) {
    Serial.print("Mensaje completo (IDs sin cifrar): "); Serial.println(mensajeCompleto);
    Serial.print("Contenido cifrado (solo DATA): "); Serial.println(cifrado);
    Serial.print("Bytes como int: ");
    for (int i = 0; i < TXPacketL; i++) { Serial.print((int)mensajeCompleto[i]); Serial.print(" "); }
    Serial.println();
    Serial.print("Longitud real del mensaje completo: "); Serial.println(TXPacketL);
    Serial.print(TXpower); Serial.print(F("dBm ")); Serial.print(F("Packet> ")); Serial.flush();
  }
  unsigned long startmS = millis();
  delay(10);
  if (TRANSMISION_COMPLETADA || TRANSMISION_FALLIDA) {
    LT.transmitDaniela((uint8_t*)mensajeCompleto.c_str(), TXPacketL, 10000, TXpower, true);
    TRANSMISION_COMPLETADA = false; TRANSMISION_FALLIDA = false;
  } else if (digitalRead(14)) {
    if (LT.readIrqStatus() & IRQ_RX_TX_TIMEOUT){
       TRANSMISION_FALLIDA = true;}
       else {
        TRANSMISION_COMPLETADA = true;
        waitingACK = true;
        lastSendTime_LoRa = currentTime;
        //LT.setMode(MODE_STDBY_RC);
        //LT.setRx(10000);
        diferencia = loRa.lastSendACK - loRa.ultimoACK;
        WAITING_LORA = false;
        loRa.lastSendACK++;
        Serial.print("Mensajes no recibidos: ");
        Serial.println(diferencia);
        }
  }
  if (TRANSMISION_COMPLETADA) {
    unsigned long endmS = millis();
    if (verbose) {
      uint16_t localCRC = LT.CRCCCITT((uint8_t*)mensajeCompleto.c_str(), TXPacketL, 0xFFFF);
      Serial.print(F("  BytesSent,")); Serial.print(TXPacketL);
      Serial.print(F("  CRC,")); Serial.print(localCRC, HEX);
      Serial.print(F("  TransmitTime,")); Serial.print(endmS - startmS); Serial.print(F("mS"));
      Serial.print(F("  PacketsSent,")); Serial.print(TXPacketCount);
    }
    /*
    // Esperar ACK después de enviar exitosamente
    if (verbose) {
      Serial.println(F("\nEsperando ACK..."));
    }
    bool ackOk = waitForACK();
    if (ackOk) {
      if (verbose) {
        Serial.println(F("✓ Mensaje confirmado por receptor"));
      }
    } else {
      if (verbose) {
        Serial.println(F("✗ No se recibió confirmación (ACK timeout)"));
      }
    }
      */
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


String LoRa::cifrarValor(String texto) {
  const int plainLength = texto.length();
  if (plainLength <= 0) {
    return "";
  }

  const int paddedLength = ((plainLength + 15) / 16) * 16; // múltiplo de 16

  uint8_t *plainBuffer = (uint8_t *)malloc(paddedLength);
  if (!plainBuffer) {
    return "";
  }
  memset(plainBuffer, 0, paddedLength);
  texto.getBytes(plainBuffer, paddedLength); // copia y rellena con 0 lo que falte

  uint8_t *cipherBuffer = (uint8_t *)malloc(paddedLength);
  if (!cipherBuffer) {
    free(plainBuffer);
    return "";
  }

  uint8_t ivLocal[16];
  memcpy(ivLocal, aes_iv, 16);

  aes.do_aes_encrypt(plainBuffer, paddedLength, cipherBuffer, aes_key, 128, ivLocal);

  String resultado;
  resultado.reserve(paddedLength * 2);
  for (int i = 0; i < paddedLength; i++) {
    if (cipherBuffer[i] < 16) resultado += "0";
    resultado += String(cipherBuffer[i], HEX);
  }

  free(plainBuffer);
  free(cipherBuffer);
  return resultado;
}


bool processLoRaMessage() { // El parámetro 'data' se mantiene por compatibilidad
  Serial.println("Entro");
  RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, LORA_TIMEOUT, true);
  PacketRSSI = LT.readPacketRSSI();
  PacketSNR = LT.readPacketSNR();
  
  
  if (RXPacketL == 0) {
      
      uint16_t IRQStatus = LT.readIrqStatus();
      //AQUI ESTA ENTRANDO

      if (IRQStatus & IRQ_RX_TIMEOUT) {
        //AQUI NO ESTA ENTRANDO
          Serial.print(F(" RXTimeout"));
          Serial.print("Received ");
          // No incrementes errores por timeout, es normal si no hay transmisión
      } else if (IRQStatus & (IRQ_HEADER_ERROR | IRQ_CRC_ERROR)) {
          Serial.print("Received ");
          errors++;
          Serial.print(F(" PacketError"));
          Serial.print(F(",RSSI,"));
          Serial.print(PacketRSSI);
          Serial.print(F("dBm,SNR,"));
          Serial.print(PacketSNR);
          Serial.print(F("dB,Length,"));
          Serial.print(LT.readRXPacketL());
          Serial.print(F(",Packets,"));
          Serial.print(RXpacketCount);
          Serial.print(F(",Errors,"));
          Serial.print(errors);
          Serial.print(F(",IRQreg,"));
          Serial.print(IRQStatus, HEX);
          LT.printIrqStatus();
      }
      // Si IRQStatus == 0, no hagas nada (no hay evento relevante)
      return false;
  } else {
      uint16_t IRQStatus = LT.readIrqStatus();
      RXpacketCount++;
      Serial.print(F("Received length: "));
      Serial.println(RXPacketL);
      Serial.print(F("Buffer HEX: "));
      for (int i = 0; i < RXPacketL; i++) {
          if (RXBUFFER[i] < 16) Serial.print("0");
          Serial.print(RXBUFFER[i], HEX);
          Serial.print(" ");
      }
      Serial.println();
      LT.printASCIIPacket(RXBUFFER, RXPacketL);
      
      // Extrae texto del paquete recibido
      String packet = "";
      for (int i = 0; i < RXPacketL; i++) {
          packet += (char)RXBUFFER[i];
      }
      Serial.print("Paquete recibido completo: ");
      Serial.println(packet);
      
      // Extraer IDs sin cifrar (formato: FROM:deviceId;TO:targetId;DATA:cifrado)
      String fromId = "";
      String toId = "";
      String ACKMessage = "";
      
      // Buscar marcadores en el formato: FROM:deviceId;TO:targetId;DATA:cifrado
      int fromIndex = packet.indexOf("FROM:");
      int toIndex = packet.indexOf("TO:");
      int ackIndex = packet.indexOf("ACK:");
      
      if (fromIndex == -1 || toIndex == -1 || ackIndex == -1) {
          Serial.println("Error: Formato de mensaje incorrecto. No se encontraron IDs.");
          return false;
      }
      
      // Verificar que los índices estén en orden correcto
      if (fromIndex >= toIndex || toIndex >= ackIndex) {
          Serial.println("Error: Orden incorrecto de los marcadores en el mensaje.");
          return false;
      }
      
      // Extraer FROM ID: desde después de "FROM:" hasta antes de ";TO:"
      // Buscar el punto y coma después de FROM:
      int fromSemicolon = packet.indexOf(';', fromIndex + 5);
      if (fromSemicolon == -1 || fromSemicolon > toIndex) {
          Serial.println("Error: No se encontró delimitador después de FROM:");
          return false;
      }
      fromId = packet.substring(fromIndex + 5, fromSemicolon);
      
      // Extraer TO ID: desde después de "TO:" hasta antes de ";DATA:"
      // Buscar el punto y coma después de TO:
      int toSemicolon = packet.indexOf(';', toIndex + 3);
      if (toSemicolon == -1 || toSemicolon > ackIndex) {
          Serial.println("Error: No se encontró delimitador después de TO:");
          return false;
      }
      toId = packet.substring(toIndex + 3, toSemicolon);
      
      // Extraer DATA (parte cifrada): desde después de "DATA:" hasta el final
      ACKMessage = packet.substring(ackIndex + 4);
      
      Serial.print("FROM ID: ");
      Serial.println(fromId);
      Serial.print("TO ID: ");
      Serial.println(toId);
      Serial.print("ACK (cifrado): ");
      Serial.println(ACKMessage);
      
      // Verificar si el mensaje es para este dispositivo
      String localDeviceId = String(DEVICE_ID);
      if (toId != localDeviceId) {
          Serial.print("Mensaje no es para este dispositivo. TO: ");
          Serial.print(toId);
          Serial.print(", Local: ");
          Serial.println(localDeviceId);
          return false;
      }
      
      Serial.println("Mensaje dirigido a este dispositivo. Procediendo a descifrar...");
      
      // Descifra solo la parte DATA del mensaje
      String packetDescifrado = descifrarValor(ACKMessage);
      Serial.print("Mensaje descifrado: ");
      Serial.println(packetDescifrado);
      
      loRa.ultimoACK = int(packetDescifrado.toInt());  
      return true;
  }
}

// Cifrar mensaje antes de enviar usando AES (Matej Sychra) - soporta longitud variable (múltiplos de 16)
String cifrarValor(String texto) {
  const int plainLength = texto.length();
  if (plainLength <= 0) {
    return "";
  }

  const int paddedLength = ((plainLength + 15) / 16) * 16; // múltiplo de 16

  uint8_t *plainBuffer = (uint8_t *)malloc(paddedLength);
  if (!plainBuffer) {
    return "";
  }
  memset(plainBuffer, 0, paddedLength);
  texto.getBytes(plainBuffer, paddedLength); // copia y rellena con 0 lo que falte

  uint8_t *cipherBuffer = (uint8_t *)malloc(paddedLength);
  if (!cipherBuffer) {
    free(plainBuffer);
    return "";
  }

  uint8_t ivLocal[16];
  memcpy(ivLocal, aes_iv, 16);

  aes.do_aes_encrypt(plainBuffer, paddedLength, cipherBuffer, aes_key, 128, ivLocal);

  String resultado;
  resultado.reserve(paddedLength * 2);
  for (int i = 0; i < paddedLength; i++) {
    if (cipherBuffer[i] < 16) resultado += "0";
    resultado += String(cipherBuffer[i], HEX);
  }

  free(plainBuffer);
  free(cipherBuffer);
  return resultado;
}

// Descifrar mensaje al recibir usando AES (Matej Sychra) - soporta longitud variable
String descifrarValor(String codificado) {
  const int encodedLength = codificado.length();
  if (encodedLength <= 0 || (encodedLength % 2) != 0) {
    return "";
  }

  const int cipherLength = encodedLength / 2; // bytes
  uint8_t *cipherBuffer = (uint8_t *)malloc(cipherLength);
  if (!cipherBuffer) {
    return "";
  }

  for (int i = 0; i < cipherLength; i++) {
    String byteStr = codificado.substring(i * 2, i * 2 + 2);
    cipherBuffer[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }

  uint8_t *plainBuffer = (uint8_t *)malloc(cipherLength);
  if (!plainBuffer) {
    free(cipherBuffer);
    return "";
  }

  uint8_t ivLocal[16];
  memcpy(ivLocal, aes_iv, 16);

  aes.do_aes_decrypt(cipherBuffer, cipherLength, plainBuffer, aes_key, 128, ivLocal);

  String resultado;
  resultado.reserve(cipherLength);
  for (int i = 0; i < cipherLength; i++) {
    if (plainBuffer[i] == 0) break; // quitar padding cero
    resultado += (char)plainBuffer[i];
  }

  free(cipherBuffer);
  free(plainBuffer);
  return resultado;
}


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

/*
void sendMessage(const char* message, bool verbose) {
  const unsigned long currentTime = millis();

  // Cifrar solo el mensaje de datos (sin los IDs)
  String cifrado = cifrarValor(String(message));
  
  // Construir mensaje completo con IDs sin cifrar al inicio
  // Formato: FROM:deviceId;TO:targetId;DATA:cifrado
  String mensajeCompleto = "FROM:" + String(DEVICE_ID) + ";TO:" + String(TARGET_DEVICE_ID) + ";ACK:" + String(loRa.lastSendACK) + ";DATA:" + cifrado;
  
  int TXPacketL = mensajeCompleto.length();
  if (verbose) {
    Serial.print("Mensaje completo (IDs sin cifrar): "); Serial.println(mensajeCompleto);
    Serial.print("Contenido cifrado (solo DATA): "); Serial.println(cifrado);
    Serial.print("Bytes como int: ");
    for (int i = 0; i < TXPacketL; i++) { Serial.print((int)mensajeCompleto[i]); Serial.print(" "); }
    Serial.println();
    Serial.print("Longitud real del mensaje completo: "); Serial.println(TXPacketL);
    Serial.print(TXpower); Serial.print(F("dBm ")); Serial.print(F("Packet> ")); Serial.flush();
  }
  unsigned long startmS = millis();
  delay(10);
  if (TRANSMISION_COMPLETADA || TRANSMISION_FALLIDA) {
    LT.transmitDaniela((uint8_t*)mensajeCompleto.c_str(), TXPacketL, 10000, TXpower, WAIT_TX);
    TRANSMISION_COMPLETADA = false; TRANSMISION_FALLIDA = false;
  } else if (digitalRead(14)) {
    if (LT.readIrqStatus() & IRQ_RX_TX_TIMEOUT){
       TRANSMISION_FALLIDA = true;}
       else {
        TRANSMISION_COMPLETADA = true;
        waitingACK = true;
        lastSendTime_LoRa = currentTime;
        //LT.setMode(MODE_STDBY_RC);
        //LT.setRx(10000);
        WAITING_LORA = false;
        loRa.lastSendACK++;
        }
  }
  if (TRANSMISION_COMPLETADA) {
    unsigned long endmS = millis();
    if (verbose) {
      uint16_t localCRC = LT.CRCCCITT((uint8_t*)mensajeCompleto.c_str(), TXPacketL, 0xFFFF);
      Serial.print(F("  BytesSent,")); Serial.print(TXPacketL);
      Serial.print(F("  CRC,")); Serial.print(localCRC, HEX);
      Serial.print(F("  TransmitTime,")); Serial.print(endmS - startmS); Serial.print(F("mS"));
      Serial.print(F("  PacketsSent,")); Serial.print(TXPacketCount);
    }
    /*
    // Esperar ACK después de enviar exitosamente
    if (verbose) {
      Serial.println(F("\nEsperando ACK..."));
    }
    bool ackOk = waitForACK();
    if (ackOk) {
      if (verbose) {
        Serial.println(F("✓ Mensaje confirmado por receptor"));
      }
    } else {
      if (verbose) {
        Serial.println(F("✗ No se recibió confirmación (ACK timeout)"));
      }
    }
      */
      /*
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
*/
// Variables globales para rastrear ACK
static bool ackReceived = false;
static bool waitingForACK = false;
static unsigned long ackWaitStartTime = 0;

// Función para verificar si hay un ACK recibido
bool checkForACK() {
  if (!waitingForACK) {
    return false;
  }
  
  // Verificar timeout
  unsigned long now = millis();
  if (now - ackWaitStartTime > ACK_TIMEOUT) {
    waitingForACK = false;
    Serial.println("ACK timeout - no se recibió confirmación");
    return false;
  }
  
  // Verificar si hay un paquete recibido
  if (digitalRead(14)) {  // DIO1 indica IRQ
    uint16_t IRQStatus = LT.readIrqStatus();
    
    if (IRQStatus & IRQ_RX_DONE) {
      // Hay un mensaje recibido, intentar leerlo
      uint8_t rxBuffer[64];
      uint8_t rxLength = LT.receive(rxBuffer, sizeof(rxBuffer), 0, WAIT_RX);
      
      if (rxLength > 0) {
        String packet = "";
        for (int i = 0; i < rxLength; i++) {
          packet += (char)rxBuffer[i];
        }
        
        // Verificar si es un ACK
        // Formato: ACK:FROM:456;TO:001
        Serial.print("Paquete recibido (posible ACK): ");
        Serial.println(packet);
        
        if (packet.indexOf("ACK:") == 0) {
          int toIndex = packet.indexOf("TO:");
          if (toIndex != -1) {
            // Extraer TO ID (desde "TO:" hasta el final, sin espacios)
            String toId = packet.substring(toIndex + 3);
            toId.trim(); // Limpiar espacios y saltos de línea
            
            Serial.print("ACK recibido con TO ID: ");
            Serial.print(toId);
            Serial.print(", Local DEVICE_ID: ");
            Serial.println(DEVICE_ID);
            
            // Verificar si el ACK es para este dispositivo
            if (toId == String(DEVICE_ID)) {
              Serial.println("✓ ACK recibido exitosamente!");
              ackReceived = true;
              waitingForACK = false;
              return true;
            } else {
              Serial.print("ACK recibido pero no es para este dispositivo. TO: ");
              Serial.println(toId);
            }
          }
        } else {
          Serial.println("Paquete recibido no es un ACK (no comienza con 'ACK:')");
        }
      }
      
      // Limpiar IRQ
      LT.clearIrqStatus(IRQ_RADIO_ALL);
    }
  }
  
  return false;
}

// Función para esperar ACK después de enviar un mensaje
bool waitForACK() {
  waitingForACK = true;
  ackReceived = false;
  ackWaitStartTime = millis();
  
  // Configurar para recibir
  LT.setMode(MODE_RX);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
  LT.setRx(ACK_TIMEOUT);
  
  // Esperar ACK
  while (waitingForACK && (millis() - ackWaitStartTime < ACK_TIMEOUT)) {
    if (checkForACK()) {
      return true;
    }
    delay(10); // Pequeño delay para no saturar el loop
  }
  
  waitingForACK = false;
  return ackReceived;
}

void checkForIncomingMessage() {
  // Esta función puede usarse para verificar mensajes entrantes en el loop principal
  // Por ahora, solo verifica ACK si estamos esperando uno
  if (waitingForACK) {
    checkForACK();
  }
}