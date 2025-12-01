#include "UI.h"
#include "Config.h"
#include "AppState.h"
#include "TrackingData.h"
#include "FirebaseHandler.h"
#include <WiFi.h>
#include <Preferences.h>
#include "SensorData.h"
// Función robusta de mapeo flotante
int mapf(float value, float in_min, float in_max, int out_min, int out_max);

// Variables globales para datos LoRa (deben estar antes de cualquier uso) hola
static SensorData lastLoRaData = {};
static unsigned long lastLoRaReceived = 0;


static void renderInfoScreen();

// Color definitions for improved UI
#define NARANJA ST7735_COLOR565(243, 91, 4)
#define AZUL_OSCURO ST7735_COLOR565(2, 48, 71)
#define MORADO ST7735_COLOR565(131, 56, 236)
#define ST7735_GRAY ST7735_COLOR565(128, 128, 128)

// Menu state (mirrors ESP32 OLEDMenu)
enum class MenuScreen { Welcome, SelectMode, MenuPrincipal, WiFiSubMenu, WiFiScan, WiFiSelectSSID, WiFiEnterPassword, WiFiConnecting, WiFiStatus, Info, LocalData, Tracking, Backtrack, WaypointManager, Pairing, BacktrackMap };
static MenuScreen currentScreen = MenuScreen::Welcome;
static bool wifiConnected = false;

// Buttons active LOW
static bool readBtnUp()   { return digitalRead(BTN_UP_PIN)   == LOW; }
static bool readBtnDown() { return digitalRead(BTN_DOWN_PIN) == LOW; }
static bool readBtnOk()   { return digitalRead(BTN_OK_PIN)   == LOW; }
static bool readBtnBack() { return digitalRead(BTN_BACK_PIN) == LOW; }

// Debounce
static unsigned long lastBtnMs = 0;
static const unsigned long btnDebounceMs = 180;

static bool canReadButtons(unsigned long now) {
  if (now - lastBtnMs >= btnDebounceMs) { lastBtnMs = now; return true; }
  return false;
}

// WiFi scan state
static int16_t numNetworks = 0;
static int selectedNetworkIdx = 0;
static String selectedSSID = "";
static String enteredPassword = "";
static bool wifiScanStarted = false;
static int MenuPrincipalIdx = 0; // 0 = Escanear redes, 1 = Estado
static bool wifiConnectingActive = false;
static unsigned long wifiConnLastCheckMs = 0;
static int wifiConnTries = 0;

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.*";
static const int allowedCharsCount = sizeof(allowedChars) - 1;
static int charIndex = 0;

// Cache for optimized rendering
static MenuScreen lastRenderedScreen = MenuScreen::Info;
static bool lastRemoteMode = false;
static int lastMenuPrincipalIdx = -1;
static String lastWifiStatus = "";
static int lastSelectedNetworkIdx = -1;
static String lastEnteredPassword = "";
static int lastCharIndex = -1;
static bool lastWifiConnected = false;
static String lastWifiIP = "";
static int lastWifiConnTries = -1;

// Sensor data cache for Info screen
static float lastLatitude = -999.0f;
static float lastLongitude = -999.0f;
static int lastRssi = -999;
static float lastSnr = -999.0f;
static bool lastSensor4 = false;

// Add main menu state variables
static int mainMenuIdx = 0; // 0 = Modo, 1 = WiFi, 2 = Datos Locales, 3 = Info, 4 = Rastreo, 5 = Backtrack, 6 = Emparejar
static int lastMainMenuIdx = -1;

// Pairing system variables
static bool pairingRequestPending = false;
static String pairingDeviceId = "";
static unsigned long lastPairingCheck = 0;
static bool datosRecibidos = false; // Flag para indicar nuevos datos LoRa
static const unsigned long PAIRING_CHECK_INTERVAL = 5000; // 5 segundos

// Tracking system variables
static TrackingData targetData;
static NavigationData currentNavigation;
static bool hasTargetData = false;
static unsigned long lastTargetUpdate = 0;
static const unsigned long TARGET_TIMEOUT = 30000; // 30 segundos timeout

// Emparejamiento
extern const char* userCorreoSolicitante; // Declaración para acceder a la variable de FirebaseHandler
bool lastPantallaEmparejamiento = false; //0 = No hay solicitudes, 1 = Solicitud pendiente
static float localLatitude = 0.0f; // Local GPS position from Heltec device
static float localLongitude = 0.0f;
static bool localPositionSet = false;
static unsigned long lastLocalGPSUpdate = 0;

// Estructura para waypoints del backtrack - ahora definida en SensorData.h
// Sistema de backtrack - waypoints y waypointCount ahora son globales desde SensorData.h/cpp
static int selectedWaypointIndex = -1;

// Variables de scroll para waypoints
static int waypointScrollOffset = 0;

// Cache para minimapa (evitar redibujado innecesario)
static float lastMapLat = -999.0f;
static float lastMapLon = -999.0f;
static int lastMapWaypointCount = -1;
static int lastMapSelectedIdx = -2;

// Navegación hacia waypoint seleccionado
static NavigationData waypointNavigation;
static bool hasWaypointTarget = false;

// Welcome screen variables
static bool welcomeScreenShown = false;
static unsigned long welcomeStartTime = 0;
static int welcomePhase = 0; // 0: welcome message
static const unsigned long WELCOME_DURATION = 2500; // 2.5 segundos

// ===== DECLARACIONES DE FUNCIONES DEL SISTEMA DE BACKTRACK =====
static void initWaypointPrefs();
//static void loadWaypointsFromStorage();
static void saveWaypointsToStorage();
static void updateWaypointNavigation();

// ===== FORWARD DECLARATIONS FOR UI FUNCTIONS =====
void renderWelcomeScreen();
bool isLocalGPSAvailable();
void saveCurrentPositionAsWaypoint(const String& name);
void deleteWaypoint(int index);
void selectWaypoint(int index);
void checkPairingRequests();
void rejectPairing();
void acceptPairing();

Display::Display() {
}

void Display::init() {
  st7735.st7735_init();

  welcomeStartTime = millis();
  welcomePhase = 0;
  currentScreen = MenuScreen::Welcome;

  renderWelcomeScreen();

  Serial.println("--------------------------------");
  Serial.println("PANTALLA INICIADO CORRECTAMENTE");
  Serial.println("--------------------------------");
}


// Drawing helper functions
static void drawCircle(int x, int y, int radius, uint16_t color) {
  for (int dy = -radius; dy <= radius; dy++) {
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx*dx + dy*dy <= radius*radius) {
        st7735.st7735_draw_pixel(x + dx, y + dy, color);
      }
    }
  }
}

// Función para dibujar el contorno de un círculo (algoritmo de punto medio)
static void drawCircleOutline(int xc, int yc, int r, uint16_t color) {
    int x = r, y = 0;
    int P = 1 - r;
    while (x > y) {
        y++;
        if (P <= 0) P = P + 2*y + 1;
        else { x--; P = P + 2*y - 2*x + 1; }
        if (x < y) break;
        st7735.st7735_draw_pixel(xc + x, yc + y, color); st7735.st7735_draw_pixel(xc - x, yc + y, color);
        st7735.st7735_draw_pixel(xc + x, yc - y, color); st7735.st7735_draw_pixel(xc - x, yc - y, color);
        if (x != y) {
            st7735.st7735_draw_pixel(xc + y, yc + x, color); st7735.st7735_draw_pixel(xc - y, yc + x, color);
            st7735.st7735_draw_pixel(xc + y, yc - x, color); st7735.st7735_draw_pixel(xc - y, yc - x, color);
        }
    }
}

static void drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;
  
  while (true) {
    st7735.st7735_draw_pixel(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x1 += sx; }
    if (e2 < dx) { err += dx; y1 += sy; }
  }
}

static void fillRectPixels(int x, int y, int w, int h, uint16_t color) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      st7735.st7735_draw_pixel(x + xx, y + yy, color);
    }
  }
}

// Función para dibujar una brújula simple
static void drawCompass(int centerX, int centerY, int radius, float bearing, uint16_t color) {
  // Variable estática para recordar la posición anterior de la flecha
  static float lastBearing = -1.0f;
  static int lastArrowX = -1, lastArrowY = -1;
  static int lastTip1X = -1, lastTip1Y = -1, lastTip2X = -1, lastTip2Y = -1;
  
  // Solo limpiar la flecha anterior si es la primera vez o si cambió la dirección
  if (lastBearing >= 0.0f && lastBearing != bearing) {
    // Limpiar solo el área de la flecha anterior (pequeño rectángulo)
    int arrowClearX = min(lastArrowX, min(lastTip1X, lastTip2X)) - 2;
    int arrowClearY = min(lastArrowY, min(lastTip1Y, lastTip2Y)) - 2;
    int arrowClearW = max(lastArrowX, max(lastTip1X, lastTip2X)) - arrowClearX + 4;
    int arrowClearH = max(lastArrowY, max(lastTip1Y, lastTip2Y)) - arrowClearY + 4;
    
    // Limpiar área de la flecha anterior
    for (int y = arrowClearY; y < arrowClearY + arrowClearH; y++) {
      for (int x = arrowClearX; x < arrowClearX + arrowClearW; x++) {
        if (x >= 0 && x < 128 && y >= 0 && y < 160) {
          st7735.st7735_draw_pixel(x, y, ST7735_BLACK);
        }
      }
    }
  }
  
  // Dibujar círculo exterior
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * PI / 180.0;
    int x1 = centerX + (radius - 2) * cos(rad);
    int y1 = centerY + (radius - 2) * sin(rad);
    int x2 = centerX + radius * cos(rad);
    int y2 = centerY + radius * sin(rad);
    drawLine(x1, y1, x2, y2, color);
  }
  
  // Dibujar flecha de dirección
  // Ajustar el ángulo para que coincida con el sistema de coordenadas de la pantalla
  // En pantalla: 0° = arriba (N), 90° = derecha (E), 180° = abajo (S), 270° = izquierda (W)
  // El bearing viene en grados donde 0° = N, 90° = E, 180° = S, 270° = W
  // Necesitamos convertir: bearing = 0° debe apuntar hacia arriba en pantalla
  float adjustedBearing = bearing - 90.0; // Rotar 90° para que N apunte hacia arriba
  if (adjustedBearing < 0) adjustedBearing += 360.0;
  
  float arrowRad = adjustedBearing * PI / 180.0;
  int arrowX = centerX + (radius - 5) * cos(arrowRad);
  int arrowY = centerY + (radius - 5) * sin(arrowRad);
  
  // Flecha principal
  drawLine(centerX, centerY, arrowX, arrowY, color);
  
    // Puntas de la flecha
  float tip1Rad = arrowRad - 0.3;
  float tip2Rad = arrowRad + 0.3;
  int tip1X = arrowX - 8 * cos(tip1Rad);
  int tip1Y = arrowY - 8 * sin(tip1Rad);
  int tip2X = arrowX - 8 * cos(tip2Rad);
  int tip2Y = arrowY - 8 * sin(tip2Rad);
     
  drawLine(arrowX, arrowY, tip1X, tip1Y, color);
  drawLine(arrowX, arrowY, tip2X, tip2Y, color);
  
  // Guardar las posiciones actuales para la próxima limpieza
  lastBearing = bearing;
  lastArrowX = arrowX;
  lastArrowY = arrowY;
  lastTip1X = tip1X;
  lastTip1Y = tip1Y;
  lastTip2X = tip2X;
  lastTip2Y = tip2Y;
    
    // Marcas cardinales
    st7735.st7735_write_str(centerX - 3, centerY - radius - 8, "N", Font_7x10, color);
    st7735.st7735_write_str(centerX - 3, centerY + radius + 2, "S", Font_7x10, color);
    st7735.st7735_write_str(centerX + radius + 2, centerY - 3, "E", Font_7x10, color);
    st7735.st7735_write_str(centerX - radius - 8, centerY - 3, "W", Font_7x10, color);
}

