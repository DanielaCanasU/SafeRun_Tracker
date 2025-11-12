# Notificador de Alertas SafeRun (laptop)

Este script permite monitorear remotamente Firestore y recibir una notificación push en tu app Android cuando sensor4 se active, incluso si la app está cerrada.

## Requisitos
- Python 3.x
- Dependencias: `firebase-admin`, `requests`
- Archivo de clave de servicio Firebase: `serviceAccountKey.json`
- API Key de Cloud Messaging (FCM)

## Instalación

1. Instala dependencias:
    ```bash
    pip install firebase-admin requests
    ```
2. Coloca tu archivo `serviceAccountKey.json` en este directorio.
3. Edita el archivo `alert_notifier.py` y configura:
    - `SERVICE_ACCOUNT_PATH`: Path donde está la clave de servicio
    - `API_KEY`: Tu clave de servidor de Cloud Messaging (FCM)
    - `DEVICE_ID`: El nombre/id de tu dispositivo en Firestore
    - `FCM_SEND_ENDPOINT`: Cambia `TU_ID_DE_PROYECTO` por el Project ID de Firebase

## Uso

Corre el script:
```bash
python alert_notifier.py
```

## ¿Cómo funciona?
- El script consulta Firestore cada pocos segundos para revisar el valor de `sensor4`.
- Si se detecta que `sensor4` cambió a `true`, se obtiene el token FCM de ese documento y se envía una notificación push a la app Android.

## Personalización
Puedes ajustar el tiempo de verificación, los mensajes de alerta y más editando `alert_notifier.py`.

---
