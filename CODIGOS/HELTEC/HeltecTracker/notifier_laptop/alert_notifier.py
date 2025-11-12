import firebase_admin
from firebase_admin import credentials, firestore
import google.auth.transport.requests
import google.oauth2.service_account
import requests
import time

# -------- CONFIGURACIÓN --------
SERVICE_ACCOUNT_PATH = r'C:/Users/danip/Documents/SafeRun_Tracker/SafeRun_Tracker_clean/CODIGOS/HELTEC/HeltecTracker/notifier_laptop/monitoreodeportistas-firebase-adminsdk-fbsvc-9f34f3b8dc.json'
DEVICE_ID = '456'          # Documento principal del dispositivo

FIRESTORE_SENSOR_FIELD = 'sensor4'
FIRESTORE_FCMTOKEN_FIELD = 'fcmToken'

# Endpoint HTTP v1
FCM_SEND_ENDPOINT = 'https://fcm.googleapis.com/v1/projects/monitoreodeportistas/messages:send'

# -------- INICIALIZAR FIREBASE --------
cred = credentials.Certificate(SERVICE_ACCOUNT_PATH)
firebase_admin.initialize_app(cred)
db = firestore.client()

# -------- OBTENER TOKEN DE ACCESO --------
def get_access_token():
    SCOPES = ['https://www.googleapis.com/auth/firebase.messaging']
    credentials_fcm = google.oauth2.service_account.Credentials.from_service_account_file(
        SERVICE_ACCOUNT_PATH, scopes=SCOPES)
    request = google.auth.transport.requests.Request()
    credentials_fcm.refresh(request)
    return credentials_fcm.token

# -------- OBTENER TOKEN FCM DESDE FIRESTORE --------
def get_fcm_token():
    doc_ref = db.collection('deviceData').document(DEVICE_ID)
    doc = doc_ref.get()
    if not doc.exists:
        print(f'❌ No existe el documento deviceData/{DEVICE_ID}')
        return None
    token = doc.get(FIRESTORE_FCMTOKEN_FIELD)
    if not token:
        print(f'⚠️ No se encontró el campo {FIRESTORE_FCMTOKEN_FIELD}')
    return token

# -------- ENVIAR NOTIFICACIÓN FCM --------
def send_push_notification(token):
    access_token = get_access_token()
    headers = {
        'Authorization': f'Bearer {access_token}',
        'Content-Type': 'application/json; UTF-8',
    }
    message = {
        "message": {
            "token": token,
            "notification": {
                "title": "¡Emergencia! 🚨",
                "body": "Su usuario puede estar en peligro. Revisa tu app SafeRun."
            },
            "android": {
                "notification": {
                    "sound": "emergency_alarm"
                }
            }
        }
    }
    resp = requests.post(FCM_SEND_ENDPOINT, headers=headers, json=message)
    print(f'Status FCM: {resp.status_code}')
    print(resp.text)

# -------- MONITOREAR FIRESTORE --------
def main():
    last_doc_id = None
    print('Esperando eventos de emergencia (por documento más reciente)...')
    

    fcm_token = get_fcm_token()
    if not fcm_token:
        return

    while True:
        try:
            docs_ref = db.collection('deviceData').document(DEVICE_ID).collection('data')
            docs = list(docs_ref.stream())
            if docs:
                docs_sorted = sorted(docs, key=lambda d: d.id, reverse=True)
                latest_doc = docs_sorted[0]
                if latest_doc.id != last_doc_id:
                    doc_data = latest_doc.to_dict()
                    sensor4_now = doc_data.get(FIRESTORE_SENSOR_FIELD, False)
                    print(f'VISTO: documento {latest_doc.id} → sensor4={sensor4_now}')
                    if sensor4_now:
                        print('🚨 ¡Emergencia detectada! Enviando notificación...')
                        send_push_notification(fcm_token)
                    last_doc_id = latest_doc.id
            else:
                print('⚠️ No hay documentos en la subcolección.')
        except Exception as e:
            print('❌ Error al consultar Firestore:', e)
        time.sleep(3)

if __name__ == '__main__':
    main()
