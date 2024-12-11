#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>

const char *ssid = "Wow";           
const char *password = "kuben753";  

const int mqtt_port = 8883;                                      
const char *mqtt_broker = "ffce3119.ala.eu-central-1.emqxsl.com";  
const char *mqtt_topic_temp = "emqx/esp8266";                      // Temat dla temperatury
const char *mqtt_topic_speech = "speech-to-text";                  // Temat dla danych z Whisper
const char *mqtt_topic_video = "video-to-text";                    // Temat dla danych z YOLO
const char *mqtt_username = "emqx";                               
const char *mqtt_password = "public";                             

// Ustawienia NTP
const char *ntp_server = "pool.ntp.org"; 
const long gmt_offset_sec = 0;            
const int daylight_offset_sec = 0;       

// Ustawienia DS18B20
const int oneWireBus = 14; 
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Ustawienia LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  

BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----

)EOF";


String receivedSpeech = "";  // Dane z Whisper
String receivedVideo = "";   // Dane z YOLO
unsigned long lastMillis = 0;
bool displaySpeech = true;   

void connectToWiFi();
void connectToMQTT();
void syncTime();
void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  syncTime();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTT();
  sensors.begin();
  lcd.init();
  lcd.backlight();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void syncTime() {
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  Serial.print("Waiting for NTP time sync: ");
  while (time(nullptr) < 8 * 3600 * 2) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Time synchronized");
}

void connectToMQTT() {
  BearSSL::X509List serverTrustedCA(ca_cert);
  espClient.setTrustAnchors(&serverTrustedCA);
  while (!mqtt_client.connected()) {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic_speech);  // Subskrypcja dla speech-to-text
      mqtt_client.subscribe(mqtt_topic_video);  // Subskrypcja dla video-to-text
    } else {
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String message = String((char *)payload).substring(0, length);

    Serial.print("Temat: ");
  Serial.println(topic);
  Serial.print("Wiadomość: ");
  Serial.println(message);
  
  if (String(topic) == mqtt_topic_speech) {
    receivedSpeech = message;
    Serial.println("Zaktualizowano dane z Whisper");
  } else if (String(topic) == mqtt_topic_video) {
    receivedVideo = message;
    Serial.println("Zaktualizowano dane z YOLO");
  }
}

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTT();
  }
  mqtt_client.loop();

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("Czujnik nie podłączony!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Blad czujnika!");
  } else {
    // Wyświetlanie temperatury na górnej linii
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureC);
    lcd.print((char)223);
    lcd.print("C");

    // Przełączanie tekstu co 5 sekund
    if (millis() - lastMillis >= 3000) {
      displaySpeech = !displaySpeech;
      lastMillis = millis();
    }

    // Wyświetlanie tekstu na dolnej linii
    lcd.setCursor(0, 1);
    if (displaySpeech) {
      lcd.print("Txt: ");
      lcd.print(receivedSpeech);
    } else {
      lcd.print("Obj: ");
      lcd.print(receivedVideo);
    }

    String temperature_str = String(temperatureC);
    mqtt_client.publish(mqtt_topic_temp, temperature_str.c_str());

    Serial.print("Temperatura: ");
    Serial.println(temperatureC);
  }

  delay(4000);
}
