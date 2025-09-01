#include "UI.h"
#include "Config.h"
#include "AppState.h"
#include "TrackingData.h"
#include <WiFi.h>
#include <Preferences.h>

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
enum class MenuScreen { Welcome, SelectMode, WiFiMenu, WiFiSubMenu, WiFiScan, WiFiSelectSSID, WiFiEnterPassword, WiFiConnecting, WiFiStatus, Info, LocalData, Tracking, Backtrack, WaypointManager };
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
static int wifiMenuIdx = 0; // 0 = Escanear redes, 1 = Estado
static bool wifiConnectingActive = false;
static unsigned long wifiConnLastCheckMs = 0;
static int wifiConnTries = 0;
static Preferences wifiPrefs;
static bool wifiPrefsReady = false;

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.*";
static const int allowedCharsCount = sizeof(allowedChars) - 1;
static int charIndex = 0;

// Cache for optimized rendering
static MenuScreen lastRenderedScreen = MenuScreen::Info;
static bool lastRemoteMode = false;
static int lastWifiMenuIdx = -1;
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
static int mainMenuIdx = 0; // 0 = Modo, 1 = WiFi, 2 = Datos Locales, 3 = Info, 4 = Rastreo, 5 = Backtrack
static int lastMainMenuIdx = -1;

// Tracking system variables
static TrackingData targetData;
static NavigationData currentNavigation;
static bool hasTargetData = false;
static unsigned long lastTargetUpdate = 0;
static const unsigned long TARGET_TIMEOUT = 30000; // 30 segundos timeout

// Local GPS position from Heltec device
static float localLatitude = 0.0f;
static float localLongitude = 0.0f;
static bool localPositionSet = false;
static unsigned long lastLocalGPSUpdate = 0;

// Estructura para waypoints del backtrack
struct Waypoint {
  float latitude;
  float longitude;
  String name;
  unsigned long timestamp;
  bool isValid;
};

// Sistema de backtrack
static const int MAX_WAYPOINTS = 10;
static Waypoint waypoints[MAX_WAYPOINTS];
static int waypointCount = 0;
static int selectedWaypointIndex = -1;
static Preferences waypointPrefs;
static bool waypointPrefsReady = false;

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
static void loadWaypointsFromStorage();
static void saveWaypointsToStorage();
static void updateWaypointNavigation();

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
    case 2: { // Datos Locales: larger storage box
      drawLine(x + 2, y + 4, x + 15, y + 4, fg);
      drawLine(x + 2, y + 14, x + 15, y + 14, fg);
      drawLine(x + 2, y + 4, x + 2, y + 14, fg);
      drawLine(x + 15, y + 4, x + 15, y + 14, fg);
      // divider
      drawLine(x + 2, y + 9, x + 15, y + 9, fg);
    } break;
    case 3: { // Info: larger circle with i
      drawCircle(x + 9, y + 9, 7, fg);
      drawLine(x + 9, y + 6, x + 9, y + 11, fg);
      st7735.st7735_draw_pixel(x + 9, y + 5, fg);
    } break;
         case 4: { // Rastreo: brújula/radar icon
       // Círculo exterior
       drawCircle(x + 9, y + 9, 7, fg);
       // Flecha central apuntando hacia arriba
       drawLine(x + 9, y + 2, x + 9, y + 16, fg);
       // Puntas de la flecha
       drawLine(x + 9, y + 2, x + 6, y + 5, fg);
       drawLine(x + 9, y + 2, x + 12, y + 5, fg);
     } break;
     case 5: { // Backtrack: flecha de retorno
       // Círculo exterior
       drawCircle(x + 9, y + 9, 7, fg);
       // Flecha curva de retorno (forma de U)
       drawLine(x + 9, y + 2, x + 9, y + 12, fg); // Línea vertical
       drawLine(x + 9, y + 12, x + 2, y + 12, fg); // Línea horizontal izquierda
       drawLine(x + 2, y + 12, x + 2, y + 16, fg); // Línea vertical izquierda
       // Puntas de la flecha
       drawLine(x + 2, y + 16, x + 5, y + 13, fg);
       drawLine(x + 2, y + 16, x + 5, y + 16, fg);
     } break;
  }
}

