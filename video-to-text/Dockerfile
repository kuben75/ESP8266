FROM python:3.10-slim

WORKDIR /app
COPY . /app

RUN apt-get update && apt-get install -y \
    libgl1-mesa-glx \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir -r yolov5/requirements.txt

RUN pip install --no-cache-dir ultralytics paho-mqtt opencv-python-headless
CMD ["python", "video-to-text.py"]
