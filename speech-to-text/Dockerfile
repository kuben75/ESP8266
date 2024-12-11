FROM python:3.10-slim

WORKDIR /app
COPY . /app

# Dodanie repozytorium non-free dla `libportaudio`
RUN apt-get update && apt-get install -y \
    libportaudio2 \
    libportaudiocpp0 \
    portaudio19-dev \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# Instalacja zależności Python
RUN pip install --no-cache-dir whisper paho-mqtt sounddevice scipy

CMD ["python", "speech-to-text.py"]