static void renderMainMenu() {
  // Only redraw when the selected index or screen changes to avoid flicker
  if (lastRenderedScreen != MenuScreen::WiFiMenu || lastMainMenuIdx != mainMenuIdx) {
    drawHeaderWithWiFi("Inicio");

    // Clear menu area (keep header)
    st7735.st7735_fill_rectangle(0, 10, 128, 70, ST7735_BLACK);

    // Menu options
    const char* menuItems[] = {"Modo", "WiFi", "Datos Locales", "Info", "Rastreo", "Backtrack"};

    // Draw option above (if exists)
    if (mainMenuIdx > 0) {
      int aboveIdx = mainMenuIdx - 1;
      String aboveText = String("^ ") + String(menuItems[aboveIdx]);
      // Calculate centered positions
      int iconX = 35; // Center of screen (128/2 - 18/2 = 55)
      int textX = 55; // Icon center + icon width/2 + spacing
      // Icon for above (on black bg) - centered
      drawMenuIcon(iconX, 15, aboveIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(textX, 20, aboveText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    // Draw selected option (highlight full width)
    String selectedText = String(menuItems[mainMenuIdx]);
    fillRectPixels(0, 34, 160, 26, ST7735_WHITE);
    // Calculate centered positions for selected item
    int selectedIconX = 35; // Center of screen
    int selectedTextX = 55; // Icon center + icon width/2 + spacing
    // Icon for selected (on white bg) - centered
    drawMenuIcon(selectedIconX, 34, mainMenuIdx, true, ST7735_WHITE);
    write_str_bold(selectedTextX, 38, selectedText.c_str(), Font_11x18, ST7735_BLACK, ST7735_WHITE);

    // Draw option below (if exists)
    if (mainMenuIdx < 5) { // Cambiado de 3 a 5 para mostrar la opción de abajo
      int belowIdx = mainMenuIdx + 1;
      String belowText = String("v ") + String(menuItems[belowIdx]);
      // Calculate centered positions
      int belowIconX = 35; // Center of screen
      int belowTextX = 55; // Icon center + icon width/2 + spacing
      // Icon for below (on black bg) - centered
      drawMenuIcon(belowIconX, 61, belowIdx, false, ST7735_BLACK);
      st7735.st7735_write_str(belowTextX, 65, belowText.c_str(), Font_7x10, ST7735_GRAY, ST7735_BLACK);
    }

    lastRenderedScreen = MenuScreen::WiFiMenu;
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
  }

  //drawFooter("OK: cambiar | OK3s menú");
}

static void renderWiFiMenu() {
  String currentWifiStatus = String(WiFi.status() == WL_CONNECTED ? "Conectado" : "No conectado");
  
  // Only redraw if screen changed or menu index changed or wifi status changed
  if (lastRenderedScreen != MenuScreen::WiFiMenu || lastWifiMenuIdx != wifiMenuIdx || lastWifiStatus != currentWifiStatus) {
    drawHeaderWithWiFi("WiFi");
    lastRenderedScreen = MenuScreen::WiFiMenu;
    lastWifiMenuIdx = wifiMenuIdx;
    lastWifiStatus = currentWifiStatus;
  }
  
  // Update only the changing parts
  st7735.st7735_write_str(0, 20, (wifiMenuIdx == 0 ? "> " : "  ") + String("Escanear redes"), Font_7x10, wifiMenuIdx == 0 ? MORADO : ST7735_WHITE);
  st7735.st7735_write_str(0, 32, (wifiMenuIdx == 1 ? "> " : "  ") + String("Estado: ") + currentWifiStatus, Font_7x10, wifiMenuIdx == 1 ? MORADO : ST7735_WHITE);
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
                  prevReceived != lastLoRaReceived);
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
    snprintf(buf, sizeof(buf), "Recibido: %lus", lastLoRaReceived / 1000);
    st7735.st7735_write_str(0, 64, buf, Font_7x10, ST7735_GRAY);
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

// ===== PANTALLAS DE BACKTRACK =====

// Pantalla de gestión de waypoints
static void renderWaypointManager() {
  if (lastRenderedScreen != MenuScreen::WaypointManager) {
    drawHeaderWithWiFi("Gestionar Puntos");
    lastRenderedScreen = MenuScreen::WaypointManager;
    
    // Inicializar preferencias si no están listas
    initWaypointPrefs();
  }
  
  // Sistema de scroll para waypoints (mostrar solo 3 por pantalla)
  static int scrollOffset = 0;
  int maxScrollOffset = max(0, waypointCount - 3);
  
  // Mostrar solo 3 waypoints por pantalla
  int yPos = 16;
  for (int i = 0; i < 3 && (i + scrollOffset) < waypointCount; i++) {
    int actualIndex = i + scrollOffset;
    String waypointText = String(actualIndex + 1) + ". " + waypoints[actualIndex].name;
    if (waypointText.length() > 18) {
      waypointText = waypointText.substring(0, 15) + "...";
    }
    
    uint16_t textColor = (actualIndex == selectedWaypointIndex) ? MORADO : ST7735_WHITE;
    st7735.st7735_write_str(0, yPos, waypointText.c_str(), Font_7x10, textColor);
    
    // Mostrar coordenadas abreviadas
    String coords = String(waypoints[actualIndex].latitude, 4) + "," + String(waypoints[actualIndex].longitude, 4);
    st7735.st7735_write_str(0, yPos + 8, coords.c_str(), Font_7x10, ST7735_GRAY);
    
    yPos += 16;
  }
  
  // Indicadores de scroll
  if (waypointCount > 3) {
    if (scrollOffset > 0) {
      st7735.st7735_write_str(140, 16, "^", Font_7x10, ST7735_GRAY); // Flecha arriba
    }
    if (scrollOffset < maxScrollOffset) {
      st7735.st7735_write_str(140, 56, "v", Font_7x10, ST7735_GRAY); // Flecha abajo
    }
  }
  
  // Mostrar opciones
  if (waypointCount == 0) {
    st7735.st7735_write_str(0, 40, "No hay puntos guardados", Font_7x10, ST7735_GRAY);
    st7735.st7735_write_str(0, 52, "OK3s: Agregar punto", Font_7x10, NARANJA);
  } else {
    st7735.st7735_write_str(0, 50, "OK: Entrar a punto", Font_7x10, ST7735_WHITE);
    st7735.st7735_write_str(0, 60, "OK3s: Agregar punto", Font_7x10, NARANJA);
    st7735.st7735_write_str(0, 70, "BACK: Eliminar punto", Font_7x10, ST7735_RED);
  }
  
 drawFooter("BACK: eliminar | BACK3s: menu | OK3s: agregar");
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
   drawFooter("OK: waypoints | BACK: menu");
}

void initUI() {
  // Configure buttons
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);

  // Initialize display
  st7735.st7735_init();

  // Initialize wifi preferences storage (NVS)
  if (!wifiPrefsReady) {
    wifiPrefs.begin("wifi", false);
    wifiPrefsReady = true;
  }
  
  // Initialize waypoint preferences storage (NVS)
  if (!waypointPrefsReady) {
    waypointPrefs.begin("waypoints", false);
    waypointPrefsReady = true;
    loadWaypointsFromStorage();
  }

  // Start with welcome screen
  welcomeStartTime = millis();
  welcomePhase = 0;
  currentScreen = MenuScreen::Welcome;
  
  // Draw initial welcome screen
  renderWelcomeScreen();
}

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
    currentScreen = MenuScreen::WiFiMenu;
    lastRenderedScreen = MenuScreen::Welcome;
    renderMainMenu();
  }
}

