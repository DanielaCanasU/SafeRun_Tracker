#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>
#include "SensorData.h"
#include "OLEDMenu.h"
#include "config.h"
#include "Utils.h"
#include "SH1106Wire.h"

// OLED instance
static SH1106Wire display(0x3C, OLED_SDA, OLED_SCL);

// Menu state
enum class MenuScreen {
  Home,
  SelectMode,
  WiFiMenu,
  WiFiScan,
  WiFiSelectSSID,
  WiFiEnterPassword,
  WiFiConnecting,
  WiFiStatus,
  Info
};

static MenuScreen currentScreen = MenuScreen::WiFiMenu;
static bool remoteMode = true; // true: remote (Firebase), false: local
static bool wifiConnected = false;
static bool screenDirty = true;
static unsigned long lastRenderMs = 0;

// Navigation util placed early so all callers see it
static void goTo(MenuScreen s) { currentScreen = s; screenDirty = true; }

// Buttons active LOW
static bool readBtnUp()   { return digitalRead(BTN_UP_PIN)   == LOW; }
static bool readBtnDown() { return digitalRead(BTN_DOWN_PIN) == LOW; }
static bool readBtnOk()   { return digitalRead(BTN_OK_PIN)   == LOW; }
static bool readBtnBack() { return digitalRead(BTN_BACK_PIN) == LOW; }

// Button debounce
static unsigned long lastBtnMs = 0;
static const unsigned long btnDebounceMs = 180;
static bool canReadButtons(unsigned long now) {
  if (now - lastBtnMs >= btnDebounceMs) { lastBtnMs = now; return true; }
  return false;
}

// Global OK long-press navigation (3s) between top-level menus
static unsigned long okHoldStart = 0;
static bool okHoldHandled = false;
static bool menuSwitchLocked = false; // requiere soltar OK tras cambiar
static bool okWasPressed = false;     // para detectar flanco de liberación
static unsigned long okPressDownMs = 0;
static void gotoNextTopMenu() {
  switch (currentScreen) {
    case MenuScreen::WiFiMenu:     currentScreen = MenuScreen::SelectMode; screenDirty = true; break;
    case MenuScreen::SelectMode:   currentScreen = MenuScreen::Info;       screenDirty = true; break;
    case MenuScreen::Info:         currentScreen = MenuScreen::WiFiMenu;   screenDirty = true; break;
    default:                       currentScreen = MenuScreen::WiFiMenu;   screenDirty = true; break;
  }
}

// WiFi scan/selection
static int16_t numNetworks = 0;
static int selectedNetworkIdx = 0;
static String selectedSSID = "";
static String enteredPassword = "";
static bool wifiScanStarted = false;
static int wifiMenuIdx = 0; // 0 = Escanear redes, 1 = Estado
// Non-blocking WiFi connection state
static bool wifiConnectingActive = false;
static unsigned long wifiConnLastCheckMs = 0;
static int wifiConnTries = 0;
// Preferences for saving WiFi credentials
static Preferences wifiPrefs;
static bool wifiPrefsReady = false;

// Character set for password input
static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.*";
static const int allowedCharsCount = sizeof(allowedChars) - 1;
static int charIndex = 0;

static void drawHeader(const String &title) {
  display.clear();
  display.drawString(0, 0, title);
}

static void drawFooter(const String &hint) {
  // Move footer up a bit to avoid clipping with 128x64 (10px font height)
  display.drawString(0, 54, hint);
}

static void drawFooter2(const String &l1, const String &l2) {
  // Draw two-line footer at y=44 and y=54
  display.drawString(0, 44, l1);
  display.drawString(0, 54, l2);
}

void initOLEDMenu() {
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  if (!wifiPrefsReady) {
    wifiPrefs.begin("wifi", false);
    wifiPrefsReady = true;
  }

  drawHeader("SafeRun Menu");
  display.drawString(0, 16, "- Modo");
  display.drawString(0, 28, "- WiFi");
  display.drawString(0, 40, "- Info");
  drawFooter("OK: entrar, Arr/Abj: nav");
  display.display();
}