// Drawing functions for welcome screen
static void drawEye(int x, int y, int size, bool open, uint16_t color) {
  if (open) {
    // Ojo abierto: círculo con pupila
    drawCircle(x, y, size, color);
    // Pupila (círculo más pequeño en el centro)
    drawCircle(x, y, size/3, ST7735_BLACK);
    // Brillo en el ojo (reflejo)
    drawCircle(x - size/4, y - size/4, size/6, ST7735_WHITE);
    // Pestañas superiores
    for (int i = -size + 1; i < size - 1; i += 2) {
      drawLine(x + i, y - size - 1, x + i, y - size/2, color);
    }
  } else {
    // Ojo cerrado: línea horizontal con pestañas
    drawLine(x - size, y, x + size, y, color);
    // Pestañas superiores más largas
    for (int i = -size + 1; i < size - 1; i += 2) {
      drawLine(x + i, y - size/2, x + i, y - size/4, color);
    }
    // Pestañas inferiores
    for (int i = -size + 1; i < size - 1; i += 2) {
      drawLine(x + i, y + size/4, x + i, y + size/2, color);
    }
  }
}

static void drawBlinkingEyes() {
  // Limpiar área de los ojos
  fillRectPixels(20, 40, 88, 40, ST7735_BLACK);
  
  // Dibujar dos ojos
  int leftEyeX = 45;
  int rightEyeX = 83;
  int eyeY = 60;
  int eyeSize = 12;
  
  drawEye(leftEyeX, eyeY, eyeSize, true, NARANJA); // Ojo abierto
  drawEye(rightEyeX, eyeY, eyeSize, true, NARANJA); // Ojo abierto
  
  // Dibujar nariz simple
  drawLine(64, 70, 64, 75, NARANJA);
  
  // Dibujar boca sonriente
  int mouthY = 85;
  for (int i = 0; i < 8; i++) {
    int x = 60 + i;
    int y = mouthY + (i < 4 ? i : 7 - i);
    drawLine(x, y, x, y + 1, NARANJA);
  }
}

static void drawWelcomeMessage() {
  // Limpiar pantalla
  st7735.st7735_fill_screen(ST7735_BLACK);
  
  // Título principal centrado
  st7735.st7735_write_str(20, 15, "BIENVENIDOS", Font_11x18, NARANJA);
  
  // Subtítulo
  st7735.st7735_write_str(15, 40, "SafeRun Tracker1", Font_7x10, ST7735_WHITE);
  
  // Mensaje adicional centrado
 
  // Línea decorativa
  drawLine(20, 85, 108, 85, NARANJA);
  
  // Indicador de carga con animación
  
  // Puntos de carga animados
  static int dotCount = 0;
  static unsigned long lastDotTime = 0;
  if (millis() - lastDotTime > 500) {
    dotCount = (dotCount + 1) % 4;
    lastDotTime = millis();
  }
  
  String dots = "";
  for (int i = 0; i < dotCount; i++) {
    dots += ".";
  }
  st7735.st7735_write_str(95, 100, dots.c_str(), Font_7x10, ST7735_GRAY);
}

// WiFi icon drawing function
static void drawWiFiIcon(int x, int y, bool connected) {
  if (connected) {
    // Connected WiFi icon with signal strength bars (like WiFi networks)
    int rssi = WiFi.RSSI();
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;
    
    // Draw signal bars (larger version)
    for (int b = 0; b < 4; b++) {
      uint16_t barColor = (b < bars) ? NARANJA : ST7735_GRAY;
      int barHeight = (b + 1) * 2; // Larger bars
      int barY = y + 3 - barHeight/2;
      drawLine(x + b*3, barY, x + b*3, barY + barHeight, barColor);
    }
  } else {
    // Disconnected WiFi icon (larger red X)
    drawLine(x + 2, y + 2, x + 10, y + 10, ST7735_RED);
    drawLine(x + 10, y + 2, x + 2, y + 10, ST7735_RED);
  }
}

static void drawHeader(const String &title) {
  st7735.st7735_fill_screen(ST7735_BLACK);
  st7735.st7735_write_str(0, 0, title.c_str(), Font_7x10, ST7735_WHITE);
}

static void drawHeaderWithWiFi(const String &title) {
  st7735.st7735_fill_screen(ST7735_BLACK);
  
  // Draw WiFi icon in top right as status bar
  bool wifiStatus = WiFi.status() == WL_CONNECTED;
  drawWiFiIcon(140, 1, wifiStatus); // Positioned at top right, larger
  
  // Draw title
  st7735.st7735_write_str(0, 0, title.c_str(), Font_7x10, ST7735_WHITE);
}

static void drawFooter(const String &hint) {
  st7735.st7735_write_str(0, 70, hint.c_str(), Font_7x10, ST7735_WHITE);
}

static void drawFooter2(const String &l1, const String &l2) {
  st7735.st7735_write_str(0, 60, l1.c_str(), Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 70, l2.c_str(), Font_7x10, ST7735_WHITE);
}

static void goTo(MenuScreen s) { currentScreen = s; }

// Helper to draw slightly bolder text by overdrawing with 1px offset
static void write_str_bold(uint16_t x, uint16_t y, const char* text, FontDef font, uint16_t color, uint16_t bgcolor) {
  st7735.st7735_write_str(x, y, text, font, color, bgcolor);
  st7735.st7735_write_str(x + 1, y, text, font, color, bgcolor);
}

static void drawMenuIcon(int x, int y, int itemIndex, bool selected, uint16_t bgcolor) {
  uint16_t fg = selected ? MORADO : ST7735_WHITE;
  // Clear icon area first to avoid artifacts - increased size to 18x18
  fillRectPixels(x, y, 18, 18, bgcolor);
  switch (itemIndex) {
    case 0: { // Modo: toggle icon - larger version
      // Base line
      drawLine(x + 2, y + 9, x + 15, y + 9, fg);
      // Left circle
      drawCircle(x + 5, y + 9, 3, fg);
      // Right circle
      drawCircle(x + 13, y + 9, 3, fg);
    } break;
    case 1: { // WiFi: larger bars
      for (int b = 0; b < 4; b++) {
        uint16_t barColor = fg;
        int barHeight = (b + 1) * 2; // Double the height
        int barY = y + 15 - barHeight;
        drawLine(x + b*3 + 3, barY, x + b*3 + 3, barY + barHeight, barColor);
      }
    } break;
         case 2: { // Remoto: icono de antena/satélite
       // Antena principal
       drawLine(x + 9, y + 2, x + 9, y + 16, fg);
       // Base de la antena
       drawLine(x + 6, y + 16, x + 12, y + 16, fg);
       // Ondas de señal (arco superior)
       for (int i = 0; i < 3; i++) {
         int waveY = y + 4 + i * 3;
         int waveW = 6 + i * 2;
         drawLine(x + 9 - waveW/2, waveY, x + 9 + waveW/2, waveY, fg);
       }
    } break;
     case 3: { // Local: icono de casa/localización
       // Base de la casa
       drawLine(x + 3, y + 12, x + 15, y + 12, fg);
       // Lados de la casa
       drawLine(x + 3, y + 12, x + 3, y + 16, fg);
       drawLine(x + 15, y + 12, x + 15, y + 16, fg);
       // Techo triangular
       drawLine(x + 3, y + 12, x + 9, y + 6, fg);
       drawLine(x + 9, y + 6, x + 15, y + 12, fg);
       // Puerta
       drawLine(x + 7, y + 12, x + 7, y + 16, fg);
       drawLine(x + 11, y + 12, x + 11, y + 16, fg);
       drawLine(x + 7, y + 12, x + 11, y + 12, fg);
     } break;
     case 4: { // Rastreo: brújula/radar icon más visible
      // Icono de retículo/mira para Rastreo
      int centerX = x + 9;
      int centerY = y + 11;
      int radius = 6;

      // Dibujar círculo (contorno)
      drawCircleOutline(centerX, centerY, radius, fg);

      // Línea vertical
      drawLine(centerX, y + 2, centerX, y + 20, fg);
      // Línea horizontal
      drawLine(x, centerY, x + 18, centerY, fg);
     } break;
     case 5: { // Backtrack: flecha de retorno simple y clara
       // Flecha de retorno simple (forma de L invertida)
       drawLine(x + 4, y + 2, x + 4, y + 16, fg); // Línea vertical
       drawLine(x + 4, y + 16, x - 4, y + 16, fg); // Línea horizontal izquierda
       // Punta de la flecha apuntando hacia la izquierda
       drawLine(x - 4, y + 16, x - 1, y + 13, fg);
       drawLine(x - 4, y + 16, x - 1, y + 16, fg);
       drawLine(x - 1, y + 13, x - 1, y + 16, fg);
     } break;
     case 6: { // Info: círculo con i
      drawCircle(x + 9, y + 9, 7, fg);
      drawLine(x + 9, y + 6, x + 9, y + 11, fg);
      st7735.st7735_draw_pixel(x + 9, y + 5, fg);
    } break;
     case 7: { // Emparejar: icono de dos dispositivos conectados
       // Dispositivo izquierdo
       drawCircle(x + 4, y + 9, 3, fg);
       // Dispositivo derecho
       drawCircle(x + 14, y + 9, 3, fg);
       // Línea de conexión entre dispositivos
       drawLine(x + 7, y + 9, x + 11, y + 9, fg);
       // Puntos de conexión en cada dispositivo
       drawLine(x + 6, y + 9, x + 8, y + 9, fg);
       drawLine(x + 10, y + 9, x + 12, y + 9, fg);
    } break;
  }
}