void renderBlinkingEyes() {
  drawBlinkingEyes();
}

void renderWelcomeMessage() {
  drawWelcomeMessage();
}

bool isWelcomeScreenComplete() {
  return welcomeScreenShown;
}

void updateUI(unsigned long now) {
  // Handle welcome screen
  if (currentScreen == MenuScreen::Welcome) {
    // Permitir saltar la pantalla de bienvenida con cualquier botón
    if (readBtnOk() || readBtnUp() || readBtnDown() || readBtnBack()) {
      welcomeScreenShown = true;
      currentScreen = MenuScreen::WiFiMenu;
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
    case MenuScreen::WiFiMenu: {
                     // Main menu navigation
        if (readBtnUp())   mainMenuIdx = (mainMenuIdx - 1 + 6) % 6;
        if (readBtnDown()) mainMenuIdx = (mainMenuIdx + 1) % 6;
      
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
             }
          }
        }
        okWasPressed = false;
      }
    } break;
    
    case MenuScreen::SelectMode: {
      if (readBtnUp() || readBtnDown()) {
        setRemoteMode(!isRemoteModeSelected());
        lastRemoteMode = !lastRemoteMode; // También actualizar el cache
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
          currentScreen = MenuScreen::WiFiMenu; // Back to main menu
          okWasPressed = false;
        }
      } else {
        // Reset long press detection when button is released
        okWasPressed = false;
      }
    } break;
    
    case MenuScreen::WiFiSubMenu: {
      // WiFi submenu (when coming from main menu)
      if (readBtnUp())   wifiMenuIdx = (wifiMenuIdx - 1 + 2) % 2;
      if (readBtnDown()) wifiMenuIdx = (wifiMenuIdx + 1) % 2;
      
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
          currentScreen = MenuScreen::WiFiMenu; // Back to main menu
          okWasPressed = false;
        }
      } else {
        // Button released
        if (okWasPressed) {
          unsigned long pressDuration = now - okDownMs;
          if (pressDuration >= 50 && pressDuration < shortPressMs) {
            // Short press: enter selected submenu
            if (wifiMenuIdx == 0) currentScreen = MenuScreen::WiFiScan;
            else currentScreen = MenuScreen::WiFiStatus;
          }
        }
        okWasPressed = false;
      }
    } break;
    
    case MenuScreen::WiFiSelectSSID:
      if (numNetworks <= 0) { if (readBtnBack()) currentScreen = MenuScreen::WiFiSubMenu; break; }
      if (readBtnUp())   selectedNetworkIdx = (selectedNetworkIdx - 1 + numNetworks) % numNetworks;
      if (readBtnDown()) selectedNetworkIdx = (selectedNetworkIdx + 1) % numNetworks;
      if (readBtnOk()) {
        selectedSSID = WiFi.SSID(selectedNetworkIdx);
        if (!wifiPrefsReady) { wifiPrefs.begin("wifi", false); wifiPrefsReady = true; }
        String savedSsid = wifiPrefs.getString("ssid", "");
        String savedPass = wifiPrefs.getString("pass", "");
        if (savedSsid == selectedSSID) enteredPassword = savedPass; else enteredPassword = "";
        charIndex = 0; currentScreen = MenuScreen::WiFiEnterPassword;
      }
      break;
    case MenuScreen::WiFiEnterPassword: {
      if (readBtnUp())   charIndex = (charIndex + 1) % allowedCharsCount;
      if (readBtnDown()) charIndex = (charIndex - 1 + allowedCharsCount) % allowedCharsCount;
      if (readBtnBack()) {
        if (enteredPassword.length() > 0) enteredPassword.remove(enteredPassword.length() - 1);
        else currentScreen = MenuScreen::WiFiSelectSSID;
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
      break;
    case MenuScreen::WiFiStatus:
      if (readBtnOk()) { currentScreen = MenuScreen::WiFiSubMenu; lastRenderedScreen = MenuScreen::Info; }
      break;
         case MenuScreen::WiFiScan:
     case MenuScreen::Info:
     case MenuScreen::LocalData:
              case MenuScreen::Tracking:
      // Check for long press (3 seconds) to go to main menu from Info/LocalData screen
      if (readBtnOk()) {
        if (!infoOkWasPressed) {
          infoOkDownMs = now;
          infoOkWasPressed = true;
        } else if (now - infoOkDownMs >= infoLongPressMs) {
          currentScreen = MenuScreen::WiFiMenu; // Back to main menu
          lastRenderedScreen = MenuScreen::Info; // Force redraw
          infoOkWasPressed = false;
        }
      } else {
        infoOkWasPressed = false;
      }
      break;
      
    case MenuScreen::WaypointManager: {
      // Navegación en la lista de waypoints con scroll
      static int scrollOffset = 0;
      int maxScrollOffset = max(0, waypointCount - 3);
      
      if (readBtnUp()) {
        if (waypointCount > 0) {
          selectedWaypointIndex = (selectedWaypointIndex - 1 + waypointCount) % waypointCount;
          
          // Ajustar scroll si es necesario
          if (selectedWaypointIndex < scrollOffset) {
            scrollOffset = max(0, selectedWaypointIndex);
          }
        }
      }
      if (readBtnDown()) {
        if (waypointCount > 0) {
          selectedWaypointIndex = (selectedWaypointIndex + 1) % waypointCount;
          
          // Ajustar scroll si es necesario
          if (selectedWaypointIndex >= scrollOffset + 3) {
            scrollOffset = min(maxScrollOffset, selectedWaypointIndex - 2);
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
          // BACK largo (3s): volver al menú principal
          currentScreen = MenuScreen::WiFiMenu;
          lastRenderedScreen = MenuScreen::Info;
          wpBackWasPressed = false;
        }
      } else {
        // Botón liberado
        if (wpBackWasPressed) {
          unsigned long pressDuration = now - wpBackDownMs;
          if (pressDuration >= 50 && pressDuration < wpBackLongPressMs) {
            // BACK corto: eliminar waypoint seleccionado
            if (waypointCount > 0 && selectedWaypointIndex >= 0) {
              deleteWaypoint(selectedWaypointIndex);
              lastRenderedScreen = MenuScreen::Info; // Force redraw
            }
          }
        }
        wpBackWasPressed = false;
      }
      break;
    }
    
         case MenuScreen::Backtrack: {
       // OK corto: volver a gestión de waypoints
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
           currentScreen = MenuScreen::WiFiMenu;
           lastRenderedScreen = MenuScreen::Info;
           btOkWasPressed = false;
         }
       } else {
         btOkWasPressed = false;
       }
       
       // Botón BACK: volver al menú principal directamente
       if (readBtnBack()) {
         currentScreen = MenuScreen::WiFiMenu;
         lastRenderedScreen = MenuScreen::Info;
       }
       break;
     }
  }

