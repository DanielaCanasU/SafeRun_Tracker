#include "FirebaseHandler.h"
#include "Config.h"
#include "Utils.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <addons/TokenHelper.h>
#include <HTTPClient.h>

#define SUBIR_ACELEROMETRO

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// =============================================================================
// FIREBASE HANDLER IMPLEMENTATION
// =============================================================================

bool initFirebaseHandler() {
    // Deprecated behavior: do not auto-connect WiFi here anymore.
    // Object setup is done before begin.
    // Buffer sizes will be set in startFirebase just before Firebase.begin().
    return true;
}

bool startFirebase() {
    if (WiFi.status() != WL_CONNECTED) {
        errorPrint("WiFi not connected. Cannot start Firebase.");
        // Intentar liberar memoria
        forceGarbageCollection();
        delay(1000);
        return false;
    }
    
    // Verificar memoria antes de iniciar Firebase
    // Firebase.begin() can consume a lot of heap for TLS buffers.
    // 40KB is a safer threshold.
    if (ESP.getFreeHeap() < 40000) {
        errorPrint("Memoria insuficiente para iniciar Firebase");
        Serial.printf("Memoria libre: %d bytes\n", ESP.getFreeHeap());
        return false;
    }
    
    debugPrint("Configurando Firebase...");
    
    // Set buffer sizes just before starting Firebase.
    // The previous values (512, 256) might be too small for a stable TLS handshake.
    // Let's try with slightly larger, more balanced values.
    // RX buffer is for incoming data, TX for outgoing.
    // If you still have issues, you might need to experiment with these.
    fbdo.setBSSLBufferSize(4096, 1024); // RX, TX
    fbdo.setResponseSize(2048);
    
    // Configurar Firebase con delays para evitar problemas
    config.api_key = FIREBASE_API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.token_status_callback = tokenStatusCallback;
    
    // Pequeño delay antes de iniciar
    delay(10000);
    Serial.println("hola");

    Firebase.reconnectNetwork(true);
    Serial.println("hola");
    Serial.println("Memoria libre ANTES de Firebase.begin():");
    Serial.println(ESP.getFreeHeap());
    Firebase.begin(&config, &auth);
    Serial.println("Memoria libre DESPUES de Firebase.begin():");
    Serial.println(ESP.getFreeHeap());
    Serial.println("hola");

    // Pequeño delay después de iniciar
    delay(100);
    
    debugPrint("Firebase started");
    return true;
}

bool setupWiFi() {
    Serial.print("\nConectando a WiFi: ");
    Serial.println(WIFI_SSID);
    
    // Limpiar cualquier conexión previa
    WiFi.disconnect(true);
    delay(1000);
    
    // Configurar modo WiFi
    WiFi.mode(WIFI_STA);
    
    // Intentar conexión
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    const int maxAttempts = 60; // 30 seconds (500ms * 60)
    
    Serial.println("Esperando conexión WiFi...");
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        // Verificar si hay problemas de memoria
        if (ESP.getFreeHeap() < 10000) { // Menos de 10KB libres
            Serial.println("\nAdvertencia: Memoria baja durante conexión WiFi");
            delay(1000);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        successPrint("WiFi conectado. Dirección IP: " + WiFi.localIP().toString());
        Serial.printf("Memoria libre después de conexión: %d bytes\n", ESP.getFreeHeap());
        // Disable WiFi power saving to improve stability, at the cost of higher power consumption.
        // This can sometimes help with random disconnects or crashes.
        WiFi.setSleep(false);
        return true;
    } else {
        errorPrint("WiFi connection failed after " + String(attempts) + " attempts");
        Serial.printf("Estado WiFi: %d\n", WiFi.status());
        Serial.printf("Memoria libre: %d bytes\n", ESP.getFreeHeap());
        return false;
    }
}

#define SUBIR_ACELEROMETRO  // Comentar para quitar la subida de acelerómetro
bool publishAccelDataToFirebase(const SensorData& data) {
    if (!isFirebaseReady()) {
        errorPrint("Firebase not ready");
        return false;
    }
    // Solo subir si state == 1
    if (data.state != 1) {
        debugPrint("No se sube acelerómetro porque state != 1");
        return false;
    }
    FirebaseJson content;
    obtenerTimestamp();
    String documentPath = "acelerometroData/" + String(DEVICE_ID) + "/data/" + String(timestamp);
    content.set("fields/accelX/doubleValue", data.accelX);
    content.set("fields/accelY/doubleValue", data.accelY);
    content.set("fields/accelZ/doubleValue", data.accelZ);
    content.set("fields/receivedAt/integerValue", data.receivedAt);
    content.set("fields/myLatLng/geoPointValue/latitude", data.latitude);
    content.set("fields/myLatLng/geoPointValue/longitude", data.longitude);
    content.set("fields/sensor1/booleanValue", data.sensor1);
    content.set("fields/sensor2/booleanValue", data.sensor2);
    content.set("fields/sensor3/booleanValue", data.sensor3);
    content.set("fields/sensor4/booleanValue", data.sensor4);
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
        successPrint("Datos de acelerómetro enviados correctamente");
        return true;
    } else {
        errorPrint("Error al enviar datos de acelerómetro: " + fbdo.errorReason());
        return false;
    }
}