static void renderMainMenu() {
  // Si es la primera vez que entramos a esta pantalla, dibujar el fondo estático
  if (lastRenderedScreen != MenuScreen::MenuPrincipal) {
    st7735.st7735_fill_rectangle(0, 20, 128, 70, ST7735_BLACK);

    drawHeaderWithWiFi("Inicio");
    // Dibujar la barra blanca de selección UNA SOLA VEZ
    fillRectPixels(0, 34, 160, 26, ST7735_WHITE);
    lastRenderedScreen = MenuScreen::MenuPrincipal;
    lastMainMenuIdx = -1; // Forzar un redibujado completo de los items del menú
  }

  // Si el índice del menú ha cambiado, redibujar solo los items
  if (lastMainMenuIdx != mainMenuIdx) {
    // --- Limpieza selectiva ---
    // 1. Limpiar el área de texto superior (encima de la barra blanca)
    st7735.st7735_fill_rectangle(0, 15, 160, 19, ST7735_BLACK);

    // 2. Limpiar el área de texto inferior (debajo de la barra blanca)
    st7735.st7735_fill_rectangle(0, 60, 160, 20, ST7735_BLACK);

    // 3. Limpiar el contenido anterior de la barra blanca (sin redibujar toda la barra).
    //    Esto es necesario para borrar el texto e icono antiguos antes de dibujar los nuevos.
    fillRectPixels(20, 34, 120, 26, ST7735_WHITE); // Limpia el centro de la barra blanca

    // --- Redibujado de items ---
    const char* menuItems[] = {"Modo", "WiFi", "Remoto", "Local", "Rastreo", "Backtrack", "Emparejar"};

    // Dibujar opción de arriba (si existe)
    if (mainMenuIdx > 0) {
      int aboveIdx = mainMenuIdx - 1;
      String aboveText = String(menuItems[aboveIdx]);
      // Calculate centered positions
      int iconX = 35; // Center of screen (128/2 - 18/2 = 55)
      int textX = 55; // Icon center + icon width/2 + spacing
      switch (aboveIdx) {
        case 0:
          textX = 75;
          iconX = 50;
          break;
        case 1:
          textX = 75;
          iconX = 55;
          break;
        case 2:
          textX = 65;
          iconX = 45;
          break;
        case 3:
          textX = 73;
          iconX = 53;
          break;
        case 4:
          textX = 63;
          iconX = 43;
          break;
        case 5:
          textX = 55;
          iconX = 45;
          break;
      }

      // Icon for above (on black bg) - centered
      drawMenuIcon(iconX, 15, aboveIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(textX, 20, aboveText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    // Dibujar opción seleccionada (sobre la barra blanca ya existente)
    String selectedText = String(menuItems[mainMenuIdx]);    
    // Calculate centered positions for selected item
    int selectedIconX = 35; // Center of screen
    int selectedTextX = 55; // Icon center + icon width/2 + spacing
    switch (mainMenuIdx) {
      case 0:
        selectedTextX = 65;
        selectedIconX = 45;
        break;
      case 1:
        selectedTextX = 65;
        selectedIconX = 45;
        break;
      case 2:
        selectedTextX = 55;
        selectedIconX = 35;
        break;
      case 3:
        selectedTextX = 60;
        selectedIconX = 40;
        break;
      case 4:
        selectedTextX = 52;
        selectedIconX = 32;
        break;
      case 5:
        selectedTextX = 40;
        selectedIconX = 25;
        break;
      case 6:
        selectedTextX = 40;
        selectedIconX = 25;
        break;
    }
     

     
    // Icon for selected (on white bg) - centered
    drawMenuIcon(selectedIconX, 34, mainMenuIdx, true, ST7735_WHITE);
    write_str_bold(selectedTextX, 38, selectedText.c_str(), Font_11x18, ST7735_BLACK, ST7735_WHITE);

    // Dibujar opción de abajo (si existe)
      if (mainMenuIdx < 6) { // Cambiado de 5 a 6 para mostrar la opción de abajo
      int belowIdx = mainMenuIdx + 1;
      String belowText =  String(menuItems[belowIdx]);
      // Calculate centered positions
      int belowIconX = 35; // Center of screen
      int belowTextX = 55; // Icon center + icon width/2 + spacing
      switch (belowIdx) {
        case 0:
          belowTextX = 65;
          belowIconX = 45;
          break;
        case 1:
          belowTextX = 70;
          belowIconX = 50;
          break;
        case 2: 
          belowTextX = 64;
          belowIconX = 44;
          break;
        case 3: 
          belowTextX = 70;
          belowIconX = 50;
          break;
        case 4: 
          belowTextX = 64;
          belowIconX = 42;
          break;
        case 5:
          belowTextX = 58;
          belowIconX = 48;
          break;
        case 6:
          belowTextX = 55;
          belowIconX = 35;
          break;
      }
       

       
      // Icon for below (on black bg) - centered
      drawMenuIcon(belowIconX, 61, belowIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(belowTextX, 65, belowText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    lastMainMenuIdx = mainMenuIdx;
  }
}

static void renderMode() {
  bool remoteMode = isRemoteModeSelected();
  
  // Solo redibujar si cambia el modo o la pantalla
  if (lastRenderedScreen != MenuScreen::SelectMode || lastRemoteMode != remoteMode) {
    drawHeaderWithWiFi("Seleccionar Modo");
    lastRenderedScreen = MenuScreen::SelectMode;
    lastRemoteMode = remoteMode;

    // Limpiar área de opciones
    st7735.st7735_fill_rectangle(0, 18, 160, 62, ST7735_BLACK);

    // Opciones verticales centradas
    int boxW = 180, boxH = 28;
    int x = 0;
    int yRemoto = 28;
    int yLocal = 60;

    // Remoto (arriba)
    if (remoteMode) {
      fillRectPixels(x, yRemoto, boxW, boxH, ST7735_WHITE);
      st7735.st7735_write_str(x + 50, yRemoto + 6, "Remoto", Font_11x18, ST7735_BLACK, ST7735_WHITE);
    } else {
      fillRectPixels(x, yRemoto, boxW, boxH, ST7735_BLACK);
      st7735.st7735_write_str(x + 55, yRemoto - 10, "Remoto", Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    // Local (abajo)
    if (!remoteMode) {
      fillRectPixels(x, yRemoto + 4, boxW, boxH, ST7735_WHITE);
      st7735.st7735_write_str(x + 54, yRemoto + 10, "Local", Font_11x18, ST7735_BLACK, ST7735_WHITE);
    } else {
      fillRectPixels(x, yLocal, boxW, boxH, ST7735_BLACK);
      st7735.st7735_write_str(x + 64, yLocal + 6, "Local", Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }
    lastRemoteMode = remoteMode; // También actualizar el cache

  }

  //drawFooter("OK: cambiar | OK3s menú");
}

static void renderMenuPrincipal() {
  String currentWifiStatus = String(WiFi.status() == WL_CONNECTED ? "Conectado" : "No conectado");
  
  // Only redraw if screen changed or menu index changed or wifi status changed
  if (lastRenderedScreen != MenuScreen::MenuPrincipal || lastMenuPrincipalIdx != MenuPrincipalIdx || lastWifiStatus != currentWifiStatus) {
    drawHeaderWithWiFi("WiFi");
    lastRenderedScreen = MenuScreen::MenuPrincipal;
    lastMenuPrincipalIdx = MenuPrincipalIdx;
    lastWifiStatus = currentWifiStatus;
  }
  
  // Update only the changing parts
  st7735.st7735_write_str(0, 20, (MenuPrincipalIdx == 0 ? "> " : "  ") + String("Escanear redes"), Font_7x10, MenuPrincipalIdx == 0 ? MORADO : ST7735_WHITE);
  st7735.st7735_write_str(0, 32, (MenuPrincipalIdx == 1 ? "> " : "  ") + String("Estado: ") + currentWifiStatus, Font_7x10, MenuPrincipalIdx == 1 ? MORADO : ST7735_WHITE);
  drawFooter2("Arr/Abj mover | OK entrar", "OK3s menu");
}

static void renderWiFiScanning() {
  if (lastRenderedScreen != MenuScreen::WiFiScan) {
    drawHeaderWithWiFi("Escaneando...");
    lastRenderedScreen = MenuScreen::WiFiScan;
  }
  drawFooter("OK3s menu");
}

static void renderWiFiSelect() {
  if (lastRenderedScreen != MenuScreen::WiFiSelectSSID || lastSelectedNetworkIdx != selectedNetworkIdx) {
    drawHeaderWithWiFi("Seleccionar WiFi");
    lastRenderedScreen = MenuScreen::WiFiSelectSSID;
    lastSelectedNetworkIdx = selectedNetworkIdx;
  }
  
  // Draw network list with improved design
  for (int i = 0; i < 4 && i < numNetworks; i++) {
    int idx = (selectedNetworkIdx + i) % numNetworks;
    String ssid = WiFi.SSID(idx);
    int rssi = WiFi.RSSI(idx);
    
    // Truncate SSID if too long
    if (ssid.length() > 12) {
      ssid = ssid.substring(0, 9) + "...";
    }
    
    String line = (i == 0 ? "> " : "  ") + ssid;
    uint16_t textColor = (i == 0) ? MORADO : ST7735_WHITE;
    
    st7735.st7735_write_str(0, 16 + (i * 12), line.c_str(), Font_7x10, textColor);
    
    // Draw signal strength indicator
    int signalX = 100;
    int signalY = 16 + (i * 12);
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;
    
    for (int b = 0; b < 4; b++) {
      uint16_t barColor = (b < bars) ? NARANJA : ST7735_GRAY;
      drawLine(signalX + b*3, signalY + 10 - b*2, signalX + b*3, signalY + 10, barColor);
    }
  }
  drawFooter("Arr/Abj: mover | OK: seleccionar");
}

static void renderPasswordInput() {
  if (lastRenderedScreen != MenuScreen::WiFiEnterPassword || lastEnteredPassword != enteredPassword || lastCharIndex != charIndex) {
    drawHeaderWithWiFi("Contraseña");
    st7735.st7735_write_str(0, 16, String("SSID: ") + selectedSSID, Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 28, String("Pass: ") + enteredPassword + String("*"), Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 40, String("Char: ") + String(allowedChars[charIndex]), Font_7x10, MORADO);
    lastRenderedScreen = MenuScreen::WiFiEnterPassword;
    lastEnteredPassword = enteredPassword;
    lastCharIndex = charIndex;
  }
  drawFooter2("Arr/Abj: char | OK: agregar", "OK largo: conectar");
}

static void renderConnecting() {
  if (lastRenderedScreen != MenuScreen::WiFiConnecting || lastWifiConnTries != wifiConnTries) {
    drawHeaderWithWiFi("Conectando...");
    st7735.st7735_write_str(0, 16, String("Intento: ") + String(wifiConnTries), Font_7x10, ST7735_WHITE);
    lastRenderedScreen = MenuScreen::WiFiConnecting;
    lastWifiConnTries = wifiConnTries;
  }
  drawFooter("Esperando...");
}

static void renderWiFiStatus() {
  String currentIP = WiFi.localIP().toString();
  bool currentConnected = WiFi.status() == WL_CONNECTED;
  
  if (lastRenderedScreen != MenuScreen::WiFiStatus || lastWifiConnected != currentConnected || lastWifiIP != currentIP) {
    drawHeaderWithWiFi("Estado WiFi");
    st7735.st7735_write_str(0, 16, String("Estado: ") + (currentConnected ? "Conectado" : "Desconectado"), Font_7x10, currentConnected ? MORADO : ST7735_RED);
    if (currentConnected) {
      st7735.st7735_write_str(0, 28, String("IP: ") + currentIP, Font_7x10, ST7735_WHITE);
      st7735.st7735_write_str(0, 40, String("RSSI: ") + String(WiFi.RSSI()) + " dBm", Font_7x10, ST7735_WHITE);
    }
    lastRenderedScreen = MenuScreen::WiFiStatus;
    lastWifiConnected = currentConnected;
    lastWifiIP = currentIP;
  }
  drawFooter("OK: volver");
}

static void renderLocalData() {
  // Solo redibujar si cambia la pantalla o los datos
  static SensorData prevData = {};
  static unsigned long prevReceived = 0;
  
  bool changed = (lastRenderedScreen != MenuScreen::LocalData ||
                  memcmp(&prevData, &lastLoRaData, sizeof(SensorData)) != 0 ||
                  prevReceived != lastLoRaReceived || datosRecibidos );
  
  
  if (changed) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    drawHeaderWithWiFi("Datos Remoto");
    lastRenderedScreen = MenuScreen::LocalData;
    prevData = lastLoRaData;
    prevReceived = lastLoRaReceived;

    char buf[32];
    snprintf(buf, sizeof(buf), "Ubicacion:", lastLoRaData.latitude);
    st7735.st7735_write_str(0, 16, buf, Font_7x10, ST7735_WHITE);
    snprintf(buf, sizeof(buf), "%.5f,%.5f", lastLoRaData.latitude,lastLoRaData.longitude);
    st7735.st7735_write_str(0, 28, buf, Font_7x10, ST7735_WHITE);
    snprintf(buf, sizeof(buf), "Estado: %d", lastLoRaData.sensor4);
    st7735.st7735_write_str(0, 40, buf, Font_7x10, ST7735_WHITE);
    snprintf(buf, sizeof(buf), "RSSI:%d SNR:%d", lastLoRaData.rssi, lastLoRaData.snr);
    st7735.st7735_write_str(0, 52, buf, Font_7x10, ST7735_WHITE);
    //snprintf(buf, sizeof(buf), "Recibido: %lus", lastLoRaReceived / 1000);
    //st7735.st7735_write_str(0, 64, buf, Font_7x10, ST7735_GRAY);

    if (datosRecibidos) { 
      datosRecibidos = false;
    }
  }
}

// Función para renderizar la pantalla de rastreo
static void renderTracking() {
  if (lastRenderedScreen != MenuScreen::Tracking) {
    drawHeaderWithWiFi("Rastreo GPS");
    lastRenderedScreen = MenuScreen::Tracking;
  }
  
  // Verificar estado del GPS local
  bool gpsLocalOk = isLocalGPSAvailable();
  
  // Verificar si tenemos datos del objetivo
  if (!hasTargetData || (millis() - lastTargetUpdate) > TARGET_TIMEOUT) {
    // Sin datos del objetivo
    st7735.st7735_write_str(0, 20, "Esperando datos...", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 32, "Objetivo no encontrado", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 44, "Asegurate que el", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 56, "dispositivo remoto", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 68, "este activo", Font_7x10, ST7735_WHITE);
  } else if (!gpsLocalOk) {
    // Sin GPS local
    st7735.st7735_write_str(0, 20, "GPS Local no disponible", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 32, "Esperando señal GPS...", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 44, "Objetivo detectado en:", Font_7x10, ST7735_WHITE);
    String targetPos = String("Lat: ") + String(targetData.latitude, 5);
    st7735.st7735_write_str(0, 56, targetPos.c_str(), Font_7x10, ST7735_WHITE);
    String targetPos2 = String("Lon: ") + String(targetData.longitude, 5);
    st7735.st7735_write_str(0, 68, targetPos2.c_str(), Font_7x10, ST7735_WHITE);
  } else {
    // Mostrar información de navegación completa
    String distanceStr = String("Dist: ") + String(currentNavigation.distance, 1) + "m";
    String bearingStr = String("Dir: ") + String(currentNavigation.bearing, 0) + "°";
    String directionStr = String("Hacia: ") + currentNavigation.direction;
    
    // Dibujar brújula a la derecha (centro horizontal de la mitad derecha)
    drawCompass(120, 40, 20, currentNavigation.bearing, MORADO);
    
    // Información de navegación a la izquierda
    st7735.st7735_write_str(0, 16, bearingStr.c_str(), Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 28, directionStr.c_str(), Font_7x10, NARANJA);
    
    // Distancia centrada abajo
    int distanceX = (160 - distanceStr.length() * 7) / 2; // Centrar texto
    st7735.st7735_write_str(distanceX, 68, distanceStr.c_str(), Font_7x10, ST7735_WHITE);
    
    // Indicador de GPS local
    st7735.st7735_write_str(100, 0, "GPS", Font_7x10, MORADO);
  }
}

// ===== PANTALLA DE EMPAREJAMIENTO =====

// Pantalla de emparejamiento
static void renderPairing() {
  if (lastRenderedScreen != MenuScreen::Pairing) {
    drawHeaderWithWiFi("Emparejar");
    lastRenderedScreen = MenuScreen::Pairing;
  }
  
  // Verificar estado de WiFi
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  
  if (!wifiConnected) {
    // Sin WiFi, mostrar mensaje de error
    st7735.st7735_write_str(0, 20, "WiFi no conectado", Font_7x10, ST7735_RED);
    st7735.st7735_write_str(0, 32, "Conectate a WiFi para", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 44, "recibir solicitudes", Font_7x10, ST7735_WHITE);
    
    drawFooter("BACK: Volver al menu");
    return;
  }
  
  // Verificar si hay solicitudes de emparejamiento pendientes
  if (pairingRequestPending && !lastPantallaEmparejamiento) {
    // Mostrar solicitud pendiente
    st7735.st7735_write_str(0, 20, "Solicitud", Font_7x10, NARANJA);
    st7735.st7735_write_str(0, 32, "Dispositivo:", Font_7x10, ST7735_WHITE);
    
    // Mostrar ID del dispositivo (truncado si es muy largo)
    String deviceId = pairingDeviceId;
    if (deviceId.length() > 20) {
      deviceId = deviceId.substring(0, 17) + "...";
    }
    st7735.st7735_write_str(0, 44, deviceId.c_str(), Font_7x10, MORADO);
    
    // Botón de aceptar
    fillRectPixels(20, 60, 120, 20, MORADO);
    st7735.st7735_write_str(45, 65, "ACEPTAR", Font_7x10, ST7735_WHITE, MORADO);
    lastPantallaEmparejamiento = true;

    
    //drawFooter("OK: Aceptar | BACK: Rechazar");
  } else if(!pairingRequestPending && lastPantallaEmparejamiento) {
    // Sin solicitudes pendientes
    st7735.st7735_write_str(0, 20, "No hay solicitudes", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 32, "de emparejamiento", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 44, "pendientes", Font_7x10, ST7735_GRAY);
    
    // Indicador de estado
    st7735.st7735_write_str(0, 60, "Estado: Esperando...", Font_7x10, ST7735_WHITE);
    lastPantallaEmparejamiento = true;
    //drawFooter("BACK: Volver al menu");
  }
}

// ===== PANTALLAS DE BACKTRACK =====

// Pantalla de gestión de waypoints
static void renderWaypointManager() {
  loadWaypointsFromStorage();
  if (lastRenderedScreen != MenuScreen::WaypointManager) {
    drawHeaderWithWiFi("Gestionar Puntos");
    lastRenderedScreen = MenuScreen::WaypointManager;
    // Inicializar preferencias si no están listas
    //initWaypointPrefs();
  }
  
  // Sistema de scroll para waypoints (mostrar solo 3 por pantalla)
  int maxScrollOffset = max(0, waypointCount - 3);
  
  // Mostrar solo 3 waypoints por pantalla
  int yPos = 16 ;
  for (int i = 0; i < 3 && (i + waypointScrollOffset) < waypointCount; i++) {
    int actualIndex = i + waypointScrollOffset;
    String waypointText = String(actualIndex + 1) + ". " + waypoints[actualIndex].name;
    if (waypointText.length() > 18) {
      waypointText = waypointText.substring(0, 15) + "...";
    }
    
    uint16_t textColor = (actualIndex == selectedWaypointIndex) ? MORADO : ST7735_WHITE;
    st7735.st7735_write_str(0, yPos, waypointText.c_str(), Font_7x10, textColor);
    
    // Mostrar coordenadas abreviadas
    String coords = String(waypoints[actualIndex].latitude, 4) + "," + String(waypoints[actualIndex].longitude, 4);
    st7735.st7735_write_str(0, yPos + 10, coords.c_str(), Font_7x10, ST7735_GRAY);
    
    yPos += 20;
  }
  
  // Indicadores de scroll
  if (waypointCount > 3) {
    if (waypointScrollOffset < maxScrollOffset) {
      // Triángulo arriba (relleno)
      for (int dy = 0; dy < 8; dy++) {
          int startX = 140 + dy;
          int endX = 148 - dy;
          for (int x = startX; x <= endX; x++) {
              st7735.st7735_draw_pixel(x, 56 + 7 + dy, ST7735_GRAY);
          }
      }
    }
    if (waypointScrollOffset > 0) {
        // Triángulo abajo (relleno)
        for (int dy = 0; dy < 8; dy++) {
            int startX = 140 + dy;
            int endX = 148 - dy;
            for (int x = startX; x <= endX; x++) {
                st7735.st7735_draw_pixel(x, 16 - dy, ST7735_GRAY);
            }
        }
    }
  }
  
  // Mostrar opciones
  if (waypointCount == 0) {
    st7735.st7735_write_str(0, 40, "No hay puntos guardados", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 52, "OK3s: Agregar punto", Font_7x10, NARANJA);
  } else {
    //st7735.st7735_write_str(0, 50, "OK: Entrar a punto", Font_7x10, ST7735_WHITE);
    //st7735.st7735_write_str(0, 60, "OK3s: Agregar punto", Font_7x10, NARANJA);
    //st7735.st7735_write_str(0, 70, "BACK: Eliminar punto", Font_7x10, ST7735_RED);
  }
  
 //drawFooter("BACK: eliminar | BACK3s: menu | OK3s: agregar");
}

// Pantalla de navegación hacia waypoint (backtrack)
static void renderBacktrack() {
  if (lastRenderedScreen != MenuScreen::Backtrack) {
    drawHeaderWithWiFi("Backtrack");
    lastRenderedScreen = MenuScreen::Backtrack;
  }
  
  if (!hasWaypointTarget || selectedWaypointIndex < 0) {
    // Sin waypoint seleccionado
    st7735.st7735_write_str(0, 20, "Selecciona un punto", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 32, "desde Gestionar Puntos", Font_7x10, ST7735_GRAY);
              st7735.st7735_write_str(0, 44, "para comenzar navegacion", Font_7x10, ST7735_WHITE);
      drawFooter("OK: waypoints | BACK: menu");
      return;
    }
    
    if (!localPositionSet) {
      // Sin GPS local
      st7735.st7735_write_str(0, 20, "GPS Local no disponible", Font_7x10, ST7735_RED);
      st7735.st7735_write_str(0, 32, "Esperando señal GPS...", Font_7x10, ST7735_GRAY);
      drawFooter("OK: waypoints | BACK: menu");
      return;
    }
  
  // Actualizar navegación
  updateWaypointNavigation();
  
  // Mostrar información del waypoint objetivo
  String targetName = waypoints[selectedWaypointIndex].name;
  st7735.st7735_write_str(0, 16, "Hacia: " + targetName, Font_7x10, NARANJA);
  
  // Mostrar información de navegación
  String distanceStr = String("Dist: ") + String(waypointNavigation.distance, 1) + "m";
  String bearingStr = String("Dir: ") + String(waypointNavigation.bearing, 0) + "°";
  String directionStr = String("Hacia: ") + waypointNavigation.direction;
  
  // Dibujar brújula a la derecha
  drawCompass(120, 40, 20, waypointNavigation.bearing, MORADO);
  
  // Información de navegación a la izquierda
  st7735.st7735_write_str(0, 28, bearingStr.c_str(), Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 40, directionStr.c_str(), Font_7x10, NARANJA);
  
  // Distancia centrada abajo
  int distanceX = (160 - distanceStr.length() * 7) / 2;
  st7735.st7735_write_str(distanceX, 68, distanceStr.c_str(), Font_7x10, ST7735_WHITE);
  
  // Indicador de GPS local
  st7735.st7735_write_str(100, 0, "GPS", Font_7x10, MORADO);
  
     // Footer con instrucciones
  // drawFooter("OK: waypoints | BACK: menu");
}

// 1. Agregar nuevo estado de pantalla para el minimapa:
static void renderBacktrackMap() {
    if (lastRenderedScreen != MenuScreen::BacktrackMap) {
        st7735.st7735_fill_screen(ST7735_BLACK);
        st7735.st7735_write_str(10, 0, "Mapa de ruta", Font_7x10, MORADO);
        lastRenderedScreen = MenuScreen::BacktrackMap;
    }
    if (waypointCount == 0) {
        st7735.st7735_write_str(10, 40, "No hay puntos", Font_7x10, ST7735_GRAY);
        return;
    }
    
    // Verificar si algo cambió para evitar redibujado innecesario
    bool needsRedraw = false;
    if (fabs(localLatitude - lastMapLat) > 0.00001f || 
        fabs(localLongitude - lastMapLon) > 0.00001f ||
        waypointCount != lastMapWaypointCount ||
        selectedWaypointIndex != lastMapSelectedIdx) {
        needsRedraw = true;
        lastMapLat = localLatitude;
        lastMapLon = localLongitude;
        lastMapWaypointCount = waypointCount;
        lastMapSelectedIdx = selectedWaypointIndex;
    }
    
    if (!needsRedraw) {
        return; // No redibujar si nada cambió
    }
    
    // Limpiar solo el área del mapa (no toda la pantalla)
    fillRectPixels(10, 10, 140, 60, ST7735_BLACK);
    
    // --- Filtrar puntos cercanos ---
    const float MAX_DIST_KM = 5.0f;
    int nearbyIdx[MAX_WAYPOINTS];
    int nearbyCount = 0;
    float refLat = localLatitude;
    float refLon = localLongitude;
    bool useNearby = localPositionSet;
    if (useNearby) {
        for (int i = 0; i < waypointCount; i++) {
            float dLat = radians(waypoints[i].latitude - refLat);
            float dLon = radians(waypoints[i].longitude - refLon);
            float a = sin(dLat/2)*sin(dLat/2) + cos(radians(refLat))*cos(radians(waypoints[i].latitude))*sin(dLon/2)*sin(dLon/2);
            float c = 2 * atan2(sqrt(a), sqrt(1-a));
            float dist = 6371.0f * c; // Radio Tierra en km
            if (dist <= MAX_DIST_KM) {
                nearbyIdx[nearbyCount++] = i;
            }
        }
    }
    if (useNearby && nearbyCount == 0) {
        st7735.st7735_write_str(10, 40, "No hay puntos cercanos", Font_7x10, ST7735_GRAY);
        return;
    }
    // Calcular bounding box de los puntos a mostrar
    float minLat, maxLat, minLon, maxLon;
    if (useNearby && nearbyCount > 0) {
        minLat = maxLat = waypoints[nearbyIdx[0]].latitude;
        minLon = maxLon = waypoints[nearbyIdx[0]].longitude;
        for (int i = 1; i < nearbyCount; i++) {
            float lat = waypoints[nearbyIdx[i]].latitude;
            float lon = waypoints[nearbyIdx[i]].longitude;
            if (lat < minLat) minLat = lat;
            if (lat > maxLat) maxLat = lat;
            if (lon < minLon) minLon = lon;
            if (lon > maxLon) maxLon = lon;
        }
        // Incluir posición actual
        if (localPositionSet) {
            if (localLatitude < minLat) minLat = localLatitude;
            if (localLatitude > maxLat) maxLat = localLatitude;
            if (localLongitude < minLon) minLon = localLongitude;
            if (localLongitude > maxLon) maxLon = localLongitude;
        }
    } else {
        minLat = maxLat = waypoints[0].latitude;
        minLon = maxLon = waypoints[0].longitude;
        for (int i = 1; i < waypointCount; i++) {
            if (waypoints[i].latitude < minLat) minLat = waypoints[i].latitude;
            if (waypoints[i].latitude > maxLat) maxLat = waypoints[i].latitude;
            if (waypoints[i].longitude < minLon) minLon = waypoints[i].longitude;
            if (waypoints[i].longitude > maxLon) maxLon = waypoints[i].longitude;
        }
        if (localPositionSet) {
            if (localLatitude < minLat) minLat = localLatitude;
            if (localLatitude > maxLat) maxLat = localLatitude;
            if (localLongitude < minLon) minLon = localLongitude;
            if (localLongitude > maxLon) maxLon = localLongitude;
        }
    }
    float latMargin = (maxLat - minLat) * 0.1f + 0.0001f;
    float lonMargin = (maxLon - minLon) * 0.1f + 0.0001f;
    minLat -= latMargin; maxLat += latMargin;
    minLon -= lonMargin; maxLon += lonMargin;
    int mapX0 = 10, mapY0 = 10, mapW = 140, mapH = 60;
    // Forzar rango mínimo para evitar colapso visual
    if (fabs(maxLat - minLat) < 0.00005f) { maxLat += 0.000025f; minLat -= 0.000025f; }
    if (fabs(maxLon - minLon) < 0.00005f) { maxLon += 0.000025f; minLon -= 0.000025f; }
    // Si solo hay un punto, dibujarlo centrado
    if ((useNearby && nearbyCount == 1) || (!useNearby && waypointCount == 1)) {
        int x = mapX0 + mapW / 2;
        int y = mapY0 + mapH / 2;
        drawCircle(x, y, 5, NARANJA);
        st7735.st7735_write_str(x+6, y-4, "Punto", Font_7x10, NARANJA);
        return;
    }
    // Dibujar líneas entre puntos cercanos
    if (useNearby && nearbyCount > 1) {
        for (int i = 1; i < nearbyCount; i++) {
            int x0 = mapf(waypoints[nearbyIdx[i-1]].longitude, minLon, maxLon, mapX0, mapX0+mapW);
            int y0 = mapf(waypoints[nearbyIdx[i-1]].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
            int x1 = mapf(waypoints[nearbyIdx[i]].longitude,   minLon, maxLon, mapX0, mapX0+mapW);
            int y1 = mapf(waypoints[nearbyIdx[i]].latitude,    maxLat, minLat, mapY0, mapY0+mapH);
            drawLine(x0, y0, x1, y1, ST7735_GRAY);
        }
    } else if (!useNearby && waypointCount > 1) {
        for (int i = 1; i < waypointCount; i++) {
            int x0 = mapf(waypoints[i-1].longitude, minLon, maxLon, mapX0, mapX0+mapW);
            int y0 = mapf(waypoints[i-1].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
            int x1 = mapf(waypoints[i].longitude,   minLon, maxLon, mapX0, mapX0+mapW);
            int y1 = mapf(waypoints[i].latitude,    maxLat, minLat, mapY0, mapY0+mapH);
            drawLine(x0, y0, x1, y1, ST7735_GRAY);
        }
    }
    // Dibujar los puntos
    if (useNearby && nearbyCount > 0) {
        for (int i = 0; i < nearbyCount; i++) {
            int idx = nearbyIdx[i];
            int x = mapf(waypoints[idx].longitude, minLon, maxLon, mapX0, mapX0+mapW);
            int y = mapf(waypoints[idx].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
            uint16_t color = (idx == selectedWaypointIndex) ? MORADO : NARANJA;
            drawCircle(x, y, 3, color);
        }
    } else {
        for (int i = 0; i < waypointCount; i++) {
            int x = mapf(waypoints[i].longitude, minLon, maxLon, mapX0, mapX0+mapW);
            int y = mapf(waypoints[i].latitude,  maxLat, minLat, mapY0, mapY0+mapH);
            uint16_t color = (i == selectedWaypointIndex) ? MORADO : NARANJA;
            drawCircle(x, y, 3, color);
        }
    }
    // Dibuja la posición actual
    if (localPositionSet) {
        int x = mapf(localLongitude, minLon, maxLon, mapX0, mapX0+mapW);
        int y = mapf(localLatitude,  maxLat, minLat, mapY0, mapY0+mapH);
        drawCircle(x, y, 5, ST7735_WHITE);
        st7735.st7735_write_str(x+6, y-4, "Tú", Font_7x10, ST7735_WHITE);
    }
    st7735.st7735_write_str(0, 110, "Up/Down: salir", Font_7x10, ST7735_GRAY);
}
/*
void initUI() {
  // Configure buttons
  
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);
  
  // Initialize display
  //st7735.st7735_init();

  // Initialize wifi preferences storage (NVS)
  {
    Preferences wifiPrefs;
    wifiPrefs.begin("wifi", false);
    // Leer datos si es necesario aquí
    wifiPrefs.end();
  }

  // Initialize waypoint preferences storage (NVS)
  {
    Preferences waypointPrefs;
    waypointPrefs.begin("waypoints", false);
    loadWaypointsFromStorage();
    waypointPrefs.end();
  }
*/
  // Start with welcome screen
  //welcomeStartTime = millis();
  //welcomePhase = 0;
  //currentScreen = MenuScreen::Welcome;
  
  // Draw initial welcome screen
  //renderWelcomeScreen();


// --- Pantalla de bienvenida estática ---
static void drawStaticWelcome() {
  st7735.st7735_fill_screen(ST7735_BLACK);
  // Logo principal (centrado arriba)
  st7735.st7735_write_str(40, 6, "SafeRun", Font_11x18, NARANJA); // ancho aprox 80px
  st7735.st7735_write_str(55, 26, "Tracker", Font_7x10, ST7735_WHITE); // ancho aprox 50px
  // Línea decorativa (centrada)
  drawLine(30, 38, 130, 38, NARANJA);
  // Mensaje de bienvenida (centrado)
  st7735.st7735_write_str(20, 48, "BIENVENIDOS", Font_11x18, NARANJA); // ancho aprox 104px
  // Subtítulo (centrado)
}

void renderWelcomeScreen() {
  unsigned long now = millis();
  unsigned long elapsed = now - welcomeStartTime;
  if (welcomePhase == 0) {
    drawStaticWelcome();
    welcomePhase = 1;
  }
  // Esperar un tiempo antes de pasar al menú principal
  if (elapsed > WELCOME_DURATION) {
    welcomeScreenShown = true;
  currentScreen = MenuScreen::MenuPrincipal;
    lastRenderedScreen = MenuScreen::Welcome;
  renderMainMenu();
  }
}



void renderWelcomeMessage() {
  drawWelcomeMessage();
}

bool isWelcomeScreenComplete() {
  return welcomeScreenShown;
}

void Display::updateUI(unsigned long now) {
  // Handle welcome screen
  if (currentScreen == MenuScreen::Welcome) {
    // Permitir saltar la pantalla de bienvenida con cualquier botón
    if (readBtnOk() || readBtnUp() || readBtnDown() || readBtnBack()) {
      welcomeScreenShown = true;
      currentScreen = MenuScreen::MenuPrincipal;
      renderMainMenu();
      return;
    }
    
    renderWelcomeScreen();
    return;
  }
  
  if (!canReadButtons(now)) return;
  
  // Variables estáticas para manejo de botones (declaradas fuera del switch para evitar errores de jump)
  static bool infoOkWasPressed = false;
  static unsigned long infoOkDownMs = 0;
  static const unsigned long infoLongPressMs = 3000;
  
  static bool wpOkWasPressed = false;
  static unsigned long wpOkDownMs = 0;
  static const unsigned long wpLongPressMs = 3000;
  
  static bool btOkWasPressed = false;
  static unsigned long btOkDownMs = 0;
  static const unsigned long btLongPressMs = 3000;

  // Handle button input
  switch (currentScreen) {
    case MenuScreen::MenuPrincipal: {
             // Main menu navigation
                 if (readBtnUp())   mainMenuIdx = (mainMenuIdx - 1 + 7) % 7;
         if (readBtnDown()) mainMenuIdx = (mainMenuIdx + 1) % 7;
      
      // Check for short press to enter submenu
      static bool okWasPressed = false;
      static unsigned long okDownMs = 0;
      const unsigned long shortPressMs = 800;
      const unsigned long longPressMs = 3000;
      
      if (readBtnOk()) {
        if (!okWasPressed) {
          okDownMs = now;
          okWasPressed = true;
        } else if (now - okDownMs >= longPressMs) {
          // Long press: do nothing (stay in main menu)
          okWasPressed = false;
        }
      } else {
        // Button released
        if (okWasPressed) {
          unsigned long pressDuration = now - okDownMs;
          if (pressDuration >= 50 && pressDuration < shortPressMs) {
            // Short press: enter selected submenu
                         switch (mainMenuIdx) {
               case 0: currentScreen = MenuScreen::SelectMode; break;
               case 1: currentScreen = MenuScreen::WiFiSubMenu; break; // WiFi submenu
               case 2: currentScreen = MenuScreen::LocalData; break;
               case 3: currentScreen = MenuScreen::Info; break;
               case 4: currentScreen = MenuScreen::Tracking; break;
                case 5: currentScreen = MenuScreen::WaypointManager; break; // Gestionar waypoints
                case 6: currentScreen = MenuScreen::Pairing; break; // Pantalla de emparejamiento
             }
          }
        }
        okWasPressed = false;
      }
    } break;
    
    case MenuScreen::SelectMode: {
      if (readBtnUp() || readBtnDown()) {
        setRemoteMode(!isRemoteModeSelected());
      }
      
      // Check for long press (3 seconds) to go back to main menu
      static bool okWasPressed = false;
      static unsigned long okDownMs = 0;
      const unsigned long longPressMs = 3000;
      
      if (readBtnOk()) {
        if (!okWasPressed) {
          okDownMs = now;
          okWasPressed = true;
        } else if (now - okDownMs >= longPressMs) {
          currentScreen = MenuScreen::MenuPrincipal; // Back to main menu
          okWasPressed = false;
        }
      } else {
        // Reset long press detection when button is released
        okWasPressed = false;
      }
       
       // BACK corto: volver al menú principal
       if (readBtnBack()) {
         currentScreen = MenuScreen::MenuPrincipal;
         lastRenderedScreen = MenuScreen::Info;
      }
    } break;
    
    case MenuScreen::WiFiSubMenu: {
      // WiFi submenu (when coming from main menu)
      if (readBtnUp())   MenuPrincipalIdx = (MenuPrincipalIdx - 1 + 2) % 2;
      if (readBtnDown()) MenuPrincipalIdx = (MenuPrincipalIdx + 1) % 2;
      
      // Check for short press to enter submenu
      static bool okWasPressed = false;
      static unsigned long okDownMs = 0;
      const unsigned long shortPressMs = 800;
      const unsigned long longPressMs = 3000;
      
      if (readBtnOk()) {
        if (!okWasPressed) {
          okDownMs = now;
          okWasPressed = true;
        } else if (now - okDownMs >= longPressMs) {
          // Long press: go back to main menu
          currentScreen = MenuScreen::MenuPrincipal; // Back to main menu
          okWasPressed = false;
        }
      } else {
        // Button released
        if (okWasPressed) {
          unsigned long pressDuration = now - okDownMs;
          if (pressDuration >= 50 && pressDuration < shortPressMs) {
            // Short press: enter selected submenu
            if (MenuPrincipalIdx == 0) currentScreen = MenuScreen::WiFiScan;
            else currentScreen = MenuScreen::WiFiStatus;
          }
        }
        okWasPressed = false;
      }
       
       // BACK corto: volver al menú principal
       if (readBtnBack()) {
         currentScreen = MenuScreen::MenuPrincipal;
         lastRenderedScreen = MenuScreen::Info;
      }
    } break;
    
    case MenuScreen::WiFiSelectSSID:
      if (numNetworks <= 0) { if (readBtnBack()) currentScreen = MenuScreen::WiFiSubMenu; break; }
      if (readBtnUp())   selectedNetworkIdx = (selectedNetworkIdx - 1 + numNetworks) % numNetworks;
      if (readBtnDown()) selectedNetworkIdx = (selectedNetworkIdx + 1) % numNetworks;
      if (readBtnOk()) {
        String savedSsid, savedPass;
        {
          Preferences wifiPrefs;
          wifiPrefs.begin("wifi", false);
          savedSsid = wifiPrefs.getString("ssid", "");
          savedPass = wifiPrefs.getString("pass", "");
          wifiPrefs.end();
        }
        selectedSSID = WiFi.SSID(selectedNetworkIdx);
        if (savedSsid == selectedSSID) enteredPassword = savedPass; else enteredPassword = "";
        charIndex = 0; currentScreen = MenuScreen::WiFiEnterPassword;
       }
       
       // BACK corto: volver al menú anterior
       if (readBtnBack()) {
         currentScreen = MenuScreen::WiFiSubMenu;
         lastRenderedScreen = MenuScreen::Info;
      }
      break;
    case MenuScreen::WiFiEnterPassword: {
      if (readBtnUp())   charIndex = (charIndex + 1) % allowedCharsCount;
      if (readBtnDown()) charIndex = (charIndex - 1 + allowedCharsCount) % allowedCharsCount;
       
       // Manejo del botón BACK (corto y largo)
       static bool backPwdWasPressed = false;
       static unsigned long backPwdDownMs = 0;
       static const unsigned long backPwdLongPressMs = 3000;
       
      if (readBtnBack()) {
         if (!backPwdWasPressed) {
           backPwdDownMs = now;
           backPwdWasPressed = true;
         } else if (now - backPwdDownMs >= backPwdLongPressMs) {
           // BACK largo (3s): borrar último carácter
           if (enteredPassword.length() > 0) {
             enteredPassword.remove(enteredPassword.length() - 1);
           }
           backPwdWasPressed = false;
         }
       } else {
         // Botón liberado
         if (backPwdWasPressed) {
           unsigned long pressDuration = now - backPwdDownMs;
           if (pressDuration >= 50 && pressDuration < backPwdLongPressMs) {
             // BACK corto: volver al menú anterior
             currentScreen = MenuScreen::WiFiSelectSSID;
           }
         }
         backPwdWasPressed = false;
       }
       
      static bool okPwdWasPressed = false; static unsigned long okPwdDownMs = 0; const unsigned long longPressMs = 1200;
      bool okP = readBtnOk();
      if (okP) { if (!okPwdWasPressed) okPwdDownMs = now; okPwdWasPressed = true; }
      else {
        if (okPwdWasPressed) {
          unsigned long dur = now - okPwdDownMs;
          if (dur >= longPressMs) {
            currentScreen = MenuScreen::WiFiConnecting; renderConnecting();
             
             // Limpiar conexión previa antes de intentar nueva conexión
             WiFi.disconnect(true);
             delay(1000);
             
             // Configurar WiFi con mejor manejo de errores
             WiFi.mode(WIFI_STA);
             
             // Intentar conexión
             WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());
             wifiConnectingActive = true; 
             wifiConnLastCheckMs = now; 
             wifiConnTries = 0;
          } else if (dur >= 50) {
            enteredPassword += allowedChars[charIndex];
          }
        }
        okPwdWasPressed = false; okPwdDownMs = 0;
      }
    } break;
    case MenuScreen::WiFiConnecting:
      if (wifiConnTries > 0 && (WiFi.status() == WL_CONNECTED || wifiConnTries > 30)) {
         if (readBtnOk()) { 
           wifiConnectingActive = false; 
           currentScreen = MenuScreen::WiFiSubMenu; 
           
           // Limpiar recursos si la conexión falló
           if (WiFi.status() != WL_CONNECTED) {
             WiFi.disconnect(true);
             delay(500);
           }
         }
       }
       
       // BACK corto: volver al menú anterior
       if (readBtnBack()) {
         wifiConnectingActive = false;
         currentScreen = MenuScreen::WiFiSubMenu;
         lastRenderedScreen = MenuScreen::Info;
      }
      break;
    case MenuScreen::WiFiStatus:
      if (readBtnOk()) { currentScreen = MenuScreen::WiFiSubMenu; lastRenderedScreen = MenuScreen::Info; }
      
      // BACK corto: volver al menú anterior
      if (readBtnBack()) {
        currentScreen = MenuScreen::WiFiSubMenu;
        lastRenderedScreen = MenuScreen::Info;
      }
      break;
         case MenuScreen::WiFiScan:
       // BACK corto: volver al menú anterior
       if (readBtnBack()) {
         currentScreen = MenuScreen::WiFiSubMenu;
         lastRenderedScreen = MenuScreen::Info;
       }
       break;
       
     case MenuScreen::Info:
     case MenuScreen::LocalData:
     case MenuScreen::Tracking:
      // Check for long press (3 seconds) to go to main menu from Info/LocalData screen
      if (readBtnOk()) {
        if (!infoOkWasPressed) {
          infoOkDownMs = now;
          infoOkWasPressed = true;
        } else if (now - infoOkDownMs >= infoLongPressMs) {

          infoOkWasPressed = false;
        }
      } else {
        infoOkWasPressed = false;
      }
      if (readBtnBack()) {
        currentScreen = MenuScreen::MenuPrincipal;
        lastRenderedScreen = MenuScreen::Info;
     }
      break;
      
    case MenuScreen::WaypointManager: {
      // Navegación en la lista de waypoints con scroll
      int maxScrollOffset = max(0, waypointCount - 3);
      
      if (readBtnUp()) {
        if (waypointCount > 0) {
          selectedWaypointIndex = (selectedWaypointIndex - 1 + waypointCount) % waypointCount;
          
          // Ajustar scroll si es necesario
          if (selectedWaypointIndex < waypointScrollOffset) {
            waypointScrollOffset = max(0, selectedWaypointIndex);
          }
        }
      }
      if (readBtnDown()) {
        if (waypointCount > 0) {
          int prevSelected = selectedWaypointIndex;
          selectedWaypointIndex = (selectedWaypointIndex + 1) % waypointCount;
          
          // Ajustar scroll si es necesario
          if (selectedWaypointIndex >= waypointScrollOffset + 3) {
            waypointScrollOffset = min(maxScrollOffset, selectedWaypointIndex - 2);
          }
          // Si dimos la vuelta al inicio, mostrar los primeros 3
          if (prevSelected == waypointCount - 1 && selectedWaypointIndex == 0) {
            waypointScrollOffset = 0;
          }
        }
      }
      

      
      // Manejo del botón OK (corto y largo)
      if (readBtnOk()) {
        if (!wpOkWasPressed) {
          wpOkDownMs = now;
          wpOkWasPressed = true;
        } else if (now - wpOkDownMs >= wpLongPressMs) {
                     // OK largo (3s): agregar nuevo waypoint
           if (waypointCount < MAX_WAYPOINTS && localPositionSet) {
             String waypointName = "Punto " + String(waypointCount + 1);
             saveCurrentPositionAsWaypoint(waypointName);
             
             // Ajustar scroll si es necesario para mostrar el nuevo punto
             if (waypointCount > 3) {
               waypointScrollOffset = max(0, waypointCount - 3);
             }
             
             lastRenderedScreen = MenuScreen::Info; // Force redraw
           }
          wpOkWasPressed = false;
        }
      } else {
        // Botón liberado
        if (wpOkWasPressed) {
          unsigned long pressDuration = now - wpOkDownMs;
          if (pressDuration >= 50 && pressDuration < wpLongPressMs) {
            // OK corto: entrar a los puntos (ir a Backtrack)
            if (waypointCount > 0 && selectedWaypointIndex >= 0) {
              selectWaypoint(selectedWaypointIndex);
              currentScreen = MenuScreen::Backtrack;
              lastRenderedScreen = MenuScreen::Info; // Force redraw
            }
          }
        }
        wpOkWasPressed = false;
      }
      
             // Manejo del botón BACK (corto y largo)
       static bool wpBackWasPressed = false;
       static unsigned long wpBackDownMs = 0;
       static const unsigned long wpBackLongPressMs = 3000;
       
       if (readBtnBack()) {
         if (!wpBackWasPressed) {
           wpBackDownMs = now;
           wpBackWasPressed = true;
         } else if (now - wpBackDownMs >= wpBackLongPressMs) {
          // BACK corto: eliminar waypoint seleccionado
          if (waypointCount > 0 && selectedWaypointIndex >= 0) {
            deleteWaypoint(selectedWaypointIndex);
            
            // Ajustar scroll si es necesario después de eliminar
            int maxScrollOffset = max(0, waypointCount - 3);
            if (waypointScrollOffset > maxScrollOffset) {
              waypointScrollOffset = maxScrollOffset;
            }
            
            lastRenderedScreen = MenuScreen::Info; // Force redraw
          }
         }
       } else {
         // Botón liberado
         if (wpBackWasPressed) {
           unsigned long pressDuration = now - wpBackDownMs;
           if (pressDuration >= 50 && pressDuration < wpBackLongPressMs) {
            // BACK largo (3s): volver al menú principal
            currentScreen = MenuScreen::MenuPrincipal;
            lastRenderedScreen = MenuScreen::Info;
            wpBackWasPressed = false;
           }
         }
         wpBackWasPressed = false;
       }
       break;
    }
    
      case MenuScreen::Backtrack: {
       // OK corto: volver a gestión de waypoints
       if (readBtnUp()) {
          currentScreen = MenuScreen::BacktrackMap;
          lastRenderedScreen = MenuScreen::Backtrack;
       }
       if (readBtnDown()) {
          currentScreen = MenuScreen::BacktrackMap;
          lastRenderedScreen = MenuScreen::Backtrack;
       }
       if (readBtnOk()) {
         currentScreen = MenuScreen::WaypointManager;
         lastRenderedScreen = MenuScreen::Info;
       }
       
       // OK largo (3s): volver al menú principal
       if (readBtnOk()) {
         if (!btOkWasPressed) {
           btOkDownMs = now;
           btOkWasPressed = true;
         } else if (now - btOkDownMs >= btLongPressMs) {
           currentScreen = MenuScreen::MenuPrincipal;
           lastRenderedScreen = MenuScreen::Info;
           btOkWasPressed = false;
         }
       } else {
         btOkWasPressed = false;
       }
       
       // Manejo del botón BACK (corto y largo) - LÓGICA CORREGIDA
       static bool btBackWasPressed = false;
       static unsigned long btBackDownMs = 0;
       static const unsigned long btBackLongPressMs = 3000;
       
       if (readBtnBack()) {
         if (!btBackWasPressed) {
           btBackDownMs = now;
           btBackWasPressed = true;
         }
       } else {
         // Botón liberado
         if (btBackWasPressed) {
           unsigned long pressDuration = now - btBackDownMs;
           if (pressDuration >= 50 && pressDuration < btBackLongPressMs) {
             // BACK corto: volver al menu anterior
             currentScreen = MenuScreen::WaypointManager;
             lastRenderedScreen = MenuScreen::Info;
           } else if (pressDuration >= btBackLongPressMs) {
            // BACK largo (3s): borrar waypoint seleccionado
            if (waypointCount > 0 && selectedWaypointIndex >= 0) {
              deleteWaypoint(selectedWaypointIndex);
              lastRenderedScreen = MenuScreen::Info; // Force redraw
            }
             // BACK largo (3s): volver al menú anterior
             currentScreen = MenuScreen::WaypointManager;
             lastRenderedScreen = MenuScreen::Info;
           }
           btBackWasPressed = false;
         }
               }
        break;
      }
      
      case MenuScreen::Pairing: {
        // Verificar solicitudes de emparejamiento cada cierto tiempo
        if (now - lastPairingCheck >= PAIRING_CHECK_INTERVAL) {
          checkPairingRequests();
          lastPairingCheck = now;
        }
        
        if (pairingRequestPending) {
          // Si hay solicitud pendiente, manejar botones
          if (readBtnOk()) {
            // Aceptar emparejamiento
            processLinkRequest();
            pairingRequestPending = false;
            pairingDeviceId = "";
            lastRenderedScreen = MenuScreen::Info; // Force redraw
          }
          
          if (readBtnBack()) {
            // Rechazar emparejamiento
            rejectPairing();
            pairingRequestPending = false;
            pairingDeviceId = "";
            lastRenderedScreen = MenuScreen::Info; // Force redraw
          }
        } else {
          // Sin solicitudes, solo permitir volver
          if (readBtnBack()) {
            currentScreen = MenuScreen::MenuPrincipal;
            lastRenderedScreen = MenuScreen::Info;
          }
        }
        break;
      }
      // 3. Manejo de botones para entrar/salir del minimapa desde Backtrack:
      case MenuScreen::BacktrackMap:
        renderBacktrackMap();
        if (readBtnDown()) {
            currentScreen = MenuScreen::Backtrack;
            lastRenderedScreen = MenuScreen::BacktrackMap; // Forzar redibujado
        }
        if (readBtnUp()) {
            currentScreen = MenuScreen::Backtrack;
            lastRenderedScreen = MenuScreen::BacktrackMap; // Forzar redibujado
        }
        if (readBtnOk()) {
            currentScreen = MenuScreen::WaypointManager;
            lastRenderedScreen = MenuScreen::BacktrackMap; // Forzar redibujado
        }
        break;
  }

render:
  // Render
  switch (currentScreen) {
    case MenuScreen::MenuPrincipal: 
      // Always render main menu when in MenuPrincipal state
      renderMainMenu();
      break;
    case MenuScreen::SelectMode: renderMode(); break;
    case MenuScreen::WiFiSubMenu: renderMenuPrincipal(); break;
    case MenuScreen::WiFiScan:
      if (!wifiScanStarted) { 
        WiFi.mode(WIFI_STA); 
        WiFi.disconnect(true); 
        delay(1000); // Más tiempo para limpiar conexión
        WiFi.scanDelete(); 
        WiFi.scanNetworks(true); 
        selectedNetworkIdx = 0; 
        wifiScanStarted = true; 
      }
      else {
        int res = WiFi.scanComplete();
        if (res >= 0) { numNetworks = res; wifiScanStarted = false; currentScreen = (numNetworks <= 0) ? MenuScreen::WiFiSubMenu : MenuScreen::WiFiSelectSSID; }
      }
      renderWiFiScanning();
      break;
    case MenuScreen::WiFiSelectSSID: renderWiFiSelect(); break;
    case MenuScreen::WiFiEnterPassword: renderPasswordInput(); break;
    case MenuScreen::WiFiConnecting:
      if (wifiConnectingActive && (now - wifiConnLastCheckMs >= 300)) {
        wifiConnLastCheckMs = now; wifiConnTries++;
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true; 
          if (lastRenderedScreen != MenuScreen::WiFiConnecting || lastWifiConnected != true) {
            drawHeaderWithWiFi("Conectado"); 
            st7735.st7735_write_str(0, 16, WiFi.localIP().toString().c_str(), Font_7x10, MORADO); 
            drawFooter("OK: volver");
            lastRenderedScreen = MenuScreen::WiFiConnecting;
            lastWifiConnected = true;
          }
          {
            Preferences wifiPrefs;
            wifiPrefs.begin("wifi", false);
          wifiPrefs.putString("ssid", selectedSSID);
          wifiPrefs.putString("pass", enteredPassword);
            wifiPrefs.end();
          }
        }
        else if (wifiConnTries > 30) { 
          wifiConnected = false; 
          if (lastRenderedScreen != MenuScreen::WiFiConnecting || lastWifiConnected != false) {
            drawHeaderWithWiFi("Fallo conexion"); 
            drawFooter("OK: volver");
            lastRenderedScreen = MenuScreen::WiFiConnecting;
            lastWifiConnected = false;
          }
        }
      }
      break;
         case MenuScreen::WiFiStatus: renderWiFiStatus(); break;
     case MenuScreen::LocalData: renderLocalData(); break;
      case MenuScreen::Info: renderInfoScreen(); break;
     case MenuScreen::Tracking: renderTracking(); break;
      case MenuScreen::WaypointManager: renderWaypointManager(); break;
      case MenuScreen::Backtrack: renderBacktrack(); break;
      case MenuScreen::Pairing: renderPairing(); break;
      case MenuScreen::BacktrackMap: renderBacktrackMap(); break;
  }
}

void renderInfoLoRa(const SensorData &data) {
  if (currentScreen != MenuScreen::Info) return;
  
  // Only redraw if screen changed or data changed
  bool dataChanged = (lastLatitude != data.latitude || 
                     lastLongitude != data.longitude || 
                     lastRssi != data.rssi || 
                     lastSnr != data.snr || 
                     lastSensor4 != data.sensor4);
  
  if (lastRenderedScreen != MenuScreen::Info || dataChanged) {
    drawHeaderWithWiFi("LoRa datos");
    lastRenderedScreen = MenuScreen::Info;
    lastLatitude = data.latitude;
    lastLongitude = data.longitude;
    lastRssi = data.rssi;
    lastSnr = data.snr;
    lastSensor4 = data.sensor4;
  }
  
  // Always update the values (they might have changed)
  String nom = String("Ubicación:");
  String ubi = String(data.latitude, 5) + String(",") + String(data.longitude, 5);
  String rs  = String("RSSI:") + String(data.rssi) + String(" SNR:") + String(data.snr);
  String em  = String("Emergencia: ") + (data.sensor4 ? "SI" : "NO");
  st7735.st7735_write_str(0, 16, nom.c_str(), Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 28, ubi.c_str(), Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 40, rs.c_str(),  Font_7x10, ST7735_WHITE);
  st7735.st7735_write_str(0, 52, em.c_str(),  Font_7x10, data.sensor4 ? ST7735_RED : ST7735_WHITE);
  //drawFooter("OK 3s: menú");
}

// Función para actualizar datos de rastreo desde datos LoRa recibidos
void updateTrackingData(const SensorData &data) {
  // Actualizar datos del objetivo
  targetData.latitude = data.latitude;
  targetData.longitude = data.longitude;
  targetData.altitude = 0.0f; // Por defecto, se puede obtener del GPS si está disponible
  targetData.speed = 0.0f;    // Por defecto, se puede calcular si hay datos de velocidad
  targetData.course = 0.0f;   // Por defecto, se puede calcular si hay datos de dirección
  targetData.timestamp = millis();
  targetData.deviceId = 1; // ID del dispositivo remoto
  targetData.isValid = (data.latitude != 0.0f && data.longitude != 0.0f);
  Serial.println(targetData.isValid);
  Serial.println(targetData.latitude);
  Serial.println(targetData.longitude);


  //Serial.println("Tracking: Target data: %.6f, %.6f\n", targetData.latitude, targetData.c);
  if (targetData.isValid) {
    hasTargetData = true;
    lastTargetUpdate = millis();
    
    // Usar coordenadas GPS reales del dispositivo local (Heltec)
    // Estas coordenadas se actualizan desde el GPS del dispositivo local
    if (localPositionSet) {
      // Calcular navegación usando GPS real
      currentNavigation.distance = calculateDistance(localLatitude, localLongitude, targetData.latitude, targetData.longitude);
      currentNavigation.bearing = calculateBearing(localLatitude, localLongitude, targetData.latitude, targetData.longitude);
      currentNavigation.heading = currentNavigation.bearing;
      currentNavigation.hasTarget = true;
      currentNavigation.direction = getCardinalDirection(currentNavigation.bearing);
      
      Serial.printf("Tracking: Local GPS at %.6f, %.6f, Target at %.6f, %.6f\n", 
                    localLatitude, localLongitude, targetData.latitude, targetData.longitude);
      Serial.printf("Distance: %.1fm, Bearing: %.1f°, Direction: %s\n", 
                    currentNavigation.distance, currentNavigation.bearing, 
                    currentNavigation.direction.c_str());
    } else {
      // GPS local aún no disponible
      currentNavigation.hasTarget = false;
      Serial.println("Tracking: Waiting for local GPS fix...");
    }
  }
}

// Función para obtener datos de rastreo actuales
bool getTrackingData(TrackingData &data) {
  if (hasTargetData && (millis() - lastTargetUpdate) <= TARGET_TIMEOUT) {
    data = targetData;
    return true;
  }
  return false;
}

// Función para obtener datos de navegación actuales
bool getNavigationData(NavigationData &nav) {
  if (hasTargetData && (millis() - lastTargetUpdate) <= TARGET_TIMEOUT) {
    nav = currentNavigation;
    return true;
  }
  return false;
}

// Función para actualizar la posición GPS local desde el GPS del Heltec
void updateLocalGPSPosition(float lat, float lon) {
  if (lat != 0.0f && lon != 0.0f) {
    localLatitude = lat;
    localLongitude = lon;
    localPositionSet = true;
    lastLocalGPSUpdate = millis();
    
    // Si tenemos datos del objetivo, recalcular navegación
    if (hasTargetData && targetData.isValid) {
      currentNavigation.distance = calculateDistance(localLatitude, localLongitude, targetData.latitude, targetData.longitude);
      currentNavigation.bearing = calculateBearing(localLatitude, localLongitude, targetData.latitude, targetData.longitude);
      currentNavigation.heading = currentNavigation.bearing;
      currentNavigation.hasTarget = true;
      currentNavigation.direction = getCardinalDirection(currentNavigation.bearing);
      
      Serial.printf("Local GPS Updated: %.6f, %.6f\n", localLatitude, localLongitude);
      Serial.printf("Recalculated: Distance: %.1fm, Bearing: %.1f°, Direction: %s\n", 
                    currentNavigation.distance, currentNavigation.bearing, 
                    currentNavigation.direction.c_str());
    }
      
      // Si tenemos un waypoint seleccionado, recalcular navegación hacia él
      if (hasWaypointTarget && selectedWaypointIndex >= 0) {
        updateWaypointNavigation();
        Serial.printf("Waypoint Navigation Updated: Distance: %.1fm, Bearing: %.1f°, Direction: %s\n", 
                      waypointNavigation.distance, waypointNavigation.bearing, 
                      waypointNavigation.direction.c_str());
    }
  }
}

// Función para verificar si el GPS local está disponible
bool isLocalGPSAvailable() {
  return localPositionSet && (millis() - lastLocalGPSUpdate) < 60000; // 1 minuto timeout
}

// ===== SISTEMA DE BACKTRACK =====

// Inicializar preferencias para waypoints
static void initWaypointPrefs() {
  loadWaypointsFromStorage();
}
/*
// Cargar waypoints desde almacenamiento
static void loadWaypointsFromStorage() {
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointCount = waypointPrefs.getInt("count", 0);
  if (waypointCount > MAX_WAYPOINTS) waypointCount = MAX_WAYPOINTS;
  
  for (int i = 0; i < waypointCount; i++) {
    String prefix = "wp" + String(i) + "_";
    waypoints[i].latitude = waypointPrefs.getFloat((prefix + "lat").c_str(), 0.0f);
    waypoints[i].longitude = waypointPrefs.getFloat((prefix + "lon").c_str(), 0.0f);
    waypoints[i].name = waypointPrefs.getString((prefix + "name").c_str(), "Punto " + String(i + 1));
    waypoints[i].timestamp = waypointPrefs.getULong((prefix + "time").c_str(), 0);
    waypoints[i].isValid = true;
  }
  waypointPrefs.end();
}
*/
// Guardar waypoints en almacenamiento
static void saveWaypointsToStorage() {
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointPrefs.putInt("count", waypointCount);
  for (int i = 0; i < waypointCount; i++) {
    String prefix = "wp" + String(i) + "_";
    waypointPrefs.putFloat((prefix + "lat").c_str(), waypoints[i].latitude);
    waypointPrefs.putFloat((prefix + "lon").c_str(), waypoints[i].longitude);
    waypointPrefs.putString((prefix + "name").c_str(), waypoints[i].name);
    waypointPrefs.putULong((prefix + "time").c_str(), waypoints[i].timestamp);
  }
  waypointPrefs.end();
}

// Guardar posición actual como waypoint
void saveCurrentPositionAsWaypoint(const String& name) {
  if (!localPositionSet || waypointCount >= MAX_WAYPOINTS) return;
  
  initWaypointPrefs();
  
  waypoints[waypointCount].latitude = localLatitude;
  waypoints[waypointCount].longitude = localLongitude;
  waypoints[waypointCount].name = name;
  waypoints[waypointCount].timestamp = millis();
  waypoints[waypointCount].isValid = true;
  
  waypointCount++;
  saveWaypointsToStorage();
}

// Eliminar waypoint
void deleteWaypoint(int index) {
  if (index < 0 || index >= waypointCount) return;
  
  // Mover waypoints posteriores hacia adelante
  for (int i = index; i < waypointCount - 1; i++) {
    waypoints[i] = waypoints[i + 1];
  }
  
  waypointCount--;
  if (selectedWaypointIndex >= waypointCount) {
    selectedWaypointIndex = -1;
  }
  
  saveWaypointsToStorage();
}

// Seleccionar waypoint para navegación
void selectWaypoint(int index) {
  if (index >= 0 && index < waypointCount) {
    selectedWaypointIndex = index;
    hasWaypointTarget = true;
    
    // Calcular navegación hacia el waypoint
    if (localPositionSet) {
      waypointNavigation.distance = calculateDistance(localLatitude, localLongitude, 
                                                   waypoints[index].latitude, waypoints[index].longitude);
      waypointNavigation.bearing = calculateBearing(localLatitude, localLongitude, 
                                                  waypoints[index].latitude, waypoints[index].longitude);
      waypointNavigation.heading = waypointNavigation.bearing;
      waypointNavigation.direction = getCardinalDirection(waypointNavigation.bearing);
      waypointNavigation.hasTarget = true;
    }
  }
}

// Obtener waypoint seleccionado
bool getSelectedWaypoint(float& lat, float& lon, String& name) {
  if (selectedWaypointIndex >= 0 && selectedWaypointIndex < waypointCount) {
    lat = waypoints[selectedWaypointIndex].latitude;
    lon = waypoints[selectedWaypointIndex].longitude;
    name = waypoints[selectedWaypointIndex].name;
    return true;
  }
  return false;
}

// Obtener cantidad de waypoints
int getWaypointCount() {
  return waypointCount;
}

// Obtener nombre del waypoint
String getWaypointName(int index) {
  if (index >= 0 && index < waypointCount) {
    return waypoints[index].name;
  }
  return "";
}

// Limpiar todos los waypoints
void clearAllWaypoints() {
  waypointCount = 0;
  selectedWaypointIndex = -1;
  hasWaypointTarget = false;
  Preferences waypointPrefs;
  waypointPrefs.begin("waypoints", false);
  waypointPrefs.clear();
  waypointPrefs.end();
}

// Actualizar navegación hacia waypoint cuando cambia la posición local
static void updateWaypointNavigation() {
  if (hasWaypointTarget && selectedWaypointIndex >= 0 && localPositionSet) {
    waypointNavigation.distance = calculateDistance(localLatitude, localLongitude, 
                                                 waypoints[selectedWaypointIndex].latitude, 
                                                 waypoints[selectedWaypointIndex].longitude);
    waypointNavigation.bearing = calculateBearing(localLatitude, localLongitude, 
                                                waypoints[selectedWaypointIndex].latitude, 
                                                waypoints[selectedWaypointIndex].longitude);
    waypointNavigation.heading = waypointNavigation.bearing;
    waypointNavigation.direction = getCardinalDirection(waypointNavigation.bearing);
  }
}

// Cache para pantalla Info
static bool lastInfoRemoteMode = false;
static bool lastInfoWifi = false;
static float lastInfoLat = 0.0f;
static float lastInfoLon = 0.0f;
static bool lastInfoValid = false;

static void renderInfoScreen() {
  // Estado actual
  bool remoteMode = isRemoteModeSelected();
  bool wifi = WiFi.status() == WL_CONNECTED;
  float lat = localLatitude;
  float lon = localLongitude;
  bool valid = localPositionSet;

  bool firstEntry = (lastRenderedScreen != MenuScreen::Info);

  if (firstEntry) {
    st7735.st7735_fill_screen(ST7735_BLACK);
    // Etiquetas fijas
    st7735.st7735_write_str(10, 8, "Estado:", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(10, 22, "WiFi:", Font_7x10, ST7735_WHITE);
    drawLine(10, 36, 150, 36, ST7735_GRAY);
    st7735.st7735_write_str(20, 44, "Lat:", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(20, 58, "Lon:", Font_7x10, ST7735_WHITE);

    // Dibuja todos los campos explícitamente
    fillRectPixels(60, 8, 60, 12, ST7735_BLACK);
    const char* modo = remoteMode ? "Remoto" : "Local";
    st7735.st7735_write_str(60, 8, modo, Font_7x10, remoteMode ? MORADO : NARANJA);

    fillRectPixels(60, 22, 70, 12, ST7735_BLACK);
    st7735.st7735_write_str(60, 22, wifi ? "Conectado" : "No conectado", Font_7x10, wifi ? ST7735_GREEN : ST7735_RED);

    fillRectPixels(60, 44, 90, 12, ST7735_BLACK);
    if (valid) {
      char latbuf[16];
      dtostrf(lat, 9, 6, latbuf);
      st7735.st7735_write_str(60, 44, latbuf, Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(60, 44, "--", Font_7x10, ST7735_GRAY);
    }

    fillRectPixels(60, 58, 90, 12, ST7735_BLACK);
    if (valid) {
      char lonbuf[16];
      dtostrf(lon, 10, 6, lonbuf);
      st7735.st7735_write_str(60, 58, lonbuf, Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(60, 58, "--", Font_7x10, ST7735_GRAY);
    }

    // Actualiza los valores cacheados
    lastInfoRemoteMode = remoteMode;
    lastInfoWifi = wifi;
    lastInfoLat = lat;
    lastInfoLon = lon;
    lastInfoValid = valid;
    lastRenderedScreen = MenuScreen::Info;
    return;
  }

  // Estado de modo
  if (lastInfoRemoteMode != remoteMode) {
    fillRectPixels(60, 8, 60, 12, ST7735_BLACK);
    const char* modo = remoteMode ? "Remoto" : "Local";
    st7735.st7735_write_str(60, 8, modo, Font_7x10, remoteMode ? MORADO : NARANJA);
    lastInfoRemoteMode = remoteMode;
  }

  // Estado WiFi
  if (lastInfoWifi != wifi) {
    fillRectPixels(60, 22, 70, 12, ST7735_BLACK);
    st7735.st7735_write_str(60, 22, wifi ? "Conectado" : "No conectado", Font_7x10, wifi ? ST7735_GREEN : ST7735_RED);
    lastInfoWifi = wifi;
  }

  // Latitud
  if (lastInfoLat != lat || lastInfoValid != valid) {
    fillRectPixels(60, 44, 90, 12, ST7735_BLACK);
    if (valid) {
      char latbuf[16];
      dtostrf(lat, 9, 6, latbuf);
      st7735.st7735_write_str(60, 44, latbuf, Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(60, 44, "--", Font_7x10, ST7735_GRAY);
    }
    lastInfoLat = lat;
    lastInfoValid = valid;
  }

  // Longitud
  if (lastInfoLon != lon || lastInfoValid != valid) {
    fillRectPixels(60, 58, 90, 12, ST7735_BLACK);
    if (valid) {
      char lonbuf[16];
      dtostrf(lon, 10, 6, lonbuf);
      st7735.st7735_write_str(60, 58, lonbuf, Font_7x10, ST7735_WHITE);
    } else {
      st7735.st7735_write_str(60, 58, "--", Font_7x10, ST7735_GRAY);
    }
    lastInfoLon = lon;
    lastInfoValid = valid;
  }
}

// Mostrar datos LoRa en pantalla LocalData


void updateLocalLoRaData(const SensorData& data) {
  lastLoRaData = data;
  lastLoRaReceived = millis();
  datosRecibidos = true;
}

// ===== FUNCIONES DE EMPAREJAMIENTO =====

// Verificar solicitudes de emparejamiento pendientes
void checkPairingRequests() {
  // Solo verificar si WiFi está conectado
  if (WiFi.status() != WL_CONNECTED) {
    pairingRequestPending = false;
    pairingDeviceId = "";
    return;
  }
  
  // Usar la función real de Firebase para verificar solicitudes
  if (checkForLinkRequest()) {
    // Si hay una solicitud pendiente, obtener el ID del dispositivo
    // Por ahora usamos un ID genérico, pero se puede mejorar para obtener el real
    pairingRequestPending = true;
    pairingDeviceId = userCorreoSolicitante; // TODO: Obtener ID real desde Firebase
  } else {
    pairingRequestPending = false;
    pairingDeviceId = "";
  }
}

// Aceptar emparejamiento
void acceptPairing() {
  Serial.println("Aceptando emparejamiento con: " + pairingDeviceId);
  
  // Usar la función real de Firebase para aceptar el emparejamiento
  if (acceptLinkRequest()) {
    Serial.println("Emparejamiento aceptado exitosamente");
    // Limpiar el estado local
    pairingRequestPending = false;
    pairingDeviceId = "";
  } else {
    Serial.println("Error al aceptar el emparejamiento");
  }
}

// Rechazar emparejamiento
void rejectPairing() {
  Serial.println("Rechazando emparejamiento con: " + pairingDeviceId);
  
  // Usar la función real de Firebase para rechazar el emparejamiento
  if (deleteLinkRequest()) {
    Serial.println("Emparejamiento rechazado exitosamente");
    // Limpiar el estado local
    pairingRequestPending = false;
    pairingDeviceId = "";
  } else {
    Serial.println("Error al rechazar el emparejamiento");
  }
}

// Función para establecer solicitud de emparejamiento (llamada desde Firebase)
void setPairingRequest(const String& deviceId) {
  pairingRequestPending = true;
  pairingDeviceId = deviceId;
  lastPairingCheck = millis();
}

// Función para limpiar solicitud de emparejamiento
void clearPairingRequest() {
  pairingRequestPending = false;
  pairingDeviceId = "";
}

// Agregar función robusta de mapeo flotante al inicio del archivo (después de includes):
int mapf(float value, float in_min, float in_max, int out_min, int out_max) {
    if (fabs(in_max - in_min) < 1e-8) return (out_min + out_max) / 2;
    return out_min + (int)(((value - in_min) * (out_max - out_min)) / (in_max - in_min));
}