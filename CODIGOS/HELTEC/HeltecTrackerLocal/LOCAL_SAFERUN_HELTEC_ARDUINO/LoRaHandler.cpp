#include "LoRaHandler.h"
#include "Config.h"
#include "SX1262_Settings.h"
#include "SensorData.h"
#include <SPI.h>
#include "AES.h"  // Librería AES de Matej Sychra
#include "UI.h"   // Incluir para notificar a la UI de nuevos datos

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
SX126XLT LT;  //create a library class instance called LT

// =============================================================================
// PRIVATE VARIABLES
// =============================================================================
static uint32_t RXpacketCount = 0;
static uint32_t errors = 0;
static uint8_t RXBUFFER[RXBUFFER_SIZE];
static uint8_t RXPacketL = 0;
static int8_t PacketRSSI = 0;
static int8_t PacketSNR = 0;
static String lastPacket = "";

// Clave y vector de inicialización para AES (16 bytes cada uno)
static uint8_t aes_key[16] = { 'S', 'A', 'F', 'E', 'R', 'U', 'N', 'C', 'I', 'F', 'R', 'A', 'D', 'O', '1', '2' };
static uint8_t aes_iv[16]  = { 'I', 'n', 'i', 'c', 'i', 'a', 'l', 'I', 'V', '1', '2', '3', '4', '5', '6', '7'};
AES aes;


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
    texto.getBytes(plainBuffer, paddedLength);

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

    const int cipherLength = encodedLength / 2;
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

// =============================================================================
// LORA HANDLER IMPLEMENTATION
// =============================================================================

bool initLoRaHandler() {
    SPI.begin();
    
    if (LT.begin(NSS, NRESET, RFBUSY, DIO1, LORA_DEVICE)) {
        Serial.println("LoRa Device found");
        configureSX1262();
        return true;
    } else {
        Serial.println("No LoRa device responding");
        return false;
    }
}

void configureSX1262() {
    //Setup LoRa device
    LT.setMode(MODE_RX);
    LT.setRegulatorMode(USE_DCDC);
    LT.setPaConfig(0x04, PAAUTO, LORA_DEVICE);
    LT.setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
    LT.calibrateDevice(ALLDevices);  //is required after setting TCXO
    LT.calibrateImage(Frequency);
    LT.setDIO2AsRfSwitchCtrl();
    LT.setPacketType(PACKET_TYPE_LORA);
    LT.setRfFrequency(Frequency, Offset);
    LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);
    LT.setBufferBaseAddress(0, 0);
    LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
    LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
    LT.setHighSensitivity();
    LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
    
    Serial.println("SX1262 LoRa module configured");
}

bool processLoRaMessage(SensorData &data) { // El parámetro 'data' se mantiene por compatibilidad
    Serial.println("Entro");
    RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, LORA_TIMEOUT, WAIT_RX);
    PacketRSSI = LT.readPacketRSSI();
    PacketSNR = LT.readPacketSNR();
    
    Serial.print("Received ");
    
    if (RXPacketL == 0) {
        uint16_t IRQStatus = LT.readIrqStatus();
        
        if (IRQStatus & IRQ_RX_TIMEOUT) {
            Serial.print(F(" RXTimeout"));
            // No incrementes errores por timeout, es normal si no hay transmisión
        } else if (IRQStatus & (IRQ_HEADER_ERROR | IRQ_CRC_ERROR)) {
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
        Serial.println(packet);
        // Descifra el mensaje recibido
        String packetDescifrado = descifrarValor(packet);
        Serial.println(packetDescifrado);
        // Parse the message and populate sensor data
        SensorData tempData;
        initSensorData(tempData); // Inicializar con valores por defecto

        if (parseLoRaMessage(packetDescifrado, tempData)) {
            // El parseo fue exitoso

            // Añadir RSSI y SNR a los datos
            tempData.rssi = PacketRSSI;
            tempData.snr = PacketSNR;

            // Actualizar el struct 'data' pasado por referencia (para compatibilidad con el loop principal)
            data = tempData;

            // --- CONEXIÓN EXPLÍCITA CON LA UI ---
            // Notificar directamente a la UI y otros módulos sobre los nuevos datos.
            // Esto hace que la actualización sea inmediata y visible en la pantalla "Remoto".
            updateLocalLoRaData(tempData);
            updateTrackingData(tempData);
            // Actualizar el string del último paquete para otras interfaces (si se usa)
            lastPacket = String(millis() / 1000) + "s," + packetDescifrado + "," + String(PacketRSSI) + "dBm," + String(PacketSNR) + "dB";
            
            return true;
        } else {
            // El parseo falló
            Serial.println("Error: Fallo al parsear el mensaje LoRa.");
            return false;
        }
    }
}

uint8_t getLastPacketInfo(int8_t* rssi, int8_t* snr) {
    if (rssi) *rssi = PacketRSSI;
    if (snr) *snr = PacketSNR;
    return RXPacketL;
}

void getPacketStats(uint32_t* packetCount, uint32_t* errorCount) {
    if (packetCount) *packetCount = RXpacketCount;
    if (errorCount) *errorCount = errors;
}

void printPacketStats() {
    Serial.printf("Packets received: %lu\n", RXpacketCount);
    Serial.printf("Errors: %lu\n", errors);
    Serial.printf("Success rate: %.2f%%\n", 
        (RXpacketCount > 0) ? ((float)(RXpacketCount - errors) / RXpacketCount) * 100.0f : 0.0f);
}

String getLastPacketString() {
    return lastPacket;
}