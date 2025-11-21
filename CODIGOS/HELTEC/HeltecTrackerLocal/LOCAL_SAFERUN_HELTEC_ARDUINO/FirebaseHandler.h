#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "SensorData.h"

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize Firebase handler
 * @return true if initialization successful, false otherwise
 */
bool initFirebaseHandler();

// Initialize Firebase objects after WiFi is connected
bool startFirebase();

/**
 * @brief Setup WiFi connection
 * @return true if connection successful, false otherwise
 */
bool setupWiFi();

/**
 * @brief Publish sensor data to Firebase
 * @param data Reference to SensorData struct to publish
 * @return true if publish successful, false otherwise
 */
bool publishSensorDataToFirebase(const SensorData& data);

/**
 * @brief Check for pending link requests
 * @return true if request found, false otherwise
 */
bool checkForLinkRequest();

/**
 * @brief Associate device with user (arrayUnion operation)
 * @return true if association successful, false otherwise
 */
bool asociarLinkRequest();

/**
 * @brief Accept link request by updating status
 * @return true if acceptance successful, false otherwise
 */
bool acceptLinkRequest();

/**
 * @brief Delete link request after processing
 * @return true if deletion successful, false otherwise
 */
bool deleteLinkRequest();

/**
 * @brief Process complete link request workflow
 * @return true if all steps successful, false otherwise
 */
bool processLinkRequest();

/**
 * @brief Check if Firebase is ready
 * @return true if ready, false otherwise
 */
bool isFirebaseReady();

/**
 * @brief Get Firebase error reason
 * @return Error reason string
 */
String getFirebaseError();

/**
 * @brief Get Firestore timestamp in ISO format
 * @return Timestamp string in ISO format
 */
String getFirestoreTimestamp();

/**
 * @brief Publish an alert to Firestore in alerts/{deviceId}/
 * @param data Reference to SensorData struct to use for the alert
 * @return true if publish successful, false otherwise
 */
bool publishAlertToFirestore(const SensorData& data);

/**
 * @brief Inicia una nueva sesión de entrenamiento en Firestore bajo trainingSessions/{deviceId}/sessions/{sessionId}
 * @param deviceId El id del dispositivo
 * @param sessionId El id de la sesión (timestamp de inicio)
 * @param data El primer punto de la sesión
 */
void startTrainingSession(const String& deviceId, const String& sessionId, const SensorData& data);

/**
 * @brief Sube un punto a la sesión activa en Firestore
 * @param deviceId El id del dispositivo
 * @param sessionId El id de la sesión
 * @param data El punto a subir
 * @param pointId El id del punto (HHMMSS)
 */
void uploadSessionPoint(const String& deviceId, const String& sessionId, const SensorData& data, const String& pointId);

/**
 * @brief Finaliza la sesión de entrenamiento en Firestore
 * @param deviceId El id del dispositivo
 * @param sessionId El id de la sesión
 * @param endTime Timestamp de fin
 */
void endTrainingSession(const String& deviceId, const String& sessionId, const String& endTime);

// =============================================================================
// EXTERNAL VARIABLES
// =============================================================================
extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

#endif // FIREBASE_HANDLER_H