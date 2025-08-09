#include "LoRaHandler.h"
#include "config.h"
#include "Utils.h"
#include "SX1262_Settings.h"

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

// =============================================================================
// LORA HANDLER IMPLEMENTATION
// =============================================================================

bool initLoRaHandler() {
    SPI.begin();
    
    if (LT.begin(NSS, NRESET, RFBUSY, DIO1, LORA_DEVICE)) {
        debugPrint("LoRa Device found");
        configureSX1262();
        return true;
    } else {
        errorPrint("No LoRa device responding");
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
    
    debugPrint("SX1262 LoRa module configured");
}

bool processLoRaMessage(SensorData &data) {

    

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
        
        // Parse the message and populate sensor data
        parseLoRaMessage(packet, data);
        
        // Set RSSI and SNR values
        data.rssi = PacketRSSI;
        data.snr = PacketSNR;
        
        // Update last packet string for web interface
        lastPacket = String(millis() / 1000) + "s," + packet + "," + String(PacketRSSI) + "dBm," + String(PacketSNR) + "dB";
        
        return true;
    }
}

void sendLoRaCommand(String cmd) {
    Serial.print("[CMD] > ");
    Serial.println(cmd);
    
    Serial2.print(cmd + "\r\n");
    delay(300);
    
    while (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        Serial.print("[RESP] < ");
        Serial.println(response);
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