static int homeIdx = 0; // 0=Modo,1=WiFi,2=Info

// Removed Home menu. Top-level menus are: WiFiMenu, SelectMode, Info

static void renderMode() {
  static bool lastRemoteRendered = !remoteMode;
  if (screenDirty || lastRemoteRendered != remoteMode) {
    drawHeader("Seleccionar Modo");
    // Clear option rows then redraw
    display.setColor(BLACK); display.fillRect(0, 16, 128, 32); display.setColor(WHITE);
    display.drawString(0, 20, String(remoteMode ? "> Remoto" : "  Remoto"));
    display.drawString(0, 32, String(!remoteMode ? "> Local" : "  Local"));
    drawFooter("OK: cambiar | OK3s menú");
    display.display();
    lastRemoteRendered = remoteMode;
    screenDirty = false;
  }
}

static void renderWiFiMenu() {
  static int lastIdx = -1;
  static bool lastWifiConn = !wifiConnected;
  if (screenDirty) {
    drawHeader("WiFi");
    // Clear body area
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    drawFooter2("Arr/Abj mover | OK entrar", "OK3s menú");
    screenDirty = false;
    lastIdx = -1; // force text redraw below
    lastWifiConn = !wifiConnected;
  }
  if (lastIdx != wifiMenuIdx || lastWifiConn != wifiConnected) {
    // Clear body rows
    display.setColor(BLACK); display.fillRect(0, 16, 128, 28); display.setColor(WHITE);
    display.drawString(0, 16, String(wifiMenuIdx == 0 ? "> " : "  ") + "Escanear redes");
    String est = String(wifiConnected ? "Conectado" : "No conectado");
    display.drawString(0, 28, String(wifiMenuIdx == 1 ? "> " : "  ") + "Estado: " + est);
    display.display();
    lastIdx = wifiMenuIdx;
    lastWifiConn = wifiConnected;
  }
}

static void renderWiFiScanning() {
  if (screenDirty) {
    drawHeader("Escaneando...");
    drawFooter("OK3s menú");
    display.display();
    screenDirty = false;
  }
}

static void renderWiFiSelect() {
  static int lastSel = -1;
  if (screenDirty) {
    drawHeader("Seleccione SSID");
    drawFooter2("Arr/Abj mover | OK clave", "OK3s menú");
    // Clear list area
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    screenDirty = false;
    lastSel = -1;
  }
  if (lastSel != selectedNetworkIdx) {
    // Redraw list
    display.setColor(BLACK); display.fillRect(0, 16, 128, 40); display.setColor(WHITE);
    int y = 16;
    for (int i = 0; i < 4 && i < numNetworks; i++) {
      int idx = (selectedNetworkIdx + i) % numNetworks;
      String prefix = (i == 0 ? "> " : "  ");
      String ssid = WiFi.SSID(idx);
      int32_t rssi = WiFi.RSSI(idx);
      display.drawString(0, y, prefix + ssid + " (" + String(rssi) + "dBm)");
      y += 12;
    }
    display.display();
    lastSel = selectedNetworkIdx;
  }
}

static void renderPasswordInput() {
  static String lastSSID = "";
  static String lastPwdPreview = "";
  if (screenDirty || lastSSID != selectedSSID) {
    drawHeader("Clave para:");
    // Clear body area
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    display.drawString(0, 12, selectedSSID);
    display.drawString(0, 42, "Arr/Abj letra | OK añadir");
    drawFooter("OK3s conectar");
    display.display();
    screenDirty = false;
    lastSSID = selectedSSID;
    lastPwdPreview = ""; // force draw below
  }
  String preview = String("Pwd: ") + enteredPassword + String(allowedChars[charIndex]) + "_";
  if (lastPwdPreview != preview) {
    // Clear password row and redraw
    display.setColor(BLACK); display.fillRect(0, 28, 128, 12); display.setColor(WHITE);
    display.drawString(0, 28, preview);
    display.display();
    lastPwdPreview = preview;
  }
}

