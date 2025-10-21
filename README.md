#  Sistema de Detección de Caídas con Comunicación LoRa

Proyecto académico desarrollado en **UNEXPO** como parte de la tesis de Ingeniería Electrónica (Control).  
El sistema consiste en un **nodo portátil** para el usuario (deportista) y un **nodo receptor/base**, conectados mediante comunicación **LoRa**, con funciones de geolocalización, reproducción de música y transmisión de emergencias.

---

##  Características principales

- **Nodo portátil (emisor):**
  - Detección de caídas mediante acelerómetro **ADXL345**.
  - Geolocalización con módulo **GNSS UC6580**.
  - Comunicación **LoRa SX1262** integrada en **Heltec ESP32-S3 Wireless Tracker**.
  - Reproducción de música con **DFPlayer Mini** + microSD.
  - Interfaz de usuario: pantalla **OLED**, botones físicos e indicador háptico (motor vibrador).
  - Monitoreo de batería con ADC de 12 bits.

- **Nodo receptor (base):**
  - Recepción de paquetes LoRa y retransmisión a aplicación móvil o servidor.
  - Antena externa de mayor ganancia para ampliar cobertura.
  - Conectividad Wi-Fi/Ethernet para integración en la nube.
  - Registro y visualización de eventos en aplicación de monitoreo.

---

##  Arquitectura del sistema

1. **Captura de datos**: acelerómetro y GPS en el nodo portátil.  
2. **Procesamiento local**: algoritmo de detección de caídas en ESP32-S3.  
3. **Transmisión LoRa**: envío de coordenadas, estado y nivel de batería.  
4. **Recepción en nodo base**: validación y reenvío a la aplicación.  
5. **Visualización**: interfaz móvil con notificación de emergencias.  

---
## Créditos

- Desarrollado por: Daniela Cañas y Gabriel Gimenez
- Universidad Nacional Experimental Politécnica "Antonio José de Sucre" (UNEXPO)
- Proyecto de grado en Ingeniería Electrónica (Control)

---

## 📂 Estructura del repositorio
