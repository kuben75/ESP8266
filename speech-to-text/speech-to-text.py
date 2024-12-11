import whisper
import paho.mqtt.client as mqtt
import sounddevice as sd
import numpy as np
import tempfile
import os
import ssl
from scipy.io.wavfile import write 

# Konfiguracja MQTT
MQTT_BROKER = "ffce3119.ala.eu-central-1.emqxsl.com"
MQTT_PORT = 8883
MQTT_TOPIC = "speech-to-text"
MQTT_USERNAME = "emqx"
MQTT_PASSWORD = "public"

# Inicjalizacja klienta MQTT z SSL
client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

client.tls_set(cert_reqs=ssl.CERT_NONE) 
client.connect(MQTT_BROKER, MQTT_PORT)

def record_and_transcribe():
    model = whisper.load_model("base")  # Załaduj model Whisper
    print("Nagrywanie...")

    duration = 5  # Długość nagrywania 
    sample_rate = 16000  # Próbkowanie

    # Nagrywanie dźwięku
    recording = sd.rec(int(duration * sample_rate), samplerate=sample_rate, channels=1, dtype="float32")
    sd.wait()

    # Zapis nagrania do pliku WAV za pomocą scipy.io.wavfile
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as temp_file:
        temp_file_path = temp_file.name

        scaled_recording = np.int16(recording * 32767)  
        write(temp_file_path, sample_rate, scaled_recording)  

    # Transkrypcja nagrania
    result = model.transcribe(temp_file_path, language="pl")
    os.unlink(temp_file_path) 
    return result["text"]

# Liczba iteracji nagrań (można sobie zmienić)
num_iterations = 2

for i in range(num_iterations):
    try:
        print(f"Nagrywanie {i + 1}/{num_iterations}")
        text = record_and_transcribe()
        print(f"Rozpoznany tekst: {text}")
        
        # Wyślij rozpoznany tekst do brokera MQTT
        client.publish(MQTT_TOPIC, text)
        print(f"Wysłano dane do MQTT: {text}")
    except Exception as e:
        print(f"Błąd: {e}")