static void renderConnecting() {
  if (screenDirty) {
    drawHeader("Conectando a:");
    // Clear body
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    display.drawString(0, 16, selectedSSID);
    drawFooter("Espere...");
    display.display();
    screenDirty = false;
  }
}

static void renderWiFiStatus() {
  static String lastSsidShown = "~";
  static String lastIpShown = "~";
  if (screenDirty) {
    drawHeader("Estado WiFi");
    // Clear body area
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    drawFooter2("OK: volver", "OK3s menú");
    display.display();
    screenDirty = false;
    lastSsidShown = "~"; lastIpShown = "~";
  }
  String ss = wifiConnected ? WiFi.SSID() : String("-");
  String ip = wifiConnected ? WiFi.localIP().toString() : String("");
  if (lastSsidShown != ss) {
    display.setColor(BLACK); display.fillRect(0, 16, 128, 12); display.setColor(WHITE);
    display.drawString(0, 16, "SSID: " + ss);
    display.display();
    lastSsidShown = ss;
  }
  if (wifiConnected && lastIpShown != ip) {
    display.setColor(BLACK); display.fillRect(0, 28, 128, 12); display.setColor(WHITE);
    display.drawString(0, 28, "IP: " + ip);
    display.display();
    lastIpShown = ip;
  }
}

