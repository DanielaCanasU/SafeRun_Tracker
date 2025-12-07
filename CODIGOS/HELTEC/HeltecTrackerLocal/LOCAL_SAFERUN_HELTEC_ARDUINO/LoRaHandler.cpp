#include "LoRaHandler.h"
#include "Config.h"
#include "SX1262_Settings.h"
#include "SensorData.h"
#include <SPI.h>
#include "AES.h"  // Librería AES de Matej Sychra
#include "UI.h" 
#include "AppState.h"  // Incluir para notificar a la UI de nuevos datos
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

void LoRa::init() {
    // Solo inicializar hardware, NO configurar parámetros de modulación
    // La configuración se hará cuando se seleccione la distancia en el menú
    SPI.begin();
    if (LT.begin(NSS, NRESET, RFBUSY, DIO1, LORA_DEVICE)) {
        Serial.println("--------------------------------");
        Serial.println("LORA INICIADO CORRECTAMENTE");
        Serial.println("--------------------------------");
        delay(1000);
        LT.setMode(MODE_STDBY_RC);
        LT.setRegulatorMode(USE_DCDC);
        LT.setPaConfig(0x04, PAAUTO, LORA_DEVICE);
        LT.setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
        LT.calibrateDevice(ALLDevices);  //is required after setting TCXO
        LT.calibrateImage(Frequency);
        LT.setDIO2AsRfSwitchCtrl();
        LT.setPacketType(PACKET_TYPE_LORA);
        LT.setRfFrequency(Frequency, Offset);
        // NO configurar parámetros de modulación aquí - se hará en configureSX1262ForDistance()
        LT.setBufferBaseAddress(0, 0);
        // NO configurar packet params aquí - se hará en configureSX1262ForDistance()
        // NO configurar IRQ params aquí - se hará en configureSX1262ForDistance()
        // NO configurar sensitivity ni sync word aquí - se hará en configureSX1262ForDistance()
        
        // Configurar con la distancia por defecto al inicializar
        configureSX1262ForDistance(selectedExerciseDistance);
    } else {
        Serial.println("--------------------------------");
        Serial.println("ERROR INICIANDO LORA");
        Serial.println("--------------------------------");
    }
}

void LoRa::checkIncomeMessage() {
    unsigned long now = millis();
    // LoRa reception logic (exactly like ESP32)
    if(!WAITING_LORA) {
        // Asegurarse de que el radio esté en modo standby antes de configurar RX
        LT.setMode(MODE_STDBY_RC);
        delay(5);
        LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on RX done or timeout
        LT.setRx(LORA_TIMEOUT);
        WAITING_LORA = true;
    }
    
    // Process LoRa messages
    if (now - lastLoraCheck >= LORA_CHECK_INTERVAL) {
        if(digitalRead(14)) {
                processLoRaMessage(datos.currentData);
                Serial.println("Valid LoRa message received");
                printSensorData(datos.currentData);
                
                // Update tracking system with received data
                updateTrackingData(datos.currentData);
                
                WAITING_LORA = false;
                ACKenviado = false;
        }   
        lastLoraCheck = now;
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
        return true;
    } else {
        Serial.println("No LoRa device responding");
        return false;
    }
}

void configureSX1262() {
    // Función legacy - ahora usa configureSX1262ForDistance con distancia por defecto
    // Verificar que selectedExerciseDistance esté inicializado
    if (selectedExerciseDistance >= DISTANCE_COUNT) {
        selectedExerciseDistance = DISTANCE_1_5KM; // Valor por defecto seguro
    }
    configureSX1262ForDistance(selectedExerciseDistance);
}