render:
  // Render
  switch (currentScreen) {
    case MenuScreen::WiFiMenu: 
      // Always render main menu when in WiFiMenu state
      renderMainMenu();
      break;
    case MenuScreen::SelectMode: renderMode(); break;
    case MenuScreen::WiFiSubMenu: renderWiFiMenu(); break;
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
          if (!wifiPrefsReady) { wifiPrefs.begin("wifi", false); wifiPrefsReady = true; }
          wifiPrefs.putString("ssid", selectedSSID);
          wifiPrefs.putString("pass", enteredPassword);
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
  if (!waypointPrefsReady) {
    waypointPrefs.begin("waypoints", false);
    waypointPrefsReady = true;
    loadWaypointsFromStorage();
  }
}

// Cargar waypoints desde almacenamiento
static void loadWaypointsFromStorage() {
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
}

// Guardar waypoints en almacenamiento
static void saveWaypointsToStorage() {
  if (!waypointPrefsReady) initWaypointPrefs();
  
  waypointPrefs.putInt("count", waypointCount);
  for (int i = 0; i < waypointCount; i++) {
    String prefix = "wp" + String(i) + "_";
    waypointPrefs.putFloat((prefix + "lat").c_str(), waypoints[i].latitude);
    waypointPrefs.putFloat((prefix + "lon").c_str(), waypoints[i].longitude);
    waypointPrefs.putString((prefix + "name").c_str(), waypoints[i].name);
    waypointPrefs.putULong((prefix + "time").c_str(), waypoints[i].timestamp);
  }
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
  
  if (waypointPrefsReady) {
    waypointPrefs.clear();
  }
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
}