void updateOLEDMenu(unsigned long now) {
  // Render current screen (throttle to avoid excessive updates)
  switch (currentScreen) {
    case MenuScreen::SelectMode:
      renderMode();
      break;
    case MenuScreen::WiFiMenu:
      renderWiFiMenu();
      break;
    case MenuScreen::WiFiScan:
      renderWiFiScanning();
      break;
    case MenuScreen::WiFiSelectSSID:
      renderWiFiSelect();
      break;
    case MenuScreen::WiFiEnterPassword:
      renderPasswordInput();
      break;
    case MenuScreen::WiFiConnecting:
      renderConnecting();
      break;
    case MenuScreen::WiFiStatus:
      renderWiFiStatus();
      break;
    case MenuScreen::Info:
      // No dibujar aquí para evitar borrar el cuerpo; renderLocalLoRaOnOLED se encarga
      break;
  }

  // If in WiFiScan, start async scan and poll without blocking loop
  if (currentScreen == MenuScreen::WiFiScan) {
    if (!wifiScanStarted) {
      wifiScanStarted = true;
      WiFi.mode(WIFI_STA);
      WiFi.disconnect(true);
      delay(50);
      WiFi.scanDelete(); // clear previous results to avoid stale data
      WiFi.scanNetworks(true); // async start
      selectedNetworkIdx = 0;
    } else {
      int res = WiFi.scanComplete();
      if (res >= 0) {
        numNetworks = res;
        // Do NOT delete scan results yet; they are needed by WiFi.SSID()/RSSI() in selection screen
        if (numNetworks <= 0) {
          drawHeader("Sin redes");
          drawFooter("OK3s menú");
          display.display();
          delay(600);
          wifiScanStarted = false;
          goTo(MenuScreen::WiFiMenu);
        } else {
          wifiScanStarted = false;
          goTo(MenuScreen::WiFiSelectSSID);
        }
      }
      // if res == -2 still running; if -1 not started
    }
  }

  // If in WiFiConnecting, poll connection without blocking
  if (currentScreen == MenuScreen::WiFiConnecting && wifiConnectingActive) {
    if (now - wifiConnLastCheckMs >= 300) {
      wifiConnLastCheckMs = now;
      wifiConnTries++;
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        drawHeader("Conectado");
        display.drawString(0, 16, WiFi.localIP().toString());
        drawFooter("OK: volver");
        display.display();
        // wait a short moment then go back to WiFi menu on short OK
      } else if (wifiConnTries > 30) {
        wifiConnected = false;
        drawHeader("Fallo conexion");
        drawFooter("OK: volver");
        display.display();
      }
    }
  }

  // Handle global OK press/hold for top-level screens
  bool onTopMenu = (currentScreen == MenuScreen::SelectMode || currentScreen == MenuScreen::WiFiMenu || currentScreen == MenuScreen::Info);
  if (onTopMenu) {
    bool okPressed = (digitalRead(BTN_OK_PIN) == LOW);
    if (okPressed) {
      if (okPressDownMs == 0) okPressDownMs = now;
      if (!okHoldHandled && !menuSwitchLocked && (now - okPressDownMs >= 3000)) {
        gotoNextTopMenu();
        okHoldHandled = true;
        menuSwitchLocked = true; // bloquear hasta soltar botón
      }
    } else {
      // En liberación de OK, si no fue long-press, ejecutar acción corta del menú actual
      if (okWasPressed) {
        unsigned long dur = okPressDownMs > 0 ? (now - okPressDownMs) : 0;
        if (!okHoldHandled && dur >= 50 && dur < 800) {
          if (currentScreen == MenuScreen::WiFiMenu) {
            if (wifiMenuIdx == 0) { wifiScanStarted = false; goTo(MenuScreen::WiFiScan); }
            else { goTo(MenuScreen::WiFiStatus); }
          } else if (currentScreen == MenuScreen::SelectMode) {
            remoteMode = !remoteMode;
          }
        }
      }
      // Reset en soltar
      okHoldStart = 0;
      okHoldHandled = false;
      menuSwitchLocked = false;
      okPressDownMs = 0;
    }
    okWasPressed = okPressed;
  }

  if (!canReadButtons(now)) return;

  // Handle navigation
  switch (currentScreen) {
    // No Home menu

    case MenuScreen::SelectMode:
      if (readBtnUp() || readBtnDown()) { remoteMode = !remoteMode; }
      // OK corto se maneja en el handler de top-level (al soltar)
      break;

    case MenuScreen::WiFiMenu:
      if (readBtnUp())   { wifiMenuIdx = (wifiMenuIdx + 1) % 2; }
      if (readBtnDown()) { wifiMenuIdx = (wifiMenuIdx + 1) % 2; }
      // OK corto se maneja en el handler de top-level (al soltar)
      break;

    case MenuScreen::WiFiSelectSSID:
      if (numNetworks <= 0) { if (readBtnBack()) goTo(MenuScreen::WiFiMenu); break; }
      if (readBtnUp())   { selectedNetworkIdx = (selectedNetworkIdx - 1 + numNetworks) % numNetworks; }
      if (readBtnDown()) { selectedNetworkIdx = (selectedNetworkIdx + 1) % numNetworks; }
      if (readBtnOk()) {
        selectedSSID = WiFi.SSID(selectedNetworkIdx);
        // Prefill password if saved for this SSID
        if (wifiPrefsReady) {
          String savedSsid = wifiPrefs.getString("ssid", "");
          String savedPass = wifiPrefs.getString("pass", "");
          if (savedSsid == selectedSSID) {
            enteredPassword = savedPass;
          } else {
            enteredPassword = "";
          }
        } else {
          enteredPassword = "";
        }
        charIndex = 0;
        goTo(MenuScreen::WiFiEnterPassword);
      }
      // OK largo para volver a top-level, no hay Back
      break;

    case MenuScreen::WiFiEnterPassword:
      if (readBtnUp())   { charIndex = (charIndex + 1) % allowedCharsCount; }
      if (readBtnDown()) { charIndex = (charIndex - 1 + allowedCharsCount) % allowedCharsCount; }
      if (readBtnBack()) {
        if (enteredPassword.length() > 0) {
          enteredPassword.remove(enteredPassword.length() - 1);
        } else {
          goTo(MenuScreen::WiFiSelectSSID);
        }
      }
      // OK handling with edge detection: short press adds char, long press connects
      {
        static bool okPwdWasPressed = false;
        static unsigned long okPwdDownMs = 0;
        const unsigned long longPressMs = 1200;
        bool okPressed = (digitalRead(BTN_OK_PIN) == LOW);
        if (okPressed) {
          if (!okPwdWasPressed) okPwdDownMs = now;
          okPwdWasPressed = true;
        } else {
          if (okPwdWasPressed) {
            unsigned long dur = now - okPwdDownMs;
            if (dur >= longPressMs) {
              goTo(MenuScreen::WiFiConnecting);
              renderConnecting();
              WiFi.mode(WIFI_STA);
              WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());
              if (wifiPrefsReady) {
                wifiPrefs.putString("ssid", selectedSSID);
                wifiPrefs.putString("pass", enteredPassword);
              }
              wifiConnectingActive = true;
              wifiConnLastCheckMs = now;
              wifiConnTries = 0;
            } else if (dur >= 50) {
              enteredPassword += allowedChars[charIndex];
            }
          }
          okPwdWasPressed = false;
          okPwdDownMs = 0;
        }
      }
      break;

    case MenuScreen::WiFiConnecting:
      // Allow OK short to exit result and return to WiFi menu once result shown
      if (wifiConnTries > 0 && (WiFi.status() == WL_CONNECTED || wifiConnTries > 30)) {
        if (readBtnOk()) {
          wifiConnectingActive = false;
          goTo(MenuScreen::WiFiMenu);
        }
      }
      break;

    case MenuScreen::Info:
      if (readBtnBack()) { goTo(MenuScreen::Home); }
      break;
    case MenuScreen::WiFiStatus:
      if (readBtnOk() && okHoldStart == 0 && !okHoldHandled) { goTo(MenuScreen::WiFiMenu); }
      break;
  }
}