void configureSX1262ForDistance(ExerciseDistance distance) {
    // Verificar que distance esté en rango válido
    if (distance >= DISTANCE_COUNT) {
        Serial.println("ERROR: distance fuera de rango, usando 1.5km por defecto");
        distance = DISTANCE_1_5KM;
    }
    
    uint8_t sf;
    uint8_t cr;
    
    // Configurar según la distancia seleccionada
    // Usar valores que sabemos que funcionan (basados en las constantes existentes)
    switch(distance) {
        case DISTANCE_500M:
            // 500m: SF más pequeño posible, CR 4/5
            // Nota: Si LORA_SF5 no existe, usar valor numérico 5
            sf = LORA_SF5;
            cr = LORA_CR_4_5;
            Serial.println("Configurando SX1262 para 500m: SF5, CR 4/5");
            break;
        case DISTANCE_1KM:
            // 1km: SF 10, CR 4/5
            // Nota: Si LORA_SF10 no existe, usar valor numérico 10
            sf = LORA_SF10;
            cr = LORA_CR_4_5;
            Serial.println("Configurando SX1262 para 1km: SF10, CR 4/5");
            break;
        case DISTANCE_1_5KM:
        default:
            // 1.5km: Configuración actual (SF12, CR 4/8) - estas constantes sabemos que existen
            sf = LORA_SF12;
            cr = LORA_CR_4_8;
            Serial.println("Configurando SX1262 para 1.5km: SF12, CR 4/8");
            break;
    }
    
    // Aplicar configuración con verificaciones de seguridad
    // IMPORTANTE: Detener cualquier operación en curso antes de reconfigurar
    LT.setMode(MODE_STDBY_RC);
    delay(50); // Delay más largo para asegurar que el modo se estableció completamente
    
    // Limpiar cualquier IRQ pendiente antes de reconfigurar
    LT.clearIrqStatus(IRQ_RADIO_ALL);
    
    // Aplicar parámetros de modulación
    LT.setModulationParams(sf, Bandwidth, cr, Optimisation);
    LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
    LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
    LT.setHighSensitivity();
    LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
    
    // IMPORTANTE: Reiniciar el estado de recepción
    // Esto asegura que el flag WAITING_LORA se resetee para que checkIncomeMessage() 
    // pueda iniciar una nueva recepción
    lora.WAITING_LORA = false;
    
    // Configurar para recepción - usar setRx directamente en lugar de MODE_RX
    // Esto asegura que el timeout se configure correctamente
    LT.setMode(MODE_STDBY_RC);
    delay(10);
    LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
    LT.setRx(LORA_TIMEOUT);
    
    Serial.print("SX1262 configurado exitosamente: SF=");
    Serial.print(sf);
    Serial.print(", CR=");
    Serial.println(cr);
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
        Serial.print("Paquete recibido completo: ");
        Serial.println(packet);
        
        // Extraer IDs sin cifrar (formato: FROM:deviceId;TO:targetId;DATA:cifrado)
        //String fromId = "";
        String toId = "";
        String numberACK = "";
        String dataPart = "";
        
        // Buscar marcadores en el formato: FROM:deviceId;TO:targetId;DATA:cifrado
        int fromIndex = packet.indexOf("FROM:");
        int toIndex = packet.indexOf("TO:");
        int numberACKIndex = packet.indexOf("ACK:");
        int dataIndex = packet.indexOf("DATA:");
        
        if (fromIndex == -1 || toIndex == -1 || dataIndex == -1 || numberACKIndex == -1) {
            Serial.println("Error: Formato de mensaje incorrecto. No se encontraron IDs.");
            return false;
        }
        
        // Verificar que los índices estén en orden correcto
        if (fromIndex >= toIndex || toIndex >= numberACKIndex || numberACKIndex >= dataIndex) {
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
        lora.lastFromID = packet.substring(fromIndex + 5, fromSemicolon);

        // Extraer TO ID: desde después de "TO:" hasta antes de ";DATA:"
        // Buscar el punto y coma después de TO:
        int toSemicolon = packet.indexOf(';', toIndex + 3);
        if (toSemicolon == -1 || toSemicolon > numberACKIndex) {
            Serial.println("Error: No se encontró delimitador después de TO:");
            return false;
        }
        toId = packet.substring(toIndex + 3, toSemicolon);
        
        // Extraer TO ID: desde después de "TO:" hasta antes de ";DATA:"
        // Buscar el punto y coma después de TO:
        int ackSemicolon = packet.indexOf(';', numberACKIndex + 4);
        if (ackSemicolon == -1 || ackSemicolon > dataIndex) {
            Serial.println("Error: No se encontró delimitador después de TO:");
            return false;
        }
        numberACK = packet.substring(numberACKIndex + 4, ackSemicolon);
        lora.lastACKreceived = int(numberACK.toInt());
        // Extraer DATA (parte cifrada): desde después de "DATA:" hasta el final
        dataPart = packet.substring(dataIndex + 5);
        
        Serial.print("FROM ID: ");
        Serial.println(lora.lastFromID);
        Serial.print("TO ID: ");
        Serial.println(toId);
        Serial.print("ACK: ");
        Serial.println(numberACK);
        Serial.print("DATA (cifrado): ");
        Serial.println(dataPart);

        
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
        String packetDescifrado = descifrarValor(dataPart);
        Serial.print("Mensaje descifrado: ");
        Serial.println(packetDescifrado);
        
        // Parse the message and populate sensor data
        SensorData tempData;
        datos.initPaquete(tempData); // Inicializar con valores por defecto

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
            lastPacket = String(millis() / 1000) + "s,FROM:" + lora.lastFromID + "," + packetDescifrado + "," + String(PacketRSSI) + "dBm," + String(PacketSNR) + "dB";
            
            // Enviar ACK al remitente para confirmar recepción
            ACKenviado = false;
            
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

// Función para enviar ACK (acknowledgment) después de procesar un mensaje exitosamente
// Formato: ACK:FROM:localDeviceId;TO:remoteDeviceId
void sendACK(const String& remoteDeviceId) {
    const unsigned long currentTime = millis();

    String cifrado = cifrarValor(String(lora.lastACKreceived));
    // Construir mensaje ACK sin cifrar (para velocidad y simplicidad)
    String mensajeCompleto = "FROM:" + String(DEVICE_ID) + ";TO:" + remoteDeviceId + ";ACK:" + cifrado;
    

    

    int TXPacketL = mensajeCompleto.length();
    unsigned long startmS = millis();
    delay(10);
    // Asegurar que no estamos en modo RX antes de transmitir
    //WAITING_LORA = false;
    
    /*// Configurar modo TX
    LT.setMode(MODE_STDBY_RC);
    delay(10);
    LT.setTx(100);
    */
    
    if (TRANSMISION_COMPLETADA || TRANSMISION_FALLIDA) {
        LT.transmitDaniela((uint8_t*)mensajeCompleto.c_str(), TXPacketL, 10000, TXpower, true);
        TRANSMISION_COMPLETADA = false; TRANSMISION_FALLIDA = false;
        Serial.println("AQUIIII");
        Serial.print("Enviando ACK: ");
        Serial.println(lora.lastACKreceived);
      } else if (digitalRead(14)) {
        Serial.println("AQUIIII");
        if (LT.readIrqStatus() & IRQ_RX_TX_TIMEOUT){
           TRANSMISION_FALLIDA = true;}
           else {
            TRANSMISION_COMPLETADA = true;
            ACKenviado = true;
            lastSendTime_ACK = currentTime;
            //lora.lastACKreceived++;
            Serial.println("CORRECTA");
            //LT.setMode(MODE_STDBY_RC);
            //LT.setRx(10000);
            }
      }
      if (TRANSMISION_COMPLETADA) {
        unsigned long endmS = millis();

      }
      if (TRANSMISION_FALLIDA) {
        uint16_t IRQStatus = LT.readIrqStatus();
        LT.printIrqStatus();
      }
}