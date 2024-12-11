import paho.mqtt.client as mqtt
import ssl
import time
# MQTT settings
mqtt_broker = "ffce3119.ala.eu-central-1.emqxsl.com"
mqtt_port = 8883
mqtt_topic = "emqx/esp8266"
mqtt_username = "emqx" 
mqtt_password = "public"  

# InfluxDB settings
influxdb_url = "http://localhost:8086"
influxdb_token = "HogG9IpcvzKi6KbQMz7uva0HdX34qA1jmv-Kc7FEglB3DtgZI_6zCnjbMMmcARzO_SgfTjtGQfD3itpz0cQj_w=="
influxdb_org = "cdv"
influxdb_bucket = "arduino"


from influxdb_client import InfluxDBClient, Point, WriteOptions
client_influx = InfluxDBClient(url=influxdb_url, token=influxdb_token)
write_api = client_influx.write_api(write_options=WriteOptions(batch_size=1))

def on_message(client, userdata, message):
    payload = message.payload.decode()
    temperature = float(payload)
    print(f"Received temperature: {temperature}°C")

    point = Point("temperature").field("value", temperature)
    write_api.write(bucket=influxdb_bucket, org=influxdb_org, record=point)
    print("Temperature saved to InfluxDB")

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")
    client.subscribe(mqtt_topic)

def on_disconnect(client, userdata, rc):
    print("Disconnected from MQTT broker")

client = mqtt.Client()
client.username_pw_set(mqtt_username, mqtt_password)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

client.tls_set(certfile=None, keyfile=None, tls_version=ssl.PROTOCOL_TLSv1_2)  
client.tls_insecure_set(True)  

client.connect(mqtt_broker, mqtt_port, 60)

client.loop_start()

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Program zakończony")
    client.loop_stop()
    client.disconnect()