bool isRemoteModeSelected() { return remoteMode; }
bool isWiFiConnectedViaMenu() { return wifiConnected; }

void renderLocalLoRaOnOLED(const SensorData &data) {
  if (currentScreen != MenuScreen::Info) return; // solo dibujar en pantalla LoRa
  static float lastLat = 123456.0f, lastLon = 123456.0f; static int lastRssi = 98765; static int lastSnr = 98765; static int lastS4 = -1;
  if (screenDirty) {
    drawHeader("LoRa datos");
    // Clear body area
    display.setColor(BLACK); display.fillRect(0, 12, 128, 44); display.setColor(WHITE);
    drawFooter("OK 3s: menú");
    display.display();
    // Force first draw regardless of values
    lastLat = 123456.0f; lastLon = 123456.0f; lastRssi = 98765; lastSnr = 98765; lastS4 = -1;
    screenDirty = false;
  }
  // Lat
  if (fabs(data.latitude - lastLat) > 0.00001f) {
    display.setColor(BLACK); display.fillRect(0, 16, 128, 12); display.setColor(WHITE);
    display.drawString(0, 16, String("Lat:") + String(data.latitude, 5));
    display.display();
    lastLat = data.latitude;
  }
  // Lon
  if (fabs(data.longitude - lastLon) > 0.00001f) {
    display.setColor(BLACK); display.fillRect(0, 28, 128, 12); display.setColor(WHITE);
    display.drawString(0, 28, String("Lon:") + String(data.longitude, 5));
    display.display();
    lastLon = data.longitude;
  }
  // RSSI/SNR
  if (data.rssi != lastRssi || data.snr != lastSnr) {
    display.setColor(BLACK); display.fillRect(0, 40, 128, 12); display.setColor(WHITE);
    display.drawString(0, 40, String("RSSI:") + String(data.rssi) + " SNR:" + String(data.snr));
    display.display();
    lastRssi = data.rssi; lastSnr = data.snr;
  }
  // Emergency
  if (data.sensor4 != lastS4) {
    display.setColor(BLACK); display.fillRect(0, 52, 128, 12); display.setColor(WHITE);
    display.drawString(0, 52, String("Emergencia: ") + (data.sensor4 ? "SI" : "NO"));
    display.display();
    lastS4 = data.sensor4;
  }
}


