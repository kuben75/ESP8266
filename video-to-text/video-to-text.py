from ultralytics import YOLO
import paho.mqtt.client as mqtt
import ssl
import cv2
import time


# Konfiguracja MQTT
MQTT_BROKER = "ffce3119.ala.eu-central-1.emqxsl.com"
MQTT_PORT = 8883
MQTT_TOPIC = "video-to-text"
MQTT_USERNAME = "emqx"
MQTT_PASSWORD = "public"

MODEL_PATH = "yolov5s.pt"

# Ścieżka do pliku wideo (można zmienić)
VIDEO_PATH = "test2.mp4"

# Minimalny czas między wysyłaniem wiadomości (w sekundach)
SEND_INTERVAL = 2.0

def send_mqtt_message(client, message):
    client.publish(MQTT_TOPIC, message)
    print(f"Wysłano do MQTT: {message}")


# Konfiguracja klienta MQTT
def setup_mqtt():
    client = mqtt.Client(client_id="video-to-text")
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.tls_set(cert_reqs=ssl.CERT_NONE)
    client.connect(MQTT_BROKER, MQTT_PORT)
    return client


# Analiza pojedynczej klatki
def analyze_frame(model, frame, client, last_sent_time, last_message):
    results = model.predict(frame, save=False)

    # Pobranie danych obiektów
    try:
        detections = results[0].boxes.data
        class_names = results[0].names
        detected_objects = [class_names[int(x[5])] for x in detections if int(x[5]) in class_names]

        # Stworzenie wiadomości
        message = ", ".join(detected_objects) if detected_objects else "Brak wykrytych obiektów"
    except AttributeError:
        message = "Brak wykrytych obiektów"

    current_time = time.time()
    if current_time - last_sent_time >= SEND_INTERVAL and message != last_message:
        send_mqtt_message(client, message)
        return current_time, message  
    return last_sent_time, last_message 


def process_video():

    model = YOLO(MODEL_PATH)

    # Połączenie z MQTT
    client = setup_mqtt()

    # Otwieranie pliku wideo
    cap = cv2.VideoCapture(VIDEO_PATH)

    if not cap.isOpened():
        print("Nie można otworzyć pliku wideo.")
        return
    last_sent_time = 0
    last_message = None

    while True:

        ret, frame = cap.read()
        if not ret:
            print("Koniec pliku wideo lub problem z odczytem.")
            break

        # Analiza klatki
        last_sent_time, last_message = analyze_frame(model, frame, client, last_sent_time, last_message)

        # Opcjonalne: wyświetlanie klatki na ekranie
        cv2.imshow('Klatka wideo', frame)

        # Sprawdzenie, czy użytkownik chce przerwać
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("Przerwanie analizy.")
            break

    cap.release()
    cv2.destroyAllWindows()
    client.disconnect()


if __name__ == "__main__":
    process_video()