bool publishSensorDataToFirebase(const SensorData& data) {
    if (!isFirebaseReady()) {
        errorPrint("Firebase not ready");
        return false;
    }
    FirebaseJson content;
    obtenerTimestamp();
    String documentPath = "deviceData/" + String(DEVICE_ID) + "/data/" + String(timestamp);
    debugPrint("Enviando datos a Firestore...");
    Serial.println(data.state);
    content.set("fields/sensor1/booleanValue", data.sensor1);
    content.set("fields/sensor2/booleanValue", data.sensor2);
    content.set("fields/sensor3/booleanValue", data.sensor3);
    content.set("fields/sensor4/booleanValue", data.sensor4);
    content.set("fields/state/booleanValue", data.state);
    content.set("fields/rssi/integerValue", data.rssi);
    content.set("fields/snr/doubleValue", data.snr);
    content.set("fields/myLatLng/geoPointValue/latitude", data.latitude);
    content.set("fields/myLatLng/geoPointValue/longitude", data.longitude);
#ifdef SUBIR_ACELEROMETRO
    // Solo sube si hay datos válidos
    if (data.accelX != 0.0f || data.accelY != 0.0f || data.accelZ != 0.0f) {
        publishAccelDataToFirebase(data);
    }
#endif
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
        successPrint("Datos enviados correctamente");
        return true;
    } else {
        errorPrint("Error al enviar datos: " + fbdo.errorReason());
        return false;
    }
}

bool checkForLinkRequest() {
    String documentPath = "linkRequests/" + String(DEVICE_ID);
    debugPrint("Checking for link request at: " + documentPath);
    
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
        if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
            debugPrint("Solicitud encontrada: " + fbdo.payload());
            return true;
        } else if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_NOT_FOUND) {
            debugPrint("No hay solicitud pendiente.");
            return false;
        } else {
            errorPrint("Error al consultar: " + String(fbdo.httpCode()) + " - " + fbdo.errorReason());
            return false;
        }
    } else {
        errorPrint("Firebase.Firestore.getDocument failed: " + fbdo.errorReason());
        return false;
    }
}

bool asociarLinkRequest() {
    // Paso 1: Leer solicitud para obtener requestedBy
    String requestDocumentPath = "linkRequests/" + String(DEVICE_ID);
    debugPrint("Reading link request to get requestedBy from: " + requestDocumentPath);
    
    if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", requestDocumentPath.c_str())) {
        errorPrint("Error al obtener solicitud: " + fbdo.errorReason());
        return false;
    }
    
    if (fbdo.httpCode() != FIREBASE_ERROR_HTTP_CODE_OK) {
        errorPrint("Solicitud no encontrada o error: " + String(fbdo.httpCode()) + " - " + fbdo.errorReason());
        return false;
    }
    
   
    DynamicJsonDocument doc(4096); // Aumentado para evitar desbordamiento
    DeserializationError error = deserializeJson(doc, fbdo.payload());
    
    if (error) {
        errorPrint("Error al parsear JSON: " + String(error.f_str()));
        return false;
    }
    
    const char* userId = doc["fields"]["requestedBy"]["stringValue"];
    if (!userId) {
        errorPrint("No se encontró requestedBy en la solicitud");
        return false;
    }
    
    debugPrint("Usuario que solicitó: " + String(userId));
    
   
    // Paso 2: Agregar deviceId a la lista de dispositivos del usuario
    String userDocumentPath = "users/" + String(userId);
    
    // Paso 1: Leer documento del usuario
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", userDocumentPath.c_str())) {
        if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
            // El documento existe, leer dispositivos existentes y añadir el nuevo
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, fbdo.payload());
            
            if (!error && doc.containsKey("fields") && doc["fields"].containsKey("devices")) {
                // Ya tiene dispositivos, verificar si el dispositivo ya está en la lista
                JsonArray devices = doc["fields"]["devices"]["arrayValue"]["values"];
                bool deviceExists = false;
                
                for (JsonObject device : devices) {
                    if (device.containsKey("stringValue") && strcmp(device["stringValue"], DEVICE_ID) == 0) {
                        deviceExists = true;
                        break;
                    }
                }
                
                if (deviceExists) {
                    successPrint("Dispositivo ya está vinculado al usuario.");
                    // Aunque ya exista, procedemos a aceptar y borrar la solicitud.
                } else {
                    // Construir array con dispositivos existentes + nuevo
                    FirebaseJsonArray newDevices;
                    newDevices.add(DEVICE_ID);
                    for (JsonObject device : devices) {
                        newDevices.add(device["stringValue"].as<const char*>());
                    }
                    
                    FirebaseJson updateContent;
                    updateContent.set("fields/devices", newDevices);
                    
                    if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", userDocumentPath.c_str(), updateContent.raw(), "devices")) {
                         errorPrint("Error al actualizar array de dispositivos: " + fbdo.errorReason());
                         return false;
                    }
                    successPrint("Dispositivo añadido a la lista del usuario.");
                }
            } 
        }
    }
    
    // Si el documento no existe o no tiene dispositivos, crear uno nuevo
    FirebaseJson payload;
    payload.set("fields/devices/arrayValue/values/[0]/stringValue", DEVICE_ID);
    
    debugPrint("Creating new user document with device");
    
    // Usamos patchDocument con merge para crear o actualizar el campo.
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", userDocumentPath.c_str(), payload.raw(), "")) {
        successPrint("Dispositivo vinculado al usuario (documento creado).");
    } else {
        errorPrint("Error al crear/actualizar documento de usuario: " + fbdo.errorReason());
        return false;
    }
    return true;
}

