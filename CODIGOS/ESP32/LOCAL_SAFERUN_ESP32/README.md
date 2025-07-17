# ESP32 SafeRun Device - Modular Architecture

Este proyecto implementa un dispositivo ESP32 para monitoreo de deportistas con arquitectura modular y profesional.

## 📁 Estructura del Proyecto

```
LOCAL_SAFERUN_ESP32/
├── include/                     # Archivos de cabecera
│   ├── config.h                # Configuración y constantes
│   ├── SensorData.h            # Estructura de datos de sensores
│   ├── Utils.h                 # Funciones utilitarias
│   ├── ButtonHandler.h         # Manejo de botones e interrupciones
│   ├── LoRaHandler.h           # Configuración y manejo de LoRa
│   ├── FirebaseHandler.h       # Integración con Firebase
│   └── SX1262_Settings.h       # Configuración específica del SX1262
├── src/                        # Archivos de implementación
│   ├── main.cpp                # Archivo principal (setup/loop)
│   ├── SensorData.cpp          # Implementación de funciones de datos
│   ├── Utils.cpp               # Implementación de utilidades
│   ├── ButtonHandler.cpp       # Implementación de manejo de botones
│   ├── LoRaHandler.cpp         # Implementación de manejo LoRa
│   └── FirebaseHandler.cpp     # Implementación de Firebase
├── lib/                        # Bibliotecas locales
├── test/                       # Tests unitarios
└── platformio.ini             # Configuración de PlatformIO
```

## 🏗️ Arquitectura Modular

### 1. **config.h** - Configuración Centralizada
- Pines de conexión
- Credenciales WiFi y Firebase
- Constantes de timing
- Configuración de botones
- Parámetros LoRa

### 2. **SensorData.h/cpp** - Manejo de Datos
- Estructura `SensorData` para almacenar información de sensores
- Funciones de parsing de mensajes LoRa
- Validación de datos
- Impresión de información para debug

### 3. **Utils.h/cpp** - Utilidades Generales
- Sincronización de tiempo NTP
- Generación de timestamps
- Funciones de logging con niveles
- Manejo de WiFi
- Formateo de tiempo

### 4. **ButtonHandler.h/cpp** - Manejo de Botones
- Interrupciones de botón
- Detección de patrones (3 clicks + long press)
- Debouncing y timeout
- Callbacks para acciones

### 5. **LoRaHandler.h/cpp** - Comunicación LoRa
- Configuración del módulo SX1262
- Recepción de mensajes
- Parsing de paquetes
- Estadísticas de comunicación
- Compatibilidad con RYLR998

### 6. **FirebaseHandler.h/cpp** - Integración Firebase
- Autenticación y conexión
- Publicación de datos de sensores
- Manejo de solicitudes de vinculación
- Operaciones CRUD en Firestore

### 7. **main.cpp** - Orquestador Principal
- Inicialización de todos los módulos
- Loop principal coordinado
- Manejo de errores y estados
- Callbacks entre módulos

## 🚀 Características

### Funcionalidades Principales
- ✅ Recepción de datos de sensores vía LoRa
- ✅ Almacenamiento en Firebase Firestore
- ✅ Sistema de vinculación de dispositivos
- ✅ Interfaz de usuario con botón físico
- ✅ Sincronización de tiempo NTP
- ✅ Logging estructurado
- ✅ Manejo robusto de errores

### Mejoras de Arquitectura
- ✅ Separación clara de responsabilidades
- ✅ Código reutilizable y mantenible
- ✅ Configuración centralizada
- ✅ Documentación completa
- ✅ Manejo de errores consistente
- ✅ Logging con timestamps

## 📋 Configuración

### Dependencias (platformio.ini)
```ini
lib_deps = 
    bblanchon/ArduinoJson@^7.4.2
    mobizt/Firebase ESP32 Client@^4.0.0
    stuartpittaway/SX126XLT@^1.0.0
```

### Configuración Requerida (config.h)
- Credenciales WiFi
- Configuración Firebase
- Pines de conexión
- Parámetros de timing

## 🔧 Uso

### Compilación
```bash
pio run
```

### Subida al dispositivo
```bash
pio run --target upload
```

### Monitoreo serial
```bash
pio device monitor
```

## 📊 Flujo de Datos

1. **Recepción LoRa**: El dispositivo recibe mensajes de sensores
2. **Parsing**: Los datos se extraen y validan
3. **Almacenamiento**: Se envían a Firebase Firestore
4. **Vinculación**: Sistema de botón para aceptar solicitudes
5. **Monitoreo**: Logging continuo del estado del sistema

## 🛠️ Mantenimiento

### Agregar Nuevas Funcionalidades
1. Crear archivo de cabecera en `include/`
2. Implementar en `src/`
3. Incluir en `main.cpp`
4. Actualizar documentación

### Debugging
- Usar funciones `debugPrint()`, `errorPrint()`, `successPrint()`
- Monitorear serial a 115200 baudios
- Verificar logs con timestamps

## 📝 Notas de Desarrollo

- Todas las funciones están documentadas con Doxygen
- Se siguen las mejores prácticas de C++ para embebidos
- El código es compatible con ESP32 y PlatformIO
- Se mantiene compatibilidad con el código original

## 🔄 Migración desde Código Original

El código original de 613 líneas se ha reorganizado en:
- **main.cpp**: 120 líneas (solo lógica principal)
- **6 módulos especializados**: ~400 líneas total
- **Mejor mantenibilidad**: Cambios aislados por módulo
- **Reutilización**: Módulos independientes

---

**Autor**: Daniela Cañas y Gabriel Gimenez
**Fecha**: 2025
**Versión**: 2.0 (Modular) 