bool acceptLinkRequest() {
    String documentPath = "linkRequests/" + String(DEVICE_ID);
    FirebaseJson content;
    content.set("fields/status/stringValue", "accepted");
    
    debugPrint("Updating link request status to accepted at: " + documentPath);
    
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "status")) {
        successPrint("Solicitud aceptada correctamente.");
        return true;
    } else {
        errorPrint("Error al aceptar: " + fbdo.errorReason());
        return false;
    }
}

bool deleteLinkRequest() {
    String documentPath = "linkRequests/" + String(DEVICE_ID);
    debugPrint("Deleting link request at: " + documentPath);
    
    if (Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
        successPrint("Solicitud eliminada.");
        return true;
    } else {
        errorPrint("Error al eliminar solicitud: " + fbdo.errorReason());
        return false;
    }
}

bool processLinkRequest() {
    debugPrint("Processing complete link request workflow");
    
    if (!asociarLinkRequest()) {
        return false;
    }
    
    delay(200);
    
    if (!acceptLinkRequest()) {
        return false;
    }
    
    delay(200);
    
    if (!deleteLinkRequest()) {
        return false;
    }
    
    successPrint("Link request workflow completed successfully");
    return true;
}

bool isFirebaseReady() {
    return Firebase.ready();
}

String getFirebaseError() {
    return fbdo.errorReason();
}



String getFirestoreTimestamp() {
    time_t now = time(nullptr);
    struct tm* t = gmtime(&now);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", t);
    return String(buf);
}

void startTrainingSession(const String& deviceId, const String& sessionId, const SensorData& data) {
    FirebaseJson content;
    content.set("fields/deviceId/stringValue", deviceId);
    content.set("fields/startTime/timestampValue", getFirestoreTimestamp());
    String documentPath = "trainingSessions/" + deviceId + "/sessions/" + sessionId;
    Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw());
    // Generar pointId (HHMMSS)
    char pointIdBuf[7];
    obtenerHoraId(pointIdBuf, sizeof(pointIdBuf));
    uploadSessionPoint(deviceId, sessionId, data, String(pointIdBuf));
}

void uploadSessionPoint(const String& deviceId, const String& sessionId, const SensorData& data, const String& pointId) {
    FirebaseJson content;
    content.set("fields/timestamp/timestampValue", getFirestoreTimestamp());
    content.set("fields/latitude/doubleValue", data.latitude);
    content.set("fields/longitude/doubleValue", data.longitude);
    content.set("fields/state/integerValue", data.state);
    String documentPath = "trainingSessions/" + deviceId + "/sessions/" + sessionId + "/points/" + pointId;
    Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw());
}

void endTrainingSession(const String& deviceId, const String& sessionId, const String& endTime) {
    FirebaseJson content;
    content.set("fields/endTime/timestampValue", endTime);
    String documentPath = "trainingSessions/" + deviceId + "/sessions/" + sessionId;
    Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "endTime");
}

bool publishAlertToFirestore(const SensorData& data) {
    if (!isFirebaseReady()) {
        errorPrint("Firebase not ready");
        return false;
    }
    FirebaseJson content;
    obtenerTimestamp();
    String alertId = String(timestamp); // Usa el timestamp como ID único
    String documentPath = "alerts/" + String(DEVICE_ID) + "/alerts/" + alertId;

    content.set("fields/timestamp/timestampValue", getFirestoreTimestamp());
    content.set("fields/location/mapValue/fields/latitude/doubleValue", data.latitude);
    content.set("fields/location/mapValue/fields/longitude/doubleValue", data.longitude);
    content.set("fields/sensor4/integerValue", data.state);

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
        successPrint("Alerta enviada correctamente");
        return true;
    } else {
        errorPrint("Error al enviar alerta: " + fbdo.errorReason());
        return false;
    }
